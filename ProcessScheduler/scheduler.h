#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <queue> 
#include "pcevent.h"

using namespace std;

int strace = 0;
#define traceSchd(fmt...)      do { if (strace == 1) { printf(fmt); fflush(stdout); } } while(0)

struct ProcessList {
    Process * proc;
    ProcessList * next;
};

class Scheduler {
    
    public:
        ProcessList * active = NULL;
        ProcessList * inactive = NULL;

        int quantum;

        Scheduler(int q, int tr) {
            strace = tr;
            quantum = q;
        }

        int getQuantum() {
            return quantum;
        }

        virtual void addProcess(Process * proc) = 0;
        virtual Process * getNextProcess() = 0;
        virtual bool test_preempt() = 0;
};


//First In First Out
class FFIO: public Scheduler {

    public:

        FFIO (int q, int tr) : Scheduler(q, tr) {}

        void addProcess(Process * proc) {
            if(active == NULL) {
                //This is the first time
                struct ProcessList* plist = (ProcessList*) malloc(sizeof(ProcessList));
                plist->proc = proc;
                plist->next = NULL;
                active = plist;
            } else {
                ProcessList * last = active;
                while(last->next != NULL) {
                    last = last->next;
                }
                struct ProcessList* plist = (ProcessList*) malloc(sizeof(ProcessList));
                last->next = plist;
                plist->proc = proc;
                plist->next = NULL;
            }
        }

        Process * getNextProcess() {
            // Should i check that head is null ? Yes we need to.
            if(active == NULL) {
                return NULL;
            }
            ProcessList * plist = active;
            active = active -> next;
            return plist->proc;
        }

        bool test_preempt() {
            return false;
        }
};



//Last Come First Serve
class LCFS: public Scheduler {

    public:

        LCFS (int q, int tr) : Scheduler(q, tr) {}

        void addProcess(Process * proc) {

            //We always need to add the process in the first place.
            struct ProcessList* plist = (ProcessList*) malloc(sizeof(ProcessList));
            plist->proc = proc;
            plist->next = active;
            active = plist;
        }

        Process * getNextProcess() {
            // Should i check that head is null ? Yes we need to.
            if(active == NULL) {
                return NULL;
            }
            ProcessList * plist = active;
            active = active -> next;
            return plist->proc;
        }

        bool test_preempt() {
            return false;
        }

};

//Shortest Remaning Time First
class SRTF: public Scheduler {

    public:

        SRTF (int q, int tr) : Scheduler(q, tr) {}

        void addProcess(Process * proc) {

            //If incoming process is the first or has lesser reamining time than active then insert at the begining.
            if(active == NULL || proc->remainingTime < active->proc->remainingTime) {
                struct ProcessList* plist = (ProcessList*) malloc(sizeof(ProcessList));
                plist->proc = proc;
                plist->next = active;
                active = plist;
            }

            //Else find the right spot for it.
            else {
                ProcessList * current = active;
                while(current->next != NULL && proc->remainingTime >=  current->next->proc->remainingTime) {
                    current = current->next;
                }
                struct ProcessList* plist = (ProcessList*) malloc(sizeof(ProcessList));
                plist->proc = proc;
                plist->next = current->next;
                current->next = plist;
            }  
        }

        Process * getNextProcess() {
            // Should i check that head is null ? Yes we need to.
            if(active == NULL) {
                return NULL;
            }
            ProcessList * plist = active;
            active = active -> next;
            return plist->proc;
        }

        bool test_preempt() {
            return false;
        }

};


//Round Robbin
class RR: public Scheduler {

    public:

        RR (int q, int tr) : Scheduler(q, tr) {}

        void addProcess(Process * proc) {
            //Every time process is added in round robin reset the dynamic priority to static priority - 1
            proc -> dynamicPriority = proc -> staticPriority - 1;

            if(active == NULL) {
                //This is the first time
                struct ProcessList* plist = (ProcessList*) malloc(sizeof(ProcessList));
                plist->proc = proc;
                plist->next = NULL;
                active = plist;
            } else {
                ProcessList * last = active;
                while(last->next != NULL) {
                    last = last->next;
                }
                struct ProcessList* plist = (ProcessList*) malloc(sizeof(ProcessList));
                last->next = plist;
                plist->proc = proc;
                plist->next = NULL;
            }
        }

        Process * getNextProcess() {
            // Should i check that head is null ? Yes we need to.
            if(active == NULL) {
                return NULL;
            }
            ProcessList * plist = active;
            active = active -> next;
            return plist->proc;
        }

        bool test_preempt() {
            return false;
        }

};

//Multi Level Priority Scheduler
class PRIO: public Scheduler {

    private:
        vector<ProcessList *> *mlqActive;
        vector<ProcessList *> *mlqInactive;
        int numPrio;
        void swapVectorPointers() {
            traceSchd("switched queues\n");
            vector<ProcessList *> *temp = mlqActive;
            mlqActive = mlqInactive;
            mlqInactive = temp;
        }

        Process * getNextProcessFromVector(vector<ProcessList *> *vec) {
            for(int i = 0; i<numPrio; i++) {
                if(vec->operator[](i) != NULL) {
                    ProcessList * head = vec->operator[](i);
                    ProcessList * plist = head;
                    head = head -> next;
                    vec->operator[](i) = head;
                    return plist->proc;
                }
            }
            return NULL;
        }

    public:

        PRIO (int q, int n, int tr) : Scheduler(q, tr) {
            numPrio = n;
            mlqActive = new vector<ProcessList *> (numPrio);
            mlqInactive = new vector<ProcessList *> (numPrio);
        }

