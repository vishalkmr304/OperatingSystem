/*
 * Created on Tue July 2nd 2021
 *
 * Copyright (c) 2021 - Vishal Kumar (vk2161)
 */
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "scheduler.h"
#include <map>
#include <utility>

using namespace std;

int dotrace = 0;
int verbose = 0;
int schdTrace = 0;
int eventTrace = 0;

#define trace(fmt...)      do { if (dotrace == 1) { printf(fmt); fflush(stdout); } } while(0)
#define traceEvents(fmt...)      do { if (eventTrace == 1) { printf(fmt); fflush(stdout); } } while(0)

map<int, Process *> pMap;
vector<pair<int, int> > concurrentIo;

const char * getState(ProcessState state) {
    if(state == TRANS_TO_READY) {
        return "READY";
    } else if(state == TRANS_TO_RUN) {
        return "RUNNG";
    } else if(state == TRANS_TO_BLOCK) {
        return "BLOCK";
    } else if(state == CREATED){
        return "CREATED";
    } else {
        return "PREEMPT";
    }
}

void displayEventQueue(PCEvent t) {
    Event * ele = t.getHead();
    while(ele != NULL) {
        traceEvents("  %d:%d:%s",ele->evtTimestamp,ele->proc->pid,getState(ele->newState));
        ele = ele -> next;
    }
}

void displayEventQueueWithoutState(PCEvent t) {
    Event * ele = t.getHead();
    while(ele != NULL) {
        traceEvents("  %d:%d",ele->evtTimestamp,ele->proc->pid);
        ele = ele -> next;
    }
}

