#!/usr/bin/env bash
#SBATCH -A lpt2_sysadmin
#SBATCH --cluster=wice
#SBATCH --nodes=2 --ntasks=72 --cpus-per-task=2
#SBATCH --time=00:15:00

module unuse /apps/leuven/skylake/2018a/modules/all
module use /apps/leuven/icelake/2021a/modules/all
module load foss

export OMP_PROC_BIND=close

mpirun -x OMP_PROC_BIND=$OMP_PROC_BIND --cpus-per-proc 2 --bind-to core mem_limit/mem_limit -t 2 -m 2gb -i 100mb -s 500ms > multinode_output.txt