        void addProcess(Process * proc) {

            vector<ProcessList *> *currentVector;            
            int prio = proc->dynamicPriority;
            if(prio == -1) {
                prio = proc->staticPriority - 1;
                proc -> dynamicPriority = prio;
                currentVector = mlqInactive;
            } else {
                currentVector = mlqActive;
            }

            // numPrio - prio is the way we sort it in reverse order.

            ProcessList * head = currentVector->operator[](numPrio-prio-1);
            //Now use this as the pointer and do as i say.
            if(head == NULL) {
                //This is the first time at this level.
                struct ProcessList* plist = (ProcessList*) malloc(sizeof(ProcessList));
                plist->proc = proc;
                plist->next = NULL;
                head = plist;
                currentVector->operator[](numPrio-prio-1) = head;
            } else {
                ProcessList * last = head;
                while(last->next != NULL) {
                    last = last->next;
                }
                struct ProcessList* plist = (ProcessList*) malloc(sizeof(ProcessList));
                last->next = plist;
                plist->proc = proc;
                plist->next = NULL;
            }
        }

        Process * getNextProcess() {

            //Here first trace the scheduler queues
            traceSchd("{ ");
            for(int i = 0; i<numPrio; i++) {
                traceSchd("[");
                ProcessList * head = mlqActive->operator[](i);
                while(head != NULL) {
                    traceSchd("%d",head->proc->pid);
                    head = head -> next;
                    if(head != NULL) {
                        traceSchd(",");
                    }
                }
                traceSchd("]");
            }

            traceSchd("}"); traceSchd(" : ");

            traceSchd("{ ");
            for(int i = 0; i<numPrio; i++) {
                traceSchd("[");
                ProcessList * head = mlqInactive->operator[](i);
                while(head != NULL) {
                    traceSchd("%d",head->proc->pid);
                    head = head -> next;
                    if(head != NULL) {
                        traceSchd(",");
                    }
                }
                traceSchd("]");
            }

            traceSchd("}"); traceSchd(" : \n");

            Process * proc = getNextProcessFromVector(mlqActive);
            if(proc == NULL) {
                // Swap the vector pointers
                swapVectorPointers();
                return getNextProcessFromVector(mlqActive);
            } else {
                return proc;
            }
        }

        bool test_preempt() {
            return false;
        }

};


//Multi Level Priority Scheduler
class PREPRIO: public Scheduler {

    private:
        vector<ProcessList *> *mlqActive;
        vector<ProcessList *> *mlqInactive;
        int numPrio;
        void swapVectorPointers() {
            traceSchd("switched queues\n");
            vector<ProcessList *> *temp = mlqActive;
            mlqActive = mlqInactive;
            mlqInactive = temp;
        }

        Process * getNextProcessFromVector(vector<ProcessList *> *vec) {
            for(int i = 0; i<numPrio; i++) {
                if(vec->operator[](i) != NULL) {
                    ProcessList * head = vec->operator[](i);
                    ProcessList * plist = head;
                    head = head -> next;
                    vec->operator[](i) = head;
                    return plist->proc;
                }
            }
            return NULL;
        }

    public:

        PREPRIO (int q, int n, int tr) : Scheduler(q, tr) {
            numPrio = n;
            mlqActive = new vector<ProcessList *> (numPrio);
            mlqInactive = new vector<ProcessList *> (numPrio);
        }

        void addProcess(Process * proc) {

            vector<ProcessList *> *currentVector;            
            int prio = proc->dynamicPriority;
            if(prio == -1) {
                prio = proc->staticPriority - 1;
                proc -> dynamicPriority = prio;
                currentVector = mlqInactive;
            } else {
                currentVector = mlqActive;
            }

            // numPrio - prio is the way we sort it in reverse order.

            ProcessList * head = currentVector->operator[](numPrio-prio-1);
            //Now use this as the pointer and do as i say.
            if(head == NULL) {
                //This is the first time at this level.
                struct ProcessList* plist = (ProcessList*) malloc(sizeof(ProcessList));
                plist->proc = proc;
                plist->next = NULL;
                head = plist;
                currentVector->operator[](numPrio-prio-1) = head;
            } else {
                ProcessList * last = head;
                while(last->next != NULL) {
                    last = last->next;
                }
                struct ProcessList* plist = (ProcessList*) malloc(sizeof(ProcessList));
                last->next = plist;
                plist->proc = proc;
                plist->next = NULL;
            }
        }

        Process * getNextProcess() {

            //Here first trace the scheduler queues
            traceSchd("{ ");
            for(int i = 0; i<numPrio; i++) {
                traceSchd("[");
                ProcessList * head = mlqActive->operator[](i);
                while(head != NULL) {
                    traceSchd("%d",head->proc->pid);
                    head = head -> next;
                    if(head != NULL) {
                        traceSchd(",");
                    }
                }
                traceSchd("]");
            }

            traceSchd("}"); traceSchd(" : ");

            traceSchd("{ ");
            for(int i = 0; i<numPrio; i++) {
                traceSchd("[");
                ProcessList * head = mlqInactive->operator[](i);
                while(head != NULL) {
                    traceSchd("%d",head->proc->pid);
                    head = head -> next;
                    if(head != NULL) {
                        traceSchd(",");
                    }
                }
                traceSchd("]");
            }

            traceSchd("}"); traceSchd(" : \n");

            Process * proc = getNextProcessFromVector(mlqActive);
            if(proc == NULL) {
                // Swap the vector pointers
                swapVectorPointers();
                return getNextProcessFromVector(mlqActive);
            } else {
                return proc;
            }
        }

        bool test_preempt() {
            return true;
        }

};