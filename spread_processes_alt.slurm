#!/usr/bin/env bash
#SBATCH -A lpt2_sysadmin
#SBATCH --nodes=1 --ntasks=16 --cpus-per-task=4
#SBATCH --time=00:05:00

module --force purge
module use /apps/leuven/icelake/2021a/modules/all
module load intel

export I_MPI_PIN=1
export I_MPI_JOB_RESPECT_PROCESS_PLACEMENT=disable

mpirun -np 4  mem_limit/mem_limit -m 3gb -i 500mb -s 2s -l 5s