void simulation(PCEvent t, Scheduler *s) {

    bool CALL_SCHEDULER = false;
    Event * evt = t.getEvent();
    //Some variables
    int currentTime;
    int timeInPrevState;
    int totalEventTime; // currentTime + i/o burst or cpu burst
    Process * currentlyRunningProcess = NULL;
    int currentProcessCpuFinishTime;

    while(evt != NULL) {
        Process *proc = evt->proc;
        currentTime = evt->evtTimestamp;
        if(proc->processTimestamp == -1)  {
            // indicates process never ran
            timeInPrevState = currentTime - proc->arrivalTime;
        } else {
            timeInPrevState = currentTime - proc->processTimestamp;
        }

        switch(evt->newState) { // which state to transition to?
            case TRANS_TO_READY:
                // must come from BLOCKED or from PREEMPTION
                // must add to run queue
                trace("%d %d %d: %s -> %s\n",currentTime,proc->pid,timeInPrevState,getState(evt->oldState),getState(evt->newState));
                {
                    if(evt->oldState == TRANS_TO_BLOCK) {
                        proc -> dynamicPriority = proc -> staticPriority - 1;
                    }

                    /*

                    The ‘E’ scheduler is a bit tricky. When a process becomes READY from creation or unblocking it might preempt the currently
                    running process. Preemption in this case happens if the unblocking process’s dynamic priority is higher than the currently
                    running processes dynamic priority AND the currently running process does not have an event pending for the same time
                    stamp. The reason is that such an event can only be a BLOCK or a PREEMPT event. So we do not force a preemption at this
                    point as the pending event will be picked up before the scheduler is called.
                    If preemption does happen, you have to remove the future event for the currently running process and add a preemption event
                    for the current time stamp (ensure that the event is properly ordered in the eventQ).

                    */
                    if(s->test_preempt() && currentlyRunningProcess != NULL) {
                        //some tracing
                        //---> PRIO preemption 0 by 1 ? 1 TS=20 now=20) --> NO
                        //time the previous process should have run.
                        int timeShouldHaveRun = currentlyRunningProcess -> currentCpuBurst + currentlyRunningProcess -> processTimestamp;
                        trace("---> PRIO preemption %d by %d ? TS=%d now=%d) --> ",currentlyRunningProcess->pid,proc->pid,timeShouldHaveRun,currentTime);
                        if(currentlyRunningProcess -> dynamicPriority < proc -> dynamicPriority /*&& t.getNextEventTime() != currentTime*/) {
                            trace("YES\n");
                            
                            //Need to do few things.
                            //Remove the future event for the currently running process.
                            traceEvents("RemoveEvent(%d):",currentlyRunningProcess->pid);
                            displayEventQueueWithoutState(t);
                            traceEvents(" ==> ");
                            t.rmEvent(currentlyRunningProcess->pid);
                            displayEventQueue(t);
                            traceEvents("\n");

                            //Add a preempt event for the current time stamp.
                            //Now make a event
                            Event *evnt = (Event *)malloc(sizeof(Event));

                            //Need to reset a few things for the process that is preempted,
                            //cpuburst.
                            //currentlyRunningProcess -> processTimestamp = currentTime;
                            int cb = currentTime - currentlyRunningProcess -> processTimestamp;

                            if(currentlyRunningProcess -> preemptCpuBurst != -1) {
                                // If this is true that means process next stage was preemption so it would have got full quantum, add back the quantum and subtract the current cb
                                currentlyRunningProcess -> preemptCpuBurst = currentlyRunningProcess -> preemptCpuBurst + currentlyRunningProcess -> currentCpuBurst - cb;
                                currentlyRunningProcess -> remainingTime =  currentlyRunningProcess -> remainingTime   + currentlyRunningProcess -> currentCpuBurst - cb;
                            } else {
                                // this means the stage was going to be blocked and the next stage will not be a preemption.
                                currentlyRunningProcess -> preemptCpuBurst = currentlyRunningProcess -> currentCpuBurst - cb;
                                currentlyRunningProcess -> remainingTime =  currentlyRunningProcess -> remainingTime  + currentlyRunningProcess -> currentCpuBurst - cb;
                            }

                            evnt->evtTimestamp = currentTime;
                            evnt->newState = TRANS_TO_PREEMPT;                        
                            evnt->oldState = TRANS_TO_RUN;
                            evnt->proc = currentlyRunningProcess;
                            traceEvents("  AddEvent(%d:%d:%s):",currentTime,currentlyRunningProcess->pid,getState(evnt->newState));                    
                            displayEventQueue(t);
                            traceEvents(" ==> ");
                            t.putEvent(evnt);
                            displayEventQueue(t);
                            traceEvents("\n");
                            currentlyRunningProcess = NULL;
                        } else {
                            // Preemption is not needed.
                            trace("NO\n");
                        }
                    }

                    //Added the process to the run queue.
                    proc->processTimestamp = currentTime;
                    s->addProcess(proc);
                    CALL_SCHEDULER = true; // conditional on whether something is run
                    totalEventTime = currentTime;
                }                
                break;
            case TRANS_TO_RUN:
                {
                    proc->cpuWaitTime += timeInPrevState;
                    trace("%d %d %d: %s -> %s",currentTime,proc->pid,timeInPrevState,getState(evt->oldState),getState(evt->newState));
                    trace(" ");
                    
                    int cb;

                    //Here if process was preempted then never call myrandom function to get cpu burst.
                    if(proc->preemptCpuBurst != -1) {
                        cb = proc->preemptCpuBurst;
                    } else {
                        //Here calculate cpu burst.
                        cb = t.myrandom(proc->cpuBurst);
                    }

                    //Now we have the cpu burst check for time quantum first.

                    // If cpu burst is > then quantum and lesser than remaming time, 
                    // then cb should be = to quantum and a preempt event is necessary

                    int quantum = s->getQuantum();
                    bool preempt = false;
                    //Now check if cpuBurst is >= remamingCpu
                    if(cb < proc->remainingTime) {
                        trace("cb=%d rem=%d prio=%d\n",cb,proc->remainingTime,proc->dynamicPriority);
                        if(cb > quantum) {
                            proc -> preemptCpuBurst = cb - quantum;
                            cb = quantum; 
                            preempt = true;    
                        } else {
                            proc->preemptCpuBurst = -1;
                        }
                    } else {
                        cb = proc->remainingTime;
                        trace("cb=%d rem=%d prio=%d\n",cb,proc->remainingTime,proc->dynamicPriority);
                        if(cb > quantum) {
                            proc -> preemptCpuBurst = cb - quantum;
                            cb = quantum; 
                            preempt = true;      
                        } else {
                            proc->preemptCpuBurst = -1;
                        }
                    } 


                    totalEventTime = currentTime + cb;

                    //Now make a event
                    Event *evnt = (Event *)malloc(sizeof(Event));
                    evnt->evtTimestamp = totalEventTime;
                    if(preempt) {
                        evnt->newState = TRANS_TO_PREEMPT;                        
                    } else {
                        evnt->newState = TRANS_TO_BLOCK;
                    }
                    evnt->oldState = TRANS_TO_RUN;
                    proc->remainingTime = proc->remainingTime - cb;
                    proc->processTimestamp = currentTime;
                    proc -> currentCpuBurst =  cb; 
                    evnt->proc = proc;
                    traceEvents("  AddEvent(%d:%d:%s):",totalEventTime,proc->pid,getState(evnt->newState));                    
                    displayEventQueue(t);
                    traceEvents(" ==> ");
                    t.putEvent(evnt);
                    displayEventQueue(t);
                    traceEvents("\n");
                    currentProcessCpuFinishTime = totalEventTime;
                    
                }
                break;

            case TRANS_TO_BLOCK:
                {
                    // Note that termination takes priority,
                    // which means when the event remaming cpu time is zero, then terminate the event. Do not do io burst.
                    if(proc->remainingTime == 0) {
                        trace("%d %d %d: Done\n",currentTime,proc->pid,timeInPrevState);
                        proc->finishTime = currentTime;
                        proc->turnAroundTime = currentTime - proc->arrivalTime;
                        pMap.insert(pair<int,Process *>(proc->pid,proc));
                    } else {

                        trace("%d %d %d: %s -> %s",currentTime,proc->pid,timeInPrevState,getState(evt->oldState),getState(evt->newState));
                        trace(" ");

                        //Here calculate the IOburst.
                        int ib = t.myrandom(proc->ioBurst);
                        totalEventTime = currentTime + ib;

                        //Merge all the IO start and end time; to do that just save them in a vector.
                        concurrentIo.push_back(make_pair(currentTime,totalEventTime));

                        trace(" ib=%d rem=%d\n",ib,proc->remainingTime);

                        Event *evnt = (Event *)malloc(sizeof(Event));
                        evnt->evtTimestamp = totalEventTime;
                        evnt->newState = TRANS_TO_READY;
                        evnt->oldState = TRANS_TO_BLOCK;
                        proc->processTimestamp = currentTime;
                        proc->totalIoTime = proc->totalIoTime + ib;
                        evnt->proc = proc;

                        traceEvents("  AddEvent(%d:%d:%s):",totalEventTime,proc->pid,getState(TRANS_TO_READY));                    
                        displayEventQueue(t);
                        traceEvents(" ==> ");
                        t.putEvent(evnt);
                        displayEventQueue(t);
                        traceEvents("\n");                        
                    }
                    CALL_SCHEDULER = true;
                    //create an event for when process becomes READY again                    
                }
                
                break;
            case TRANS_TO_PREEMPT:
                {        
                    // add to runqueue (no event is generated)
                    trace("%d %d %d: %s -> %s",currentTime,proc->pid,timeInPrevState,getState(evt->oldState),getState(TRANS_TO_READY));
                    trace(" ");
                    trace(" cb=%d rem=%d prio=%d\n",proc->preemptCpuBurst,proc->remainingTime,proc->dynamicPriority);
                    proc->processTimestamp = currentTime;
                    proc->dynamicPriority -= 1;
                    s->addProcess(proc);
                    CALL_SCHEDULER = true; // conditional on whether something is run
                    totalEventTime = currentTime;
                }
                break;
        }
        // remove current event object from Memory
        delete evt; 
        evt = NULL;
        if(CALL_SCHEDULER) {
            if (t.getNextEventTime() == currentTime) {
                evt = t.getEvent();
                continue; //process next event from Event queue
            }
            CALL_SCHEDULER = false; // reset global flag
            if (currentlyRunningProcess == NULL) {
                currentlyRunningProcess = s->getNextProcess();
                if (currentlyRunningProcess == NULL) {
                    evt = t.getEvent();
                    continue;
                }
                    
                // create event to make this process runnable for same time.
                //Here we need to trace few things:
                //1. the event we are adding
                //2. the event queue before adding the event
                //3. the event queue after the adding the event

                Event *evnt = (Event *)malloc(sizeof(Event));
                evnt->evtTimestamp = currentTime;
                evnt->newState = TRANS_TO_RUN;
                evnt->oldState = TRANS_TO_READY;
                proc->processTimestamp = currentTime;
                evnt->proc = currentlyRunningProcess;
                traceEvents("  AddEvent(%d:%d:%s):",currentTime,currentlyRunningProcess->pid,getState(TRANS_TO_RUN));
                displayEventQueue(t);
                traceEvents(" ==> ");
                t.putEvent(evnt);
                displayEventQueue(t);
                traceEvents("\n");
            }
        } 

        //Now get the event in the last.
        evt = t.getEvent();
        if(evt -> newState != TRANS_TO_RUN && currentProcessCpuFinishTime <= evt -> evtTimestamp) {
                currentlyRunningProcess = NULL;
        }
    } 
}



