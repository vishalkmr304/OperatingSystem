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
#include "tokenizer.h"
#include <map>
#include <math.h>
#include <iomanip>
#include <vector>
#include <algorithm>

using namespace std;

const string MUTIPLE_TIMES = "Error: This variable is multiple times defined; first value used";
const string ILLEGAL_OPCODE = "Error: Illegal opcode; treated as 9999";
const string ILLEGAL_IMMEDIATE_VALUE = "Error: Illegal immediate value; treated as 9999";
const string ILLEGAL_RELATIVE_ADDRESS = "Error: Relative address exceeds module size; zero used";
const string ILLEGAL_ABSOLUTE_ADDRESS = "Error: Absolute address exceeds machine size; zero used";
const string EXCEED_USELIST = "Error: External address exceeds length of uselist; treated as immediate";

void NOT_DEFINED(string sym) { printf("Error: %s is not defined; zero used\n",sym.c_str()); }
void WARN_USE_LIST(int m, string sym) { printf("Warning: Module %d: %s appeared in the uselist but was not actually used\n",m,sym.c_str()); }

const static char* errstr[] = {
        "NUM_EXPECTED",                 // Number expect, anything >= 2^30 is not a number either 
        "SYM_EXPECTED",                 // Symbol Expected
        "ADDR_EXPECTED",                // Addressing Expected which is A/E/I/R
        "SYM_TOO_LONG",                 // Symbol Name is too long
        "TOO_MANY_DEF_IN_MODULE",       // > 16
        "TOO_MANY_USE_IN_MODULE",       // > 16
        "TOO_MANY_INSTR",               // total num_instr exceeds memory size (512)
};

struct Symtab {
    int moduleAddr;
    int relativeAddr;
    int times;
};

map<string,Symtab> symbolTable;

void __parseerror(Tokenizer t,int errcode) {
    printf("Parse Error line %d offset %d: %s\n", t.getLineNo(), t.getOffset(), errstr[errcode]); 
    exit(EXIT_FAILURE);
}

char * readSymbol(Tokenizer t) {
    char * token = t.getToken();
    if(token == NULL) {
        __parseerror(t,1);
    }
    char * sym_token = token;
    int sym_length = 0;
    char c = *sym_token;
    if(!isalpha(c) || token == NULL) {
        __parseerror(t,1);
    }
    for(char c = *sym_token; c; c=*++sym_token) {
        sym_length++;
        if(!isalnum(c)) {
            __parseerror(t,1);
        }
        if(sym_length > 16) {
            __parseerror(t,3);
        }
    }
    return token;
}

int readInt(Tokenizer t) {
    char * token = t.getToken();
    if(token == NULL) {
        __parseerror(t,0);
    }
    char * copy_token = token;
    for(char c = *copy_token; c; c=*++copy_token) {
        if(!isdigit(c)) {
            __parseerror(t,0);
        }
    }

    //Check if the number is out of range.
    long long num = 0;
    string tk = string(token);
    int tokenlen = tk.length();
    int power = 0;
    for(int i=tokenlen-1; i >= 0 ; i--) {
        num = num + pow(10,power) *  (tk[i] - '0');
        if(num >= 1073741824) {
            __parseerror(t,0);
        }
        power++;
    }
    return (int)num;

}

int readInt(Tokenizer t,char * token) {
    char * copy_token = token;
    for(char c = *copy_token; c; c=*++copy_token) {
        if(!isdigit(c)) {
            __parseerror(t,0);
        }
    }

    //Check if the number is out of range.
    long long num = 0;
    string tk = string(token);
    int tokenlen = tk.length();
    int power = 0;
    for(int i=tokenlen-1; i >= 0 ; i--) {
        num = num + pow(10,power) *  (tk[i] - '0');
        if(num >= 1073741824) {
            __parseerror(t,0);
        }
        power++;
    }
    return (int)num;
}

char readIAER(Tokenizer t) {
    char * token = t.getToken();
        if(token == NULL) {
        __parseerror(t,2);
    }
    char iaer_token = *token;
    if ((token == NULL) || !(iaer_token == 'I' || iaer_token == 'A' || iaer_token == 'E' || iaer_token == 'R')) {
        __parseerror(t,2);
    }
    return iaer_token;
}

void createSymbol(string sym, int val, int moduleaddr) {
    map<string,Symtab>::iterator it = symbolTable.find(sym);
    if (it != symbolTable.end()) {
        Symtab stable = symbolTable.at(sym);
        stable.times = stable.times + 1;
        symbolTable.erase(sym);
        symbolTable.insert(pair<string,Symtab>(sym,stable));
    } else {
        Symtab stable;
        stable.moduleAddr = moduleaddr;
        stable.relativeAddr = val;
        stable.times = 0;
        symbolTable.insert(pair<string,Symtab>(sym,stable));
    }
}

void __pass1(Tokenizer t) {
    int moduleaddr = 0;
    char * token = t.getToken();
    int moduleNo = 0;
    vector<string> orderderedSymbols;
    while(token != NULL) {

        //deflist ordered
        vector<string> orderdeflist;

        //Store the deflist before saving it to the symbol table to check for the warning.
        map<string,int> defmap;

        //Module No
        moduleNo++;

        // Read defcount from token and do all the checks
        // Create another readInt method which accepts a token
        int defcount = readInt(t,token);
        if(defcount > 16) {
            __parseerror(t,4);
        }
        for(int i=0; i < defcount; i++) {
            char * sym = readSymbol(t);
            int val = readInt(t);
            defmap.insert(pair<string,int>(string(sym),val));

            //Store only if you didn't find it.
            if(std::find(orderderedSymbols.begin(),orderderedSymbols.end(),string(sym)) == orderderedSymbols.end()) {
                orderderedSymbols.push_back(sym);
            }
            
            orderdeflist.push_back(sym);
        }
        int usecount = readInt(t);
        if(usecount > 16) {
            __parseerror(t,5);    
        }
        for(int i = 0; i< usecount; i++) {
            char * sym = readSymbol(t);
        }

        int instcount = readInt(t);
        moduleaddr = moduleaddr + instcount;
        if(moduleaddr > 512) {
            __parseerror(t,6);
        }
        for (int i = 0; i<instcount; i++) {
            char addressmode = readIAER(t);
            int operand = readInt(t);

        }

        for(int i = 0; i < orderdeflist.size(); i++) {
            string s = orderdeflist[i];

            // Find in symbol table first if it does not occur in symbol table then it means we are seeing it for the first time
            // If it is in symbol table then we know that it has come in symbol table and we need to take it from there.
            string sym;
            int val;
            map<string,Symtab>::iterator syit = symbolTable.find(s);
            if(syit != symbolTable.end()) {
                sym = syit->first;
                val = syit->second.relativeAddr;
            } else {
                map<string,int>::iterator it = defmap.find(s);
                sym = it->first;
                val  = it->second;
            }

            if(val > instcount-1) {
                printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n",moduleNo,sym.c_str(),val,instcount-1);
                createSymbol(sym,0,moduleaddr-instcount);
            } else {
                createSymbol(sym,val,moduleaddr-instcount);
            }
        }
        token = t.getToken();
    }

    cout<<"Symbol Table"<<endl;
    for(int i = 0; i < orderderedSymbols.size(); i++) {
        string sym = orderderedSymbols[i];
        map<string,Symtab>::iterator it = symbolTable.find(sym);
        Symtab stable = it->second;
        int address = stable.moduleAddr + stable.relativeAddr;
        cout<<it->first<<"="<<address;
        if(stable.times>0) {
            cout<<" "<<MUTIPLE_TIMES;
        }
        cout<<endl;
    }
}

map<string,int> createSymbolMap(map<string,Symtab> symTab) {
    map<string,int> symbols;
    for(map<string, Symtab>::iterator it = symTab.begin(); it != symTab.end(); it++) {
        symbols.insert(pair<string,int>(it->first,0));
    }
    return symbols;
}


