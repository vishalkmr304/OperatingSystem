SHELL:=/bin/bash

scheduler: loadmodule simulation.cpp
	g++ simulation.cpp -o scheduler

loadmodule: 
	module load gcc-9.2

clean:
	rm -f scheduler *~