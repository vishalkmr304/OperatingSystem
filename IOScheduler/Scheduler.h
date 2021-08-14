#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <queue> 
#include <list>
#include <cmath>
#include <algorithm>
#include "IO.h"
#include "extern_variables.h"

using namespace std;

int qtrace = 0;
#define traceQ(fmt...)      do { if (qtrace == 1) { printf(fmt); fflush(stdout); } } while(0)


class Scheduler {
    
    public:
        Scheduler(int tr) {
            qtrace = tr;
        }

        virtual void addIORequest(io_req * io) = 0;
        virtual io_req * getIORequest() = 0;

};


//First In First Out
class FFIO: public Scheduler {

    private:
        queue<io_req *> ioQ;

    public:

        FFIO (int tr) : Scheduler(tr) {}

        void addIORequest(io_req * io) {
            ioQ.push(io);
        }

        io_req * getIORequest() {
            // Should i check that queue is null ? Yes we need to.
            if(!ioQ.empty()) {
                io_req * req = ioQ.front();
                ioQ.pop();
                return req;
            } else {
                return NULL;
            }
            
        }
};


//Shortest Seek Time First
class SSTF: public Scheduler {

    private:
        list<io_req *> ioList;

    public:

        SSTF (int tr) : Scheduler(tr) {}

        void addIORequest(io_req * io) {

            //Since when i am adding the process the shortest seek time is impossible to determine because seek position can be changed.
            //cout<<"Saving o list here"<<endl;
            ioList.push_back(io);
        }

        // For this we need to have global seek position otherwise it is impossible to determine.
        
        io_req * getIORequest() {

            io_req * shortest_seek = NULL;
            list<io_req *>::iterator it;
            if(!ioList.empty()) {
                
                int min_seek_pos = abs((*(ioList.begin()))->track_no - curr_seek_pos);
                shortest_seek = *(ioList.begin());
                //printf("Min Seek Pos=%d\n",min_seek_pos);
                for(it = ioList.begin(); it != ioList.end(); it++) {
                    if(abs((*it)->track_no - curr_seek_pos) < min_seek_pos) {
                        min_seek_pos = abs((*it)->track_no - curr_seek_pos);
                        shortest_seek = *it;
                    }
                }
            } else {
                return NULL;
            }
            ioList.remove(shortest_seek);
            return shortest_seek;
        }

};


// LOOK
class LOOK : public Scheduler {

    private:
        int dir;
        list<io_req *> ioList;

    public:

        LOOK (int tr) : Scheduler(tr) {
            dir = 1; // initially the direction is moving forward
        }

        void addIORequest(io_req * io) {
            //Add smartly?
            //The need is to find the current direction i think it will be better if i insert it in sorted order.
            //Inserting in sorted order complicates things. dont do it. Because i did it so that i could traverse from back but
            //then i miss the insertion order it chaotic.
            ioList.push_back(io);
            
        }

        io_req * getIORequest() {
            //Traverse the whole list and find me the one element which is just greater than the curr_seek_pos
            //in short min(element->track_no - curr_seek_pos)

            if(ioList.empty()) {
                return NULL;
            }

            traceQ("\tGot: (");
            for(list<io_req *>::iterator i = ioList.begin(); i != ioList.end(); i++) {
                    traceQ("%d:%d ",(*i)->inst_no,(*i)->track_no);
            }
            traceQ(") --> %d dir=%d\n",curr_seek_pos,dir);

            io_req * io = NULL;
            list<io_req *>::iterator it;
            bool found = false;
            int min_diff;
            //find the first non negative diff
            for(it = ioList.begin(); it != ioList.end() ; it++) {
                int diff = ((*it) -> track_no - curr_seek_pos) * dir;
                if(diff >= 0) {
                    min_diff = diff;
                    found = true;
                    io = (*it);
                    break;
                }
            }

            //If we couldn't find it then we dont do the next for, and we reverse the direction.
            if(found) {
                for(it = ioList.begin(); it != ioList.end(); it++) {
                    int diff = ((*it) -> track_no - curr_seek_pos) * dir;
                    if(diff >= 0 && diff < min_diff) {
                        min_diff = diff;
                        io = (*it);
                    }
                }
            } else {
                //traceQ("Reversing diretion, initial dir=%d ",dir);
                //reverse the direction and theen we know all the elements
                dir = -dir;
                //traceQ("after switching=%d\n",dir);
                int min_diff = ((*ioList.begin())->track_no - curr_seek_pos) * dir;
                io = (*ioList.begin());
                list<io_req *>::iterator i;
                for(i = ioList.begin(); i != ioList.end(); i++) {
                    int diff = ((*i) -> track_no - curr_seek_pos) * dir;
                    if(diff >= 0 && diff < min_diff) {
                        min_diff = diff;
                        io = (*i);
                    }
                }
                
            }

            if(io != NULL) {
                ioList.remove(io);
            }
            return io;
        }
};


