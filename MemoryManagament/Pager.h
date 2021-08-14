/*
 * Created on Tue July 13th 2021
 *
 * Copyright (c) 2021 - Vishal Kumar (vk2161)
 */

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "Process.h"
#include "extern_variables.h"

using namespace std;

#define traceA(fmt...)      do { if (aselect == 1) { printf(fmt); fflush(stdout); } } while(0)

class Pager {

    public:
        int no_of_frames;

        Pager(int f) {
            no_of_frames = f;
        }

        virtual frame_t * select_victim_frame() = 0; // virtual base class
        virtual void reset_age(frame_t * f) = 0; // virtual base class
};

class FFIO : public Pager {

    private:
        int hand;

    public:

        FFIO (int f, int h): Pager(f) {
            hand = h;
        }

        frame_t * select_victim_frame() {
            frame_t * f = &frame_table[hand];
            hand = hand + 1;
            if(hand > no_of_frames-1) {
                // Cause zero based index.
                hand = 0;
            }
            return f;
        }

        void reset_age(frame_t * f) {
            //Do nothing
        }
};


class Clock : public Pager {

    private:
        int hand;

    public:

        Clock(int f, int h): Pager(f) {
            hand = h;
        }

        frame_t * select_victim_frame() {
            int totalframes = no_of_frames + 1;
            while(totalframes--) {
                Process * p = frame_table[hand].proc;
                int v_page = frame_table[hand].v_page;
                if(!p->page_table[v_page].referenced) {
                    //referened bit is 0 just return the frame whih hand is pointing to.
                    frame_t * f = &frame_table[hand];
                    hand = hand + 1;
                    if(hand > no_of_frames-1) {
                        // Cause zero based index.
                        hand = 0;
                    }
                    return f;
                } else {
                    //We know that the referene bit is 1.
                    //Now we want to find a referene bit which is zero or if all frames are finished then it will essentially be a FIFO implementation
                    p->page_table[v_page].referenced = 0;
                    hand = hand + 1;
                    if(hand > no_of_frames - 1) {
                        // Cause zero based index.
                        hand = 0;
                    }
                }
            }
        }

        void reset_age(frame_t * f) {
            //Do nothing
        }
};


class Random : public Pager {

    private:
        int hand; // Here hand will act as offset.
    
    public:

        Random(int f, int h): Pager(f) {
            hand = h;
        }

        int myrandom() {
            hand =  hand % totalRandNo;
            int randomOff = randVector.at(hand) % no_of_frames; 
            hand++;
            return randomOff;
        }

        frame_t * select_victim_frame() {
            return &frame_table[myrandom()];
        }

        void reset_age(frame_t * f) {
            //Do nothing
        }
};

class NRU : public Pager {

    private:
        int hand;
        unsigned long last_reset_instruction;
        unsigned reset_reference             :1;

    public:

        NRU(int f, int h): Pager(f) {
            hand = h;
            last_reset_instruction = 0;
            reset_reference = 0;
        }

        frame_t * select_victim_frame() {

            //Here keep track of a boolean variable when 50 instructions have past then we need to scan all the valid page table 
            // entries and then set the referenced bit as 1.

            //If the 50 instructions hasn't passed and then we need to find the first frame index which has the lowest class-index.
            //Obviously class-index can not go below 0 so the moment you find the frame with the class-index 0 just return that frame and
            //advance the hand.

            //Note if it is the 50th instruction then start scanning the page table enntries linked to the frames, we need to return the frame 
            //which has lowest class-index, in case we find class-index 0, we need to return that frame but still we need to continue to scan to 
            //reset the reference bit.

            //Okay let's start
            if(inst_count - last_reset_instruction >= 50) {
                reset_reference = 1;
                last_reset_instruction = inst_count;
            }

            traceA("ASELECT: hand= %d %d | ",hand,reset_reference);
            
            int totalframes = no_of_frames;
            frame_t * victim_frame;
            int lowest_class_index = INT_MAX; 
            int curr_hand = hand;
            int no_frames_scanned = 0;
            while(totalframes--) {
                Process * p = frame_table[hand].proc;
                int v_page = frame_table[hand].v_page;

                int r_bit = p->page_table[v_page].referenced;
                int m_bit = p->page_table[v_page].modified;
                int curr_class_index = 2*r_bit + m_bit;
                if(curr_class_index < lowest_class_index) {
                    lowest_class_index = curr_class_index;
                    victim_frame = &frame_table[hand];
                    curr_hand = hand;
                }
                
                if(curr_class_index == 0 && !reset_reference) {
                    hand = hand + 1;
                    if(hand > no_of_frames - 1) {
                        // Cause zero based index.
                        hand = 0;
                    }
                    no_frames_scanned++;
                    traceA("%d %d  %d\n",lowest_class_index,victim_frame->frame_id,no_frames_scanned);
                    return victim_frame;
                }

                //Now if reset reference is true then reset the r bit
                if(reset_reference) {
                    p->page_table[v_page].referenced = 0;
                }

                hand = hand + 1;
                if(hand > no_of_frames - 1) {
                    // Cause zero based index.
                    hand = 0;
                }
                no_frames_scanned++;
            }
            hand = curr_hand + 1;
            if(hand > no_of_frames - 1) {
                // Cause zero based index.
                hand = 0;
            }
            reset_reference = 0;
            traceA("%d %d  %d\n",lowest_class_index,victim_frame->frame_id,no_frames_scanned);
            return victim_frame;
        }

