Name: Vishal Kumar
NetId: vk2161

Files: 
extern_variables.h : It contains the global variables whih are used by multiple files. 
Pager.h : All the code related to the Pager is done here. Inheritance is used for modularity. 
Process.h : Code related to the taking the input and creating process objects are here. Note frame_table is defined here globally.
Simulation.cpp : Crux of the code, does all the simulation and keeps tracks of all the stats for each process.
Makefile : Just a small sript to ease the burden for compiling and cleaning any objcet files.

Run Instructions?
Clean any previous object file named with mmu by, "make clean"
Then run "make"
Then run the test cases against "mmu" object files.