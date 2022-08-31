#!/usr/bin/env bash
#SBATCH -A lpt2_sysadmin
#SBATCH --nodes=1 --ntasks=1 --cpus-per-task=16
#SBATCH --time=00:05:00

module --force purge
module use /apps/leuven/icelake/2021a/modules/all
module load foss

srun --exclusive --cpus-per-task=4 mem_limit/mem_limit_no_mpi -t 4 -m 3gb -i 500mb -s 20ms > task-1.txt &
srun --exclusive --cpus-per-task=4 mem_limit/mem_limit_no_mpi -t 4 -m 2gb -i 200mb -s 200ms > task-2.txt &
srun --exclusive --cpus-per-task=8 mem_limit/mem_limit_no_mpi -t 8 -m 1gb -i 100mb -s 50ms > task-3.txt &

wait
