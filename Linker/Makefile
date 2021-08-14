SHELL:=/bin/bash

linker: loadmodule linker.cpp
	g++ linker.cpp -o linker

loadmodule: 
	module load gcc-9.2

clean:
	rm -f linker *~