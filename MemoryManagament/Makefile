SHELL:=/bin/bash

mmu: loadmodule Simulation.cpp
	g++ Simulation.cpp -o mmu

loadmodule: 
	module load gcc-9.2

clean:
	rm -f mmu *~