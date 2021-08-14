SHELL:=/bin/bash

mmu: loadmodule Simulation.cpp
	g++ Simulation.cpp -o iosched

loadmodule: 
	module load gcc-9.2

clean:
	rm -f iosched *~