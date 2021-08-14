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
#include "Pager.h"

using namespace std;

int otrace = 0;
#define traceO(fmt...)      do { if (otrace == 1) { printf(fmt); fflush(stdout); } } while(0)

int ptrace = 0;
#define traceP(fmt...)      do { if (ptrace == 1) { printf(fmt); fflush(stdout); } } while(0)

int ftrace = 0;
#define traceF(fmt...)      do { if (ftrace == 1) { printf(fmt); fflush(stdout); } } while(0)

int strace = 0; 
#define traceS(fmt...)      do { if (strace == 1) { printf(fmt); fflush(stdout); } } while(0)


#define COST_MAP 300
#define COST_UNMAP 400
#define COST_IN 3100
#define COST_OUT 2700 
#define COST_FIN 2800
#define COST_FOUT 2400
#define COST_ZERO 140
#define COST_SEGV 340
#define COST_SEGPROT 420
#define COST_CTX_SWITCH 130
#define COST_PROCESS_EXIT 1250
#define COST_RW 1

unsigned long ctx_switches = 0;
unsigned long process_exits = 0;
unsigned long long total_cost = 0;


struct p_stat{
    unsigned long unmaps;
    unsigned long maps;
    unsigned long ins;
    unsigned long outs;
    unsigned long fins;
    unsigned long fouts;
    unsigned long zeros;
    unsigned long segv;
    unsigned long segprot;
};

p_stat process_stats[MAX_PROCESS];

bool checkAccess(Process * p, int vpage) {

    //on first page fault we need to do the few things, first search throught the entire vma of the process and set file_mapped and write_protected.
    //It is very important do this here. Also, think of some clever way to achieve the mapping of holes in the vmas.
    if(!p->page_table[vpage].is_valid_set) {
        //If we have never set the valid bit for that process, then this is the time.
        vector<vma_s *> vec = p->virtual_memory_addr;
        for(int i = 0; i < vec.size(); i++ ) {
            int start_vpage = p->virtual_memory_addr.at(i)->start_vpage;
            int end_vpage = p->virtual_memory_addr.at(i)->end_vpage;
            int file_mapped = p->virtual_memory_addr.at(i)->file_mapped;
            int write_protected = p->virtual_memory_addr.at(i)->write_protected;
            for(int j = start_vpage; j <= end_vpage; j++ ) {
                p->page_table[j].valid = 1;
                p->page_table[j].file_mapped = file_mapped;
                p->page_table[j].write_protected = write_protected;
            }
        }

        for(int i = 0; i < MAX_VPAGES; i++ ) {
            p->page_table[i].is_valid_set = 1;
        }
    }

    
    if(p->page_table[vpage].valid) {
        return true;
    } else {
        return false;
    }
    
}

//1.somefunction
//2.reset_pte
//3.update_pte
void reset_pte(pte_t *pte) {
    pte->referenced = 0;
    pte->modified = 0;
    pte->present=0; 
}

void update_pte(pte_t *pte, char operation) {
    pte->present = 1;
    pte->referenced = 1;
    if(operation == 'w' && !pte->write_protected) {
        pte->modified = 1;
    }
}

void pageIn(pte_t *pte, Process * p) {

    if(pte->file_mapped) {
        traceO(" FIN\n");
        //For stats
        process_stats[p->pid].fins += 1;
    } else if(pte->pagedout) {
        traceO(" IN\n");
        //For stats
        process_stats[p->pid].ins += 1;        
    } else {
        traceO(" ZERO\n");
        //For stats
        process_stats[p->pid].zeros += 1;        
    }
}



frame_t * getFrame(Pager * pager, ProcessList * p) {
    frame_t *frame = p->alloateFromFreeList();
    if (frame == NULL) {
        frame = pager->select_victim_frame();
    }
    return frame;
}

int main(int argc, char* argv[]) {

    int c;
    char pgr = 'A'; //by deafault the algorithm is aging.
    int arg = 1;
    int quantum;
    int numframe = 4;
    Pager * pager;

    //initilize pstat with zeros
    memset(process_stats, 0, MAX_PROCESS * sizeof(p_stat));

    while ((c = getopt(argc,argv,"f:a:o:")) != -1 )
    {
        switch(c) {
            case 'f': 
                {   
                    //Here check the that frame should be given, if they have used the 'f' and still do not provide a number error out and exit.
                    if(optarg != NULL) {
                        numframe = atoi(optarg);
                        if(numframe < 0) {
                            cout<<"Really funny .. you need at least one frame"<<endl;
                            exit(1);
                        } else if(numframe > 128) {
                            cout<<"sorry max frames supported = 128"<<endl;
                            exit(1);
                        }
                    } else {
                        cout<<"Really funny .. you need at least one frame"<<endl;
                        exit(1);
                    }
                    arg++;
                }
                break;
            case 'a': 
                {
                    if(optarg != NULL) {
                        //ALGOS="f r c e a w"
                        switch (optarg[0])
                        {
                            case 'f':
                                pager = new FFIO(numframe,0); // Here 0 represents the hand. In FIFO the first member that goes out of the pool is hand
                                //cout<<"Pager FFIO is set"<<endl;
                                break;
                        
                            case 'r':
                                pager = new Random(numframe,0); // Here 0 represents offset, note that offset start with 0.                            
                                break;

                            case 'c':
                                pager = new Clock(numframe,0);
                                break;

                            case 'e':
                                pager = new NRU(numframe,0);
                                break;

                            case 'a':
                                pager = new Aging(numframe,0);
                                break;
                                
                            case 'w':
                                pager = new WS(numframe,0); //working set
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
            case 'o':
                if(optarg != NULL) {
                    for (char* c = optarg; c[0] != '\0' ; c=++optarg) {
                        switch (c[0])
                        {
                            case 'O':
                                otrace = 1;
                                break;
                            
                            case 'P':
                                ptrace = 1;
                                break;

                            case 'F':    
                                ftrace = 1;
                                break;
                            
                            case 'S':
                                strace = 1;
                                break;

                            case 'a':
                                aselect = 1;
                                break;
                                
                            default:
                                cout<<"Unknown output option: <"<<c<<">"<<endl;
                                exit(1);
                                break;
                        }
                    }

                } else {
                    cout<<"Unknown output option:<>"<<endl;
                }

                arg++;
                break;

            default:
                cout<<"illegal option"<<endl;
                exit(1);
                break;
        }
    }

    if(pager == NULL) {
        // If no algorithm is defined then it is by default aging.
        pager = new Aging(numframe,0);
    }

    if(argc > arg + 1) {
        //Now first initialize the so that events list cann be made.
        ProcessList p;
        char * inputFile = argv[arg++];
        char * rfile = argv[arg++];
        //get the file pointer from the initilize file;
        p.initializeRFile(rfile);
        FILE * fp = p.initializeFile(inputFile);

        //Once the file initilization is finished just initilize the free_list.
        p.initializeFreeList(numframe);

        //Do the simulation now? We already have the pager details. Let's do the simulation.
        // Pass the file pointer cause its important.

        char operation;
        int vpage;
        
        Process * currentProcess;

        while (p.getNextInstruction(operation, vpage)) {

            traceO("%d: ==> %c %d\n",inst_count,operation,vpage);
            inst_count++;
            if(operation == 'c') {
                //Here vpage will be a proess number. Get that process objet from the vector and assign its pointer to the
                //currentProcess.
                currentProcess = p.getProcessById(vpage);
                ctx_switches++;
            } else if(operation == 'e') {
                /* On process exit (instruction), you have to traverse the active process’s page table starting from 
                    0..63 and for each valid entry UNMAP the page and FOUT modified filemapped pages. Note that dirty 
                    non-fmapped (anonymous) pages are not written back (OUT) as the process exits. The used frame has 
                    to be returned to the free pool and made available to the get_frame() function again. The frames 
                    then should be used again in the order they were released. */

                // we are avoiding the above because there is not need to search for it, we just have to exit the currently running process, no matter what they send in the vpage entry
                
                printf("EXIT current process %d\n",currentProcess->pid);
                process_exits++;
                for(int i=0 ; i < MAX_VPAGES; i++) {
                    //Check if the pte is referenced.
                    if(currentProcess->page_table[i].present) {
                        
                        traceO(" UNMAP %d:%d\n",currentProcess->pid,i);  
                        
                        //For stats
                        process_stats[currentProcess->pid].unmaps += 1;

                        if(currentProcess->page_table[i].modified) {
                            if(currentProcess->page_table[i].file_mapped) {
                                    traceO(" FOUT\n");

                                    //For stats
                                    process_stats[currentProcess->pid].fouts += 1;

                            }
                        }
                        //Now add to free list
                        p.addToFreeList(currentProcess->page_table[i].frame);

                        //And free the pagetable of the process
                        reset_pte(&currentProcess->page_table[i]);
                    }

                    if(currentProcess->page_table[i].valid) {
                        currentProcess->page_table[i].pagedout = 0;
                    }
                    
                }
                
                // Here it is very important that you do the currently running process null, because when a process exits at this moment there is no currently running process.
                currentProcess = NULL;

            } else {
                pte_t *pte = &currentProcess->page_table[vpage];
                //cout<<"Present Bit:"<<pte->present<<endl;
                if(!pte->present) {
                    // If the page is not present in the memory then get it in the memory. This is page fault.
                    // Send to pagefault handler, and check whether it is part of vmas or not, if not not output SEGV.  
                    if(checkAccess(currentProcess,vpage)) {
                        frame_t * newframe = getFrame(pager,&p);
                        Process * mappedProcess = newframe->proc;
                        //If frame is mapped to some process 
                        //cout<<"This is my mapped proess:"<<mappedProcess<<endl;
                        if(mappedProcess != NULL) {
                            // If page is mapped check where the mapped page is modified 
                            // Do the unmap
                            int mapped_vpage = newframe->v_page;
                            traceO(" UNMAP %d:%d\n",mappedProcess->pid,mapped_vpage);                            

                            //For stats
                            process_stats[mappedProcess->pid].unmaps += 1;

                            if(mappedProcess->page_table[mapped_vpage].modified) {
                                if(mappedProcess->page_table[mapped_vpage].file_mapped) {
                                    traceO(" FOUT\n");
                                    //For stats
                                    process_stats[mappedProcess->pid].fouts += 1;                                    
                                } else {
                                    traceO(" OUT\n");
                                    //For stats
                                    process_stats[mappedProcess->pid].outs += 1;                                    
                                    mappedProcess->page_table[mapped_vpage].pagedout = 1;
                                }                                
                            }
                            //Here reset the pagetable entries, then set at the last
                            reset_pte(&mappedProcess->page_table[mapped_vpage]);                            
                        } 
                        pageIn(pte,currentProcess);
                        traceO(" MAP %d\n",newframe->frame_id);
                        //Here reset the aging to 0
                        pager->reset_age(newframe);

                        //For stats
                        process_stats[currentProcess->pid].maps += 1;                                    

                        currentProcess->page_table[vpage].frame = newframe->frame_id;
                        newframe->proc = currentProcess;
                        newframe->v_page = vpage;
                    } else {
                        traceO(" SEGV\n");
                        //For stats
                        process_stats[currentProcess->pid].segv += 1;                        
                        continue;
                    }
                }
                //Check write protection first.
                if(operation == 'w' && pte->write_protected) {
                    traceO(" SEGPROT\n");
                    //For stats
                    process_stats[currentProcess->pid].segprot += 1;                    
                }

                update_pte(pte,operation);
            }
        }
        //Now the simulation is finished let print out the other stuff

        //First print the page table
        vector<Process *> pList = p.getPList();
        for(int i=0; i < pList.size(); i++) {
            Process * p = pList.at(i);
            traceP("PT[%d]: ",p->pid);
            for(int j=0; j < MAX_VPAGES; j++) {
                if(p->page_table[j].valid) {
                    if(p->page_table[j].present) {
                        char referenced = '-';
                        char modified = '-';
                        char swappedout = '-';
                        if(p->page_table[j].referenced) {
                            referenced = 'R';
                        }
                        if(p->page_table[j].modified) {
                            modified = 'M';
                        }
                        if(p->page_table[j].pagedout) {
                            swappedout = 'S';
                        }
                        traceP("%d:%c%c%c ",j,referenced,modified,swappedout);
                    } else if(p->page_table[j].pagedout){
                        traceP("# ");
                    } else {
                        traceP("* ");
                    }
                } else {
                    traceP("* ");
                }
            }
            traceP("\n");
        }

        //Then we print the frame table
        traceF("FT: ");
        for(int i = 0; i < numframe; i++) {
            Process * p = frame_table[i].proc;
            if(p != NULL) {
                traceF("%d:%d ",p->pid,frame_table[i].v_page);
            } else {
                traceF("* ");
            }
        }
        traceF("\n");

        for(int i=0; i < pList.size(); i++) {
           Process * p = pList.at(i);
           int pid = p->pid;
           traceS("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                     pid,
                     process_stats[pid].unmaps, process_stats[pid].maps, process_stats[pid].ins, process_stats[pid].outs,
                     process_stats[pid].fins, process_stats[pid].fouts, process_stats[pid].zeros,
                     process_stats[pid].segv, process_stats[pid].segprot);
                     total_cost += process_stats[pid].unmaps * COST_UNMAP;
                     total_cost += process_stats[pid].maps * COST_MAP;
                     total_cost += process_stats[pid].ins * COST_IN;
                     total_cost += process_stats[pid].outs * COST_OUT;
                     total_cost += process_stats[pid].fins * COST_FIN;
                     total_cost += process_stats[pid].fouts * COST_FOUT;
                     total_cost += process_stats[pid].zeros * COST_ZERO;
                     total_cost += process_stats[pid].segv * COST_SEGV;
                     total_cost += process_stats[pid].segprot * COST_SEGPROT;
        }
        total_cost += ctx_switches * COST_CTX_SWITCH;
        total_cost += process_exits * COST_PROCESS_EXIT;
        total_cost += (inst_count - ctx_switches - process_exits) * COST_RW;
        //Now is the time for total cost
        /* 
            printf("TOTALCOST %lu %lu %lu %llu %lu\n",
            inst_count, ctx_switches, process_exits, cost, sizeof(pte_t));
        */
       traceS("TOTALCOST %lu %lu %lu %llu %lu\n",inst_count, ctx_switches, process_exits, total_cost, sizeof(pte_t));

    } else if(argc == arg + 1) {
        cout<<"Please give a valid rfile name:"<<endl;
    } else {
        cout<<"Please give a valid input file name:"<<endl;
    }

}