#!/bin/bash

#SBATCH --nodes=3
#SBATCH --ntasks=3
#SBATCH --cpus-per-task=6
#SBATCH --output="result.log"
#SBATCH --mem=64G
#SBATCH -p cpu # This is the default partition, you can use any of the following; intel, batch, highmem, gpu
##SBATCH --exclude=ac057,ac058,ac059 # Add the nodes you want to exclude here

nodes=($( scontrol show hostnames $SLURM_NODELIST ))
nnodes=${#nodes[@]}
last=$(( $nnodes - 1 ))

OPREP_HOME="/scratch/user/u.js213354/Optimistic-Replication/tcp/build"
RESULT_LOC="/scratch/user/u.js213354/Optimistic-Replication/wellcoordination/workload/"
RESULTS_DIR_NAME="AE_results"

NUM_NODES=3
NUM_OPS=12000
WRITE_PERC=5
REP=1 # number of reps
USECASE="crdt"
FAILURE=0 # 0 for no failure, 1 for follower and 2 for leader failure
THROUGHPUT=0
OperationRate=1000000000
Node_List="166 145 155";

hostlist=""
for i in $( seq 0 $last ); do
    ip=$(ping -c 1 ${nodes[$i]} | grep 'PING' | awk -F'[()]' '{print $2}')
    hostlist+="$ip "
done
echo $hostlist

for n in $( seq  $NUM_NODES $NUM_NODES ); do
        for p in $WRITE_PERC; do
                BENCH_DIRECTORY=$RESULT_LOC$n-$NUM_OPS-$p/$USECASE;
                echo $BENCH_DIRECTORY;
                if [ ! -d "$BENCH_DIRECTORY" ]; then
                        mkdir -p $BENCH_DIRECTORY;
                        mkdir -p $BENCH_DIRECTORY/$RESULTS_DIR_NAME;
                        /scratch/user/u.js213354/Optimistic-Replication/wellcoordination/benchmark/$USECASE-benchmark.out $n $NUM_OPS $p
                        echo "benchmark generated";

                fi
        done
done

NUM_NODES_TMP=$(($NUM_NODES-1))

for i in $( seq 0 $NUM_NODES_TMP ); do
    #printf "ssh ${nodes[$i]}";
    ssh ${nodes[$i]} "cd ${OPREP_HOME};"
    #sleep 1;
done

sleep 6

j=0
for i in $( seq 0 $NUM_NODES_TMP ); do
    j=$(($j+1))
    printf "ssh ${nodes[$i]} 'cd ${OPREP_HOME}; ./main-replication-protocols $j $NUM_NODES $NUM_OPS $WRITE_PERC $USECASE $OperationRate $hostlist > $RESULT_LOC/$j.log'\n"
    ssh ${nodes[$i]} "cd ${OPREP_HOME}; ./main-replication-protocols $j $NUM_NODES $NUM_OPS $WRITE_PERC $USECASE $OperationRate $hostlist > $RESULT_LOC$n-$NUM_OPS-$p/$USECASE/$RESULTS_DIR_NAME/$i.log"&
    sleep 1;
done
sleep 200;
sleep 2;

sleep 500000
