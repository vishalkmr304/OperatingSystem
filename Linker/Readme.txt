Name: Vishal Kumar
NetID: vk2161

tokenizer.h:  Tokenizer.h has functionality to getToken from a file and it keeps track of offset and line.
linker.cpp: The whole code is defined in this file. In the main function pass1 and pass2 functions are clearly defined. 
Makefile: running make will compile the code and create an object file with the name "linker" and load the gcc module 9.2.

How to run it?
Just enter make clean in the flder where all the files given above are present. (This is to ensure that there aren't any stale file with that name.)
Run ./linker <input file name> and it will print the output directly to coonsole. 