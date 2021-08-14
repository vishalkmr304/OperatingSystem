/*
 * Created on Tue July 13th 2021
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

#define MAX_FRAMES 128
#define MAX_VPAGES 64
#define MAX_PROCESS 10

vector<int> randVector;
int totalRandNo;

typedef struct {
    int start_vpage;
    int end_vpage;
    int write_protected;
    int file_mapped;
} vma_s;

typedef struct { 
    unsigned present              :1; // This bit tells us that whether the page is in memory or not.
    unsigned write_protected      :1; 
    unsigned referenced           :1; //This page is recently being referenced 
    unsigned modified             :1; //This page is modified
    unsigned pagedout             :1;
    unsigned file_mapped          :1;
    unsigned frame                :7; // This is to keep the record of 128 frames. This is predefined.

    //For software use
    unsigned valid                :1;
    unsigned is_valid_set         :1;
} pte_t;

class Process {

    public:

        int pid;
        vector<vma_s *> virtual_memory_addr;
        pte_t page_table[MAX_VPAGES];

        Process(int processId) {
            pid = processId;
        }
        //Will have to see how to access/reference page table
};

typedef struct { 
    int v_page;
    Process * proc;
    int frame_id;
    
    //For specific pagers
    uint32_t aging;
    unsigned long time_last_used;

 } frame_t;

frame_t frame_table[MAX_FRAMES];

//Some global variables
size_t len = 0;

class ProcessList {
    
    private:
        queue<frame_t *> free_list;
        vector<Process *> pList;
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

        //Initialize the file and return the file pointer to read the instructions.
        FILE * initializeFile(char * inputFile) {
            fp = fopen(inputFile,"r");
            if(fp == NULL) {
                cout<<"Not a valid inputfile"<<" "<<"<"<<inputFile<<">"<<endl;
                exit(EXIT_FAILURE);
            }

            //Maybe we just read properly.
            //First ignore all the comments in the begining.
            char * line = readComments(fp);
            int no_of_process = atoi(line);

            for(int i=0; i < no_of_process;  i++) {
                Process *proc = new Process(i);
                // Get the next uncommented line
                int no_of_vmas = atoi(readComments(fp));
                for(int j=0; j < no_of_vmas; j++) {
                    char * s_p = strtok(readComments(fp)," \n\t");
                    if(s_p != NULL) {
                        int start_vpage = atoi(s_p);
                        int end_vpage = atoi(strtok(NULL," "));
                        int write_protected = atoi(strtok(NULL," "));
                        int file_mapped = atoi(strtok(NULL," "));
                        memset(proc->page_table, 0, MAX_VPAGES * sizeof(pte_t));
                        vma_s *vma = (vma_s *)malloc(sizeof(vma_s));
                        vma->start_vpage = start_vpage;
                        vma->end_vpage = end_vpage;
                        vma->write_protected = write_protected;
                        vma->file_mapped = file_mapped;
                        proc->virtual_memory_addr.push_back(vma);
                    }
                }
                pList.push_back(proc);
            }
            //This will return the file pointer so that we can read the instructions in simulation
            return fp;
        }
        

        void initializeRFile(char * randomFile) {
            FILE * fpRandom = fopen(randomFile,"r");
            if(fpRandom == NULL) {
                cout<<"Not a valid randomFile"<<" "<<"<"<<randomFile<<">"<<endl;
                exit(EXIT_FAILURE);
            }
            //Read the random file first.
            //The first line is the total random numbbers in the file.
            getline(&line, &len, fpRandom);
            totalRandNo = atoi(line);
            while((linelen = getline(&line, &len, fpRandom)) != -1) {
                randVector.push_back(atoi(line));
            }
        }

        bool getNextInstruction(char &operation, int &vpage) {
            char * op  = readComments(fp);
            if(op != NULL) {
                char * oper = strtok(op," ");
                operation = oper[0];
                vpage = atoi(strtok(NULL," "));
                return true;
            } else {
                // Here this means the file is finished and we are done just return false to break the while loop
                return false;
            }
            
        }

        Process * getProcessById(int pid) {
            for(int i=0; i< pList.size(); i++) {
                Process * p = pList.at(i);
                if(p->pid == pid) {
                    return p;
                } 
            }
            return NULL;
        }

        //Initialize the queue with numframe.
        void initializeFreeList(int numframe) {
            for(int i=0; i < numframe; i++) {
                frame_t * f = &frame_table[i]; // it points to frame table address
                f->frame_id = i;
                free_list.push(f);
            }
        }

        //Return the frame from free_list.
        frame_t * alloateFromFreeList() {
            if(!free_list.empty()) {
                frame_t * f = free_list.front();
                free_list.pop();
                return f;
            } else {
                return NULL;
            }
        }

        //This is to when process exits we need to return the frame to free list
        void addToFreeList(int frameNo) {
            frame_t * f = &frame_table[frameNo];
            f->frame_id = frameNo;
            f->proc = NULL;
            f->v_page = -1; //This indicates that none of the v_page is mapped.
            free_list.push(f); 
        }


        //This is to return the pList vector to simluation so that it can print the page table of each process
        vector<Process *> getPList() {
            return pList;
        }
};