        void reset_age(frame_t * f) {
            //Do nothing
        }
};



class Aging: public Pager {
    private:
        int hand; // Here hand will act as offset.
    
    public:

        Aging(int f, int h) : Pager(f) {
            hand = h;
        }

        frame_t * select_victim_frame() {
            traceA("ASELECT %d-%d |",hand,((hand+no_of_frames-1)%no_of_frames));
            int totalframes = no_of_frames-1;
            int curr_hand = hand;
            
            int v_page = frame_table[hand].v_page;

            frame_table[hand].aging = frame_table[hand].aging >> 1;

            //When do we have to set the leading bit to 1? IF it is currently being referenced
            if(frame_table[hand].proc->page_table[v_page].referenced) {
                frame_table[hand].aging = (frame_table[hand].aging | 0x80000000);
                frame_table[hand].proc->page_table[v_page].referenced = 0;
            }
            traceA(" %d:%x",hand,frame_table[hand].aging);

            int max_age = frame_table[hand].aging;
            while(totalframes--) {
                hand = hand + 1;
                if(hand > no_of_frames - 1) {
                    // Cause zero based index.
                    hand = 0;
                }
                int v_page = frame_table[hand].v_page;
                frame_table[hand].aging = frame_table[hand].aging >> 1;
                if(frame_table[hand].proc->page_table[v_page].referenced) {
                    frame_table[hand].aging = (frame_table[hand].aging | 0x80000000);
                    frame_table[hand].proc->page_table[v_page].referenced = 0;
                }
                if(frame_table[hand].aging < max_age) {
                    curr_hand = hand;
                    max_age = frame_table[hand].aging;
                }
                traceA(" %d:%x",hand,frame_table[hand].aging);
            }
            hand = curr_hand+1;
            if(hand > no_of_frames - 1) {
                // Cause zero based index.
                hand = 0;
            }
            traceA(" | %d\n",curr_hand);
            return &frame_table[curr_hand];
        }

        void reset_age(frame_t * f) {
            f->aging = 0;
        }
};


class WS : public Pager {
    private:
        int hand;

    public:

        WS(int f, int h) : Pager(f) {
            hand = h;
        }

        frame_t * select_victim_frame() {
            unsigned long smallest_time = -1;
            int totalframes = no_of_frames;
            int curr_hand = hand;
            while(totalframes--) {
                Process * p = frame_table[hand].proc;
                int v_page = frame_table[hand].v_page;
                if(!p->page_table[v_page].referenced && (inst_count - frame_table[hand].time_last_used) > 49) {
                    //reference bit is zero and inst count - last time used is > 49.
                    frame_t * f = &frame_table[hand];
                    hand = hand + 1;
                    if(hand > no_of_frames-1) {
                        // Cause zero based index.
                        hand = 0;
                    }
                    return f;
                } else if(!p->page_table[v_page].referenced && (inst_count - frame_table[hand].time_last_used) <= 49) {
                    
                    //Here we know that the reference bit is zero but the time hasn't passed. So we need to so something.
                    // We need to remember the smallest time.
                    if(frame_table[hand].time_last_used < smallest_time) {
                        curr_hand = hand;
                        smallest_time = frame_table[hand].time_last_used;
                    }

                    hand = hand + 1;
                    if(hand > no_of_frames - 1) {
                        // Cause zero based index.
                        hand = 0;
                    }

                } else {
                    //We know that the referene bit is 1, so we want to record the current time as time_last_used and reset the reference bit.
                    frame_table[hand].time_last_used = inst_count;
                    p->page_table[v_page].referenced = 0;
                    hand = hand + 1;
                    if(hand > no_of_frames - 1) {
                        // Cause zero based index.
                        hand = 0;
                    }

                }
            }
            hand = curr_hand+1;
            if(hand > no_of_frames - 1) {
                // Cause zero based index.
                hand = 0;
            }
            return &frame_table[curr_hand];
        }

        void reset_age(frame_t * f) {
            f->time_last_used = inst_count;
        }
};