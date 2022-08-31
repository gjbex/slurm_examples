#!/usr/bin/env bash
#SBATCH -A lpt2_sysadmin
#SBATCH --nodes=1 --cpus-per-task=1 
#SBATCH hetjob
#SBATCH --nodes=1 --ntasks=12 --cpus-per-task=6
#SBATCH --time=00:15:00

module unuse /apps/leuven/skylake/2018a/modules/all
module use /apps/leuven/icelake/2021a/modules/all
module load foss

srun --exclusive --nodes=1 --ntasks=1 --cpus-per-task=1 \
     mem_limit/mem_limit_no_mpi -m 1gb -i 200mb -s 1s > output_0.txt &

export OMP_PROC_BIND=true
export OMP_PLACES=cores

for (( computation_id=1; computation_id <= $SLURM_NTASKS_HET_GROUP_1; computation_id++ ))
do
    srun --exclusive --het-group=1 \
        --ntasks=3 --nodes=1 --cpus-per-task=2 \
        --threads-per-core=1 \
            mpirun -n 3 mem_limit/mem_limit -t 2 -m 1gb -i 200mb -s 1s > "output_${computation_id}.txt" &
done

wait