void __pass2(Tokenizer t) {
    int e = 0;
    int moduleaddr = 0;
    char * token = t.getToken();
    int instructionscount = 0;
    int moduleNo = 0;
    map<string,int> symbols = createSymbolMap(symbolTable);
    map<string,int> moduleMap;
    vector<string> orderderedSymbols;
    cout<<"Memory Map"<<endl;
    while(token != NULL) {
        //Store the deflist before saving it to the symbol table to check for the warning.

        //Module No
        moduleNo++;

        //Read defcount from token and do all the checks
        // Create another readInt method which accepts a token
        int defcount = readInt(t,token);
        
        for(int i=0; i < defcount; i++) {
            char * sym = readSymbol(t);
            int val = readInt(t);

            //Here mark the module for the symbols
            moduleMap.insert(pair<string,int>(sym,moduleNo));
            
            //Save symbols in vector to maintain the order, but we do have to be cautious that once the symbol is in we dont want to insert again.
            if(std::find(orderderedSymbols.begin(),orderderedSymbols.end(),string(sym)) == orderderedSymbols.end()) {
                orderderedSymbols.push_back(sym);
            }
        }

        int usecount = readInt(t);
        string uselist[usecount+1];
        for(int i = 0; i< usecount; i++) {
            char * sym = readSymbol(t);
            //Store these in the array because we need to know whether they will be used in the instructions or not.
            //Also, here we might as well see which all use list are not defined, if they aren't definied then maybe print a warning?
            uselist[i] = string(sym);
        }

        int instcount = readInt(t);
        int uselistcount[usecount];
        for (int i = 0; i<instcount; i++) {
            char addressmode = readIAER(t);
            int operand = readInt(t);
            //Here all the magic will happen.

            if(operand/1000 >= 10 && addressmode != 'I') {
                cout<<std::setw(3)<<std::setfill('0')<<instructionscount<<": "<<"9999"<<" "<<ILLEGAL_OPCODE<<endl;
                instructionscount++;
            } else if(operand >= 10000 && addressmode == 'I') {
                cout<<std::setw(3)<<std::setfill('0')<<instructionscount<<": "<<"9999"<<" "<<ILLEGAL_IMMEDIATE_VALUE<<endl;
                instructionscount++;
            } else if(addressmode == 'R') {
                int relative = operand % 1000;
                if(relative > instcount - 1) {
                    cout<<std::setw(3)<<std::setfill('0')<<instructionscount<<": "<<std::setw(4)<<std::setfill('0')<<operand - relative + moduleaddr<<" "<<ILLEGAL_RELATIVE_ADDRESS<<endl;
                    instructionscount++;
                }
                else {
                    cout<<std::setw(3)<<std::setfill('0')<<instructionscount<<": "<<std::setw(4)<<std::setfill('0')<<operand + moduleaddr<<endl;
                    instructionscount++;
                }
            } else if(addressmode == 'I') {
                cout<<std::setw(3)<<std::setfill('0')<<instructionscount<<": "<<std::setw(4)<<std::setfill('0')<<operand<<endl;
                instructionscount++;
            } else if(addressmode == 'A') {
                int relative = operand % 1000;
                if(relative >= 512) {
                    cout<<std::setw(3)<<std::setfill('0')<<instructionscount<<": "<<std::setw(4)<<std::setfill('0')<<operand - relative <<" "<<ILLEGAL_ABSOLUTE_ADDRESS<<endl;
                    instructionscount++;
                } else {
                    cout<<std::setw(3)<<std::setfill('0')<<instructionscount<<": "<<std::setw(4)<<std::setfill('0')<<operand<<endl;
                    instructionscount++;
                }
            } else if(addressmode == 'E') {
                int op = operand % 1000;
                if(usecount<op+1) {
                    cout<<std::setw(3)<<std::setfill('0')<<instructionscount<<": "<<std::setw(4)<<std::setfill('0')<<operand<<" "<<EXCEED_USELIST<<endl;
                    instructionscount++;    
                } else {
                    uselistcount[op] = 1;
                    string sym = uselist[op];
                    map<string,Symtab>::iterator it = symbolTable.find(sym);
                    if (it != symbolTable.end()) {
                        int absaddr = it->second.moduleAddr + it->second.relativeAddr;
                        cout<<std::setw(3)<<std::setfill('0')<<instructionscount<<": "<<std::setw(4)<<std::setfill('0')<<operand - op + absaddr<<endl;
                        instructionscount++;    

                        //mark the symbols which are used from symbol table.
                        map<string,int>::iterator i = symbols.find(sym);
                        if(i != symbols.end()) {
                            symbols.erase(sym);
                            symbols.insert(pair<string,int>(sym,1));
                        }

                    } else {
                        cout<<std::setw(3)<<std::setfill('0')<<instructionscount<<": "<<std::setw(4)<<std::setfill('0')<<operand - op<<" ";NOT_DEFINED(sym);
                        instructionscount++;
                    }

                }

            }
        }

        for(int i=0; i<usecount; i++) {
            if(uselistcount[i] != 1) {
                WARN_USE_LIST(moduleNo,uselist[i]);
            }
        }
        moduleaddr = moduleaddr + instcount;
        token = t.getToken();
    }

    //Between memory map and warning there's a newline
    cout<<endl;

    for(int i = 0; i < orderderedSymbols.size(); i++) {
        string sym = orderderedSymbols[i];
        map<string,int>::iterator ite = symbols.find(sym);
        if(ite->second == 0) {
            map<string,int>::iterator it2 = moduleMap.find(ite->first);
            printf("Warning: Module %d: %s was defined but never used\n",it2->second,it2->first.c_str());
        }
    }

}


int main(int argc, char* argv[]) {
    if(argc > 1) {
        Tokenizer t (argv[1]);
        
        //Pass 1 will print the symbol table as well.
        __pass1(t);

        //Newline between symbol table and memory map
        cout<<endl;
        
        //Now time for pass two
        //First reset the file pointer the best way is to close the file. and set the file pointer to be null.
        t.closeFile();

        //Pass 2 will print the memory map
        __pass2(t);

        //After pass 2 there's a newline
        cout<<endl;

    } else {
        cout<<"Expected argument after options"<<endl;
    }

    return 0;
}