/*
 * Created on Tue June 29th 2021
 *
 * Copyright (c) 2021 - Vishal Kumar (vk2161)
 */
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
using namespace std;

enum ProcessState {CREATED, TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT};

typedef struct Process {
    int pid;
    int arrivalTime;
    int totalCpuTime;
    int cpuBurst;
    int ioBurst;
    int totalIoTime;
    int processTimestamp; //This is the time when this process was last started.
    int remainingTime; //remaningTime for CPU.
    int staticPriority;
    int dynamicPriority;
    int cpuWaitTime;
    int finishTime;
    int turnAroundTime;
    int preemptCpuBurst;
    int currentCpuBurst;
}Process;

struct Event {
        int evtTimestamp;
        ProcessState oldState;
        ProcessState newState;
        Process *proc;
        Event *next;
};

class EventList {
    private:
        Event *head;

    public:

        EventList() {
            head = NULL;
        }

        void addEvent(Event *evnt) {
            if(head == NULL) {
                // There are no events. Just enter it into the list.
                evnt->next = head;
                head = evnt;
            }
            
            else {
                // Now check one by one.
                Event * current = head;
                if(current -> next == NULL) {
                    // There's only one node 
                    if(evnt -> evtTimestamp < current -> evtTimestamp) {
                        //Insert at the begining
                        evnt -> next = current;
                        head = evnt;
                    } else {
                            //Insert after the head
                            evnt->next = current->next;
                            current->next = evnt;                        
                    }
                } else {
                    // there are atleast two nodes.
                    Event * previous = NULL;
                    bool found = false;
                    while(current != NULL && !found) {
                        if(evnt->evtTimestamp >=  current->evtTimestamp) {
                            previous = current;
                            current = current->next;
                        } else {
                            found = true;
                        }
                    }

                    if(previous == NULL) {
                        //This means we are inserting at the begining. Make sure to change the head
                        evnt->next = current;
                        head = evnt;
                    } else {
                        evnt -> next = current;
                        previous -> next = evnt;
                    }
                }
            }           
        }

        Event* getEvent() {
            if(head == NULL) {
                return NULL;
            } else {
                Event *tmp = head;
                head = head -> next;
                return tmp;
            }
        }

        int getNextEventTime() {
            //return the next event time, return -1 if there are no events.
            if(head == NULL) {
                return -1;
            } else {
                return head->evtTimestamp;
            }
        }

        void deleteEvent(int pidToRemove) {
            if(head != NULL) {
                // Check if there's only head is left.
                if(head -> next == NULL || head -> proc -> pid == pidToRemove) {
                    head = head -> next;
                } else {
                    //There are more events find the process id and remove from the event queue.
                    Event * previous = NULL;
                    Event * current = head;
                    while(current->proc->pid != pidToRemove) {
                        previous = current;
                        current = current -> next;
                    }
                    previous -> next = current -> next;
                }
            } else {
                //There is nothing to delete, cannot do anything.
            }
        }

        Event * getHead() {
            return head;
        }
};

int offset = 0;
int globalpid = 0;
size_t len = 0;

class PCEvent {
    private:
        char * line;
        int linelen;
        EventList evnLst;
        vector<int> randVector;
        int totalRandNo;
    public:

        int myrandom(int burst) {
            int index =  offset % totalRandNo;
            int randomOff =  1 + (randVector.at(index) % burst); 
            offset++;
            return randomOff;
        } 

        void initializeFiles(char * inputFile, char * randomFile, int numprio) {
            FILE * fpInput = fopen(inputFile,"r");
            if(fpInput == NULL) {
                cout<<"Not a valid inputfile"<<" "<<"<"<<inputFile<<">"<<endl;
                exit(EXIT_FAILURE);
            }
            FILE * fpRandom = fopen(randomFile,"r");
            if(fpRandom == NULL) {
                cout<<"Not a valid random file"<<" "<<"<"<<randomFile<<">"<<endl;
                exit(EXIT_FAILURE);
            }
            //Read the random file first.
            //The first line is the total random numbbers in the file.
            getline(&line, &len, fpRandom);
            totalRandNo = atoi(line);
            while((linelen = getline(&line, &len, fpRandom)) != -1) {
                randVector.push_back(atoi(line));
            }

            //Now read the input file.
            while((linelen = getline(&line, &len, fpInput)) != -1) {
                    char * tmstmp = strtok(line," \n\t");
                    if(tmstmp != NULL) {
                        int eventTimeStamp = atoi(tmstmp);
                        Process *proc = (Process *)malloc(sizeof(Process));
                        proc->pid = globalpid;
                        proc->arrivalTime = eventTimeStamp;
                        int tct = atoi(strtok(NULL," "));
                        proc->totalCpuTime = tct;
                        proc->remainingTime = tct;
                        proc->cpuBurst = atoi(strtok(NULL," "));
                        proc->ioBurst = atoi(strtok(NULL," "));
                        proc->processTimestamp = -1; // This indicates that the process never ran.
                        proc->preemptCpuBurst = -1;  // This indicates the process was never preempted, otherwise this value will tell us 
                                                     // how many cpuBurst is remaining when this process was preempted.
                        proc->totalIoTime = 0;
                        proc->cpuWaitTime = 0;
                        int sP = myrandom(numprio);
                        proc->staticPriority = sP;
                        proc->dynamicPriority = sP - 1; 
                        Event *evnt = (Event *)malloc(sizeof(Event));
                        evnt->evtTimestamp = eventTimeStamp;
                        evnt->oldState = CREATED;
                        evnt->newState = TRANS_TO_READY;
                        evnt->proc = proc;
                        evnLst.addEvent(evnt);
                        globalpid++;
                    }
            }

        }

        Event* getEvent() {
            return evnLst.getEvent();
        }

        void putEvent(Event * evnt) {
            evnLst.addEvent(evnt);
        }

        void rmEvent(int pid) {
            evnLst.deleteEvent(pid);
        }

        int getNextEventTime() {
            return evnLst.getNextEventTime();
        }

        Event * getHead() {
            return evnLst.getHead();
        }

};