// CLOOK
class CLOOK : public Scheduler {

    private:
        int dir;
        list<io_req *> ioList;

    public:

        CLOOK (int tr) : Scheduler(tr) {}

        void addIORequest(io_req * io) {
            //Add smartly?
            //The need is to find the current direction i think it will be better if i insert it in sorted order.
            if(!ioList.empty()) {
                list<io_req *>::iterator it;                
                bool found = false;
                it = ioList.begin();
                while(it != ioList.end() && !found) {
                    if(io->track_no >= (*it)->track_no) {
                        it++;
                    } else {
                        found = true;
                    }
                }
                ioList.insert(it,io);
            } else {
                ioList.push_back(io);
            }
        }

        io_req * getIORequest() {
            //First check if the curr_seek is 0
            //If it is then we need to take the first element of the list, 
            //otherwise we try to find the first in the given direction.

            if(curr_seek_pos == 0) {
                io_req * s_seek = *(ioList.begin());
                ioList.remove(s_seek);
                dir = 1;
                return s_seek;
            } else {
                //Direction will only change when there are no elements which are greater than the curr_seek_pos
                io_req * io = NULL;
                traceQ("\tGot: (");
                for(list<io_req *>::iterator i=ioList.begin(); i!=ioList.end();i++) {
                    traceQ("%d:%d ",(*i)->inst_no,(*i)->track_no);
                }
                traceQ(") --> %d dir=%d\n",curr_seek_pos,dir);

                list<io_req *>::iterator it;
                it = ioList.begin();
                bool found = false;
                while(it != ioList.end() && !found) {
                    if((*it)->track_no >= curr_seek_pos) {
                        io = (*it);
                        ioList.remove(io);
                        found = true;
                    } else {
                        it++;
                    }
                }
                
                if(!found) {
                    // This means we did not find the element we were looking for and we need to switch the direction.
                    io = *(ioList.begin());
                    ioList.remove(io);
                }
                return io;
            }

        }
};


// FLOOK
class FLOOK : public Scheduler {

    private:
        int dir;
        list<io_req *> * activeQ;
        list<io_req *> * addQ;

    public:

        FLOOK (int tr) : Scheduler(tr) {
            activeQ = new list<io_req *>;
            addQ = new list<io_req *>;
            dir = 1; // initially the direction is moving forward
        }

        void addIORequest(io_req * io) {
            //Add smartly? Only to add-queue
            //The need is to find the current direction i think it will be better if i insert it in sorted order.
            if(!addQ->empty()) {
                list<io_req *>::iterator it;                
                bool found = false;
                it = addQ->begin();
                while(it != addQ->end() && !found) {
                    if(io->track_no >= (*it)->track_no) {
                        it++;
                    } else {
                        found = true;
                    }
                }
                addQ->insert(it,io);
            } else {
                addQ->push_back(io);
            }
        }

        io_req * getIORequest() {

            if(activeQ->empty()) {
                list<io_req *> * temp = activeQ;
                activeQ = addQ;
                addQ = temp;
            }

            if(activeQ->empty()) {
                return NULL;
            }

            traceQ("\tGot: (");
            for(list<io_req *>::iterator i = activeQ->begin(); i != activeQ->end(); i++) {
                    traceQ("%d:%d ",(*i)->inst_no,(*i)->track_no);
            }
            traceQ(") --> %d dir=%d\n",curr_seek_pos,dir);

            io_req * io = NULL;
            list<io_req *>::iterator it;
            bool found = false;
            int min_diff;
            //find the first non negative diff
            for(it = activeQ->begin(); it != activeQ->end() ; it++) {
                int diff = ((*it) -> track_no - curr_seek_pos) * dir;
                if(diff >= 0) {
                    min_diff = diff;
                    found = true;
                    io = (*it);
                    break;
                }
            }

            //If we couldn't find it then we dont do the next for, and we reverse the direction.
            if(found) {
                for(it = activeQ->begin(); it != activeQ->end(); it++) {
                    int diff = ((*it) -> track_no - curr_seek_pos) * dir;
                    if(diff >= 0 && diff < min_diff) {
                        min_diff = diff;
                        io = (*it);
                    }
                }
            } else {
                
                //reverse the direction and theen we know all the elements
                dir = -dir;
                
                int min_diff = ((*activeQ->begin())->track_no - curr_seek_pos) * dir;
                io = (*activeQ->begin());
                list<io_req *>::iterator i;
                for(i = activeQ->begin(); i != activeQ->end(); i++) {
                    int diff = ((*i) -> track_no - curr_seek_pos) * dir;
                    if(diff >= 0 && diff < min_diff) {
                        min_diff = diff;
                        io = (*i);
                    }
                }
                
            }

            if(io != NULL) {
                activeQ->remove(io);
            }
            return io;
        }

};