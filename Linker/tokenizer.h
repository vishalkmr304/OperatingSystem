/*
 * Created on Tue June 8th 2021
 *
 * Copyright (c) 2021 - Vishal Kumar (vk2161)
 */

#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

int lineno=0;
int offset=0; 
int prev_line_length;
string delimimters = " \t\n";
size_t len = 0;

class Tokenizer {
        
        char * fileName;
        FILE * fp = NULL;
        char * line = NULL;
        char *tok = NULL;
        int linelen;
        
    public:
        Tokenizer(char * fName) {
            fileName = fName;
        }

        int getOffset() {
            return offset;
        }

        int getLineNo() {
            return lineno;
        }

        void closeFile() {
            fp = NULL;
        }

        char * getToken() {
            fp = getFilePointer(fp);
            if(tok != NULL) {
                tok = strtok (NULL, delimimters.c_str());
                while(tok == NULL && ((linelen = getline(&line, &len, fp)) != -1)) {
                    lineno = lineno + 1;
                    tok = strtok(line,delimimters.c_str());
                    prev_line_length = linelen;
                }
            } else {
                while(tok == NULL && ((linelen = getline(&line, &len, fp)) != -1)) {
                    lineno = lineno + 1;
                    tok = strtok(line,delimimters.c_str());
                    prev_line_length = linelen;
                }
            }
            if(tok != NULL) {
                offset = tok - line + 1;
            } else {
                offset = prev_line_length;
            }
            return tok;
        }

    private:
        FILE * getFilePointer(FILE * fp) {
            if (fp == NULL) {
                fp = fopen(fileName,"r");
                if(fp == NULL) {
                    cout<<"Not a valid inputfile"<<" "<<"<"<<fileName<<">"<<endl;
                    exit(EXIT_FAILURE);
                }
                return fp;
            } else {
                return fp;
            }
        }
};