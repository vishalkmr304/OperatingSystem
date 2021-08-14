Name: Vishal Kumar
NetId: vk2161

Files: 
extern_variables.h : It contains the global variables which are used multiple files. 
Scheduler.h : All the code related to the Scheduler is done here. Inheritance is used for modularity. 
IO.h : Code related to the taking the input and creating IO objects are here.
Simulation.cpp : Crux of the code, does all the simulation and keeps tracks of all the stats for each IO request.
Makefile : Just a small script to ease the burden for compiling and cleaning any object files.

Run Instructions?
Clean any previous object file named with iosched by, "make clean"
Then run "make"
Then run the test cases against "iosched" object files.