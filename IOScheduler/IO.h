/*
 * Created on Tue July 28th 2021
 *
 * Copyright (c) 2021 - Vishal Kumar (vk2161)
 */

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <getopt.h>
#include <queue>
#include <stdint.h>

using namespace std;


typedef struct {
    int inst_no;
    int time_issued;
    int track_no;
    int start_time;
    int end_time;
} io_req;

//Some global variables
size_t len = 0;

class IOReq {
    
    private:
        // This is forcefully made private because we do not want to make changes in the vector other than this class
        // We will provide a helper fuction which just reads a element from and returns it.
        queue<io_req *> ioReqQ;
        char * line;
        int linelen;
        FILE * fp;

        //We need a function which can take a file pointer and keep ignoring the lines if they start with '#' and returns the first line
        // which do not start with '#'.
        char * readComments(FILE * fp) {
            while((linelen = getline(&line, &len, fp)) != -1) {
                if(line[0] == '#') {
                    continue;
                } else {
                    return line;
                }
            }
            return NULL;
        }

    public:

        //Initialize the file
        void initializeFile(char * inputFile) {
            int inst_count = 0;
            fp = fopen(inputFile,"r");
            if(fp == NULL) {
                cout<<"Not a valid inputfile"<<" "<<"<"<<inputFile<<">"<<endl;
                exit(EXIT_FAILURE);
            }

            // Here readComments will return me if it has reached the end of file and i do not have any valid request to consume.
            char * req = readComments(fp);
            while(req != NULL) {
                int t_issued = atoi(strtok(req," "));
                int t_no = atoi(strtok(NULL," "));
                if(t_issued < 0 || t_no < 0) {
                    printf("Instruction %d is incorrect please have a look. Time Issued:%d Track No:%d\n",inst_count,t_issued,t_no);
                }
                io_req * ioReq = (io_req *)malloc(sizeof(io_req));
                ioReq->time_issued = t_issued;
                ioReq->track_no = t_no;
                ioReq->inst_no = inst_count;
                ioReq->start_time = -1; //Indicates never started
                ioReq->end_time = -1; //Indicates never ended
                ioReqQ.push(ioReq);
                req = readComments(fp);
                inst_count ++;
            }
        }
        io_req * getNextInstruction() {
            if(!ioReqQ.empty()) {
                return ioReqQ.front();
            } else {
                //There are no istructions left, just return NULL.
                return NULL;
            }           
        }

        void removeFromRequestQ() {
            ioReqQ.pop();
        }
};