int main(int argc, char* argv[]) {

    int c;
    char * scheduler;
    char schd = 'F';
    int arg = 1;
    int quantum;
    int numprio = 4;

    while ((c = getopt(argc,argv,"vtes:")) != -1 )
    {
        switch(c) {
            case 'v': 
                {   
                    dotrace = 1;
                    arg++;
                }
                break;
            case 's': 
                {
                    char * sarg = strtok(optarg,":");
                    schd = sarg[0];
                    ++sarg;           
                    if(sarg[0] != '\0') {
                        quantum = atoi(sarg);
                    } else {
                        if(schd == 'R' || schd == 'E' || schd == 'P') {
                            cout<<"Invalid scheduler param <"<<schd<<">"<<endl;
                        } else {
                            quantum = 10000;
                        }
                    }
                    sarg = strtok(NULL,":");
                    if(sarg != NULL) {
                         numprio = atoi(sarg);
                    }
                    arg++;
                }
                break;
            case 't':
                schdTrace = 1;
                arg++;
                break;

            case 'e':
                eventTrace = 1;
                arg++;
                break;
        }
    }

    if(argc > arg + 1) {
        //Now first initialize the so that events list cann be made.
        PCEvent t;
        char * inputFile = argv[arg++];
        char * rfile = argv[arg++];
        t.initializeFiles(inputFile,rfile,numprio);

        //Show the event Queue.
        traceEvents("ShowEventQ:");
        displayEventQueueWithoutState(t);
        traceEvents("\n");

        //Do the simulation now? Always check the scheduler and then start the simulation.

        switch (schd)
        {
        case 'F':
            {
                Scheduler *s = new FFIO(quantum,schdTrace);
                simulation(t,s);
                cout<<"FCFS"<<endl;
            }
            break;
        case 'L':
            {
                Scheduler *s = new LCFS(quantum,schdTrace);
                simulation(t,s);
                cout<<"LCFS"<<endl;
            }
            break;

        case 'S':
            {                
                Scheduler *s = new SRTF(quantum,schdTrace);
                simulation(t,s);
                cout<<"SRTF"<<endl;
            }
            break;

        case 'R':
            {
                Scheduler *s = new RR(quantum,schdTrace);
                simulation(t,s);
                cout<<"RR "<<quantum<<endl;
            }
            break;

        case 'P':
            {
                Scheduler *s = new PRIO(quantum,numprio,schdTrace);
                simulation(t,s);
                cout<<"PRIO "<<quantum<<endl;
            }
            break;

        case 'E':
            {
                Scheduler *s = new PREPRIO(quantum,numprio,schdTrace);
                simulation(t,s);
                cout<<"PREPRIO "<<quantum<<endl;
            }
            break;
            
        default:
            cout<<"Unknown Scheduler:"<<schd<<endl;
            exit(1);
            break;
        }

        //These variable are only for printing the sum;
        int totalSimulationTime=0;
        int totalCpuTime=0;
        int totalIoBusy = 0;
        int totalTurnaroundTime = 0;
        int totalCpuWaitTime = 0;
        int totalNoProcess=0;

        //Now print the final values
        map<int, Process *>::iterator it;
        for(it = pMap.begin(); it != pMap.end(); it++) {
            Process * p = it->second;
            if(totalSimulationTime < p->finishTime) {
                totalSimulationTime = p->finishTime;
            } 
            totalCpuTime += p->totalCpuTime;
            totalNoProcess++;
            totalTurnaroundTime += p->turnAroundTime;
            totalCpuWaitTime += p->cpuWaitTime;
            printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",p->pid,p->arrivalTime,p->totalCpuTime,p->cpuBurst,p->ioBurst,p->staticPriority,p->finishTime,p->turnAroundTime,p->totalIoTime,p->cpuWaitTime);
        }

        //Here we know that total simulation time. Now create an array and zero out all the intervals.
        int ioUtilization[totalSimulationTime];
        fill_n(ioUtilization, totalSimulationTime, 1);
        for(int i=0; i < concurrentIo.size(); i++) {
            for(int j = concurrentIo[i].first; j < concurrentIo[i].second; j++) {
                ioUtilization[j] = 0;
            }
        }

        //Now count the total number of zeros;
        for (int i = 0; i < totalSimulationTime; i++) {
            if(ioUtilization[i]==0) {
                totalIoBusy++;
            }
        }
        

        double cpuUtil = 100.0 * (totalCpuTime / (double) totalSimulationTime);
        double ioUtil = 100.0 * (totalIoBusy / (double) totalSimulationTime);
        double avgTurnAroundTime = ((double) totalTurnaroundTime / totalNoProcess);
        double avgWaitTime =  ((double) totalCpuWaitTime / totalNoProcess);
        double throughput = 100.0 * (totalNoProcess / ((double) totalSimulationTime));

        printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",totalSimulationTime,cpuUtil,ioUtil,avgTurnAroundTime,avgWaitTime,throughput);

    } else if(argc == arg + 1) {
        cout<<"Please give a valid rfile name:"<<endl;
    } else {
        cout<<"Please give a valid input file name:"<<endl;
    }

}