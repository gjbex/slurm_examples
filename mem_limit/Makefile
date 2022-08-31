CXX = g++
MPICXX = mpic++
CXXFLAGS = -O0 -g -Wall -std=c++14 -fopenmp

all: mem_limit mem_limit_no_mpi

mem_limit: mem_limit.o
	$(MPICXX) $(CXXFLAGS) -o $@ $^

mem_limit.o: mem_limit.cc
	$(MPICXX) $(CXXFLAGS) -c -o $@ $^

mem_limit_no_mpi: mem_limit_no_mpi.o
	$(CXX) $(CXXFLAGS) -DNO_MPI -o $@ $^

mem_limit_no_mpi.o: mem_limit.cc
	$(CXX) $(CXXFLAGS) -DNO_MPI -c -o $@ $^

clean:
	$(RM) $(wildcard *.o) $(wildcard core.*) mem_limit mem_limit_no_mpi
