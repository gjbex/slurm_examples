# Slurm_examples

Examples of slurm use cases

## What is it?

1. `spread_processes.slurm`: running a non-hybrid MPI application (Intel MPI) with
   processes spread out to maximize memory bandwidth.
1. `spread_processes_alt.slurm`: running a non-hybrid MPI application (Intel MPI) with
   processes spread out to maximize memory bandwidth (alternative approach).
1. `mpmd.slurm`: running multiple applications in parallel in the same job (Multiple
   Process, Multiple Data).

## Requirements

These slurm scripts use `mem_limit` and `mem_limit_no_mpi` to report affinity of
processes and threads.  This applications can be built from the [MemoryLimits](https://github.com/gjbex/MemoryLimits) project.
