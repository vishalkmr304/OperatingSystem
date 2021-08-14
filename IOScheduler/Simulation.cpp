/*
 * Created on Tue July 13th 2021
 *
 * Copyright (c) 2021 - Vishal Kumar (vk2161)
 */

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include "Scheduler.h"

using namespace std;

int vtrace = 0;
#define traceV(fmt...)      do { if (vtrace == 1) { printf(fmt); fflush(stdout); } } while(0)

int ftrace = 0;
#define traceF(fmt...)      do { if (ftrace == 1) { printf(fmt); fflush(stdout); } } while(0)


map<int,io_req*> io_stat;


int main(int argc, char* argv[]) {

    int c;
    int arg = 1;
    
    Scheduler * sched;

    while ((c = getopt(argc,argv,"s:vfq")) != -1 )
    {
        switch(c) {
            case 'v': 
                vtrace = 1;
                arg++;
                break;

            case 'f': 
                ftrace = 1;
                arg++;
                break;

            case 'q': 
                qtrace = 1;
                arg++;
                break;

            case 's': 
                {
                    if(optarg != NULL) {
                        //ALGOS="i(FIFO) j(SSTF) s(LOOK) c(CLOOK) f(FLOOK)"
                        switch (optarg[0])
                        {
                            case 'i':
                                sched = new FFIO(0); 
                                //cout<<"Pager FFIO is set"<<endl;
                                break;
                        
                            case 'j':
                                sched = new SSTF(0); // Here 0 represents offset, note that offset start with 0.                            
                                break;

                            case 's':
                                sched = new LOOK(0);
                                break;

                            case 'c':
                                sched = new CLOOK(0);
                                break;

                            case 'f':
                                sched = new FLOOK(0);
                                break;
                                
                            default:
                                cout<<"Unknown Replacement Algorithm: <"<<optarg<<">"<<endl;
                                exit(1);
                                break;
                        }  
                    } else {
                        cout<<"Unknown Replacement Algorithm: <->"<<endl;
                        exit(1);
                    }
                    
                    arg++;
                }
                break;

            default:
                cout<<"illegal option"<<endl;
                exit(1);
                break;
        }
    }

    /* Come back here */
    if(sched == NULL) {
        // If no algorithm is defined then it is by default aging.
        sched = new FFIO(0);
    }

    if(argc >= arg + 1) {
        //Now first initialize the so that events list cann be made.
        IOReq io;
        char * inputFile = argv[arg++];
        io.initializeFile(inputFile);

        //Let's see whether we have done it correctly or not, first.
        //Be careful with this because i am maintaining queue for some reason, maybe memory concerns ?
        //io.prettyPrint();

        //Do the simulation now? We already have the scheduler details. Let's do the simulation.
        //Pass the file pointer cause its important.

    int curr_time = 0;
    int next_io_finish_time = -1; //indicates no io scheduled
    bool simulation = true;
    int tot_movement = 0;
    int dir = 0; //The direction the seek will go to.
    io_req * curr_running_io = NULL;
    int max_waittime = 0;
    traceV("TRACE\n");
    while(simulation) {
        //Well i just can't take the next instruction like that need to do something. Wait we will get it but only pop it when the time is equal
        // to curr time.
        io_req * req = io.getNextInstruction();
        //cout<<"I am here, req:"<<req->time_issued<<endl;
        if(req != NULL && req->time_issued == curr_time) {
            //Add to IO-queue
            
            traceV("%d: %5d add %d\n",curr_time,req->inst_no,req->track_no);
            sched->addIORequest(req);
            //Then pop from request queue.
            io.removeFromRequestQ();
        }

        if(curr_running_io != NULL && next_io_finish_time == curr_time) {
            //Compute relevant info and store in IO request for final summary
            traceV("%d: %5d finish %d\n",curr_time,curr_running_io->inst_no,curr_time-curr_running_io->time_issued);
            curr_running_io->end_time = curr_time;
            //symbolTable.insert(pair<string,Symtab>(sym,stable));
            io_stat.insert(pair<int, io_req*>(curr_running_io->inst_no, curr_running_io));
            curr_running_io = NULL;
        } 

        if(curr_running_io == NULL) {
            // Check the scheduler queue
            //cout<<"I ask request"<<endl;
            io_req * schd_req = sched->getIORequest();
            if(schd_req != NULL) {
                //cout<<"I got request"<<endl;
                schd_req->start_time = curr_time;
                traceV("%d: %5d issue %d %d\n",curr_time,schd_req->inst_no,schd_req->track_no,curr_seek_pos);
                if(curr_seek_pos > schd_req->track_no) {
                    dir = -1;
                    next_io_finish_time = curr_time + curr_seek_pos - schd_req->track_no;
                } else if (curr_seek_pos < schd_req -> track_no) {
                    dir = 1;
                    next_io_finish_time = curr_time + schd_req->track_no - curr_seek_pos;
                } else {
                    next_io_finish_time = curr_time + curr_seek_pos - schd_req->track_no;
                    
                    //There is a strong reason why we are doing this. when i am on 45th sector and my next request is to read from 
                    //45th sector then i am going to do that, without any seeking or increasing my current time, and this makes total sense.

                    curr_running_io = schd_req;
                    continue;
                }
                curr_running_io = schd_req;
            } else {
                if(req == NULL) {
                    simulation = false;
                }
            }
            // If it is not empty then schedule it
                    // set the activeIO to be true and next_io_finish_time to be something, calculate this carefully.
            // otherwise finish the simulation.
        }

        if(curr_running_io != NULL) {
            //Simulate the seek
            curr_seek_pos = curr_seek_pos + dir;
            tot_movement++;
        }

        //Increment time by 1
        curr_time ++;
    }

    map<int, io_req*>::iterator it;

    double total_wait_time = 0;
    double total_turnaround_time = 0;
    int no_of_operations = 0;
    for(it = io_stat.begin(); it != io_stat.end(); it++) {
        io_req * req = it->second;
        printf("%5d: %5d %5d %5d\n",req->inst_no, req->time_issued, req->start_time, req->end_time);

        int curr_wait_time = req->start_time - req->time_issued;
        int curr_turn_around_time = req->end_time - req->time_issued;

        if(max_waittime < curr_wait_time) {
            max_waittime = curr_wait_time;
        }

        total_wait_time += curr_wait_time;
        total_turnaround_time += curr_turn_around_time;
        no_of_operations++;
    }

    double avg_turnaround = (double) (total_turnaround_time) / no_of_operations;
    double avg_waittime = (double) (total_wait_time) / no_of_operations;
    printf("SUM: %d %d %.2lf %.2lf %d\n", curr_time-1, tot_movement, avg_turnaround, avg_waittime, max_waittime);

    } else {
        cout<<"Please give a valid input file name:"<<endl;
    }

}