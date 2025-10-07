#!/bin/bash
arg1=$1
arg2=$2
arg3=$3
usecase=$4

sudo yum -y install gcc-c++
sudo yum -y install unzip cmake
unzip -n Optimistic-Replication.zip 
cd Optimistic-Replication/wellcoordination/benchmark/
g++ ${usecase}-benchmark.cpp -o $usecase-benchmark-out
cd ../
mkdir workload
cd workload/
mkdir "${arg1}-${arg2}-${arg3}"
cd "${arg1}-${arg2}-${arg3}"
mkdir $usecase
cd ../../
cd benchmark/
./${usecase}-benchmark-out "$arg1" "$arg2" "$arg3"
cd 
cd Optimistic-Replication/tcp

sh compile.sh 
cd build/

sudo sysctl -w net.core.rmem_max=16777216
sudo sysctl -w net.core.rmem_default=16777216

sudo sysctl -w net.core.wmem_max=16777216
sudo sysctl -w net.core.wmem_default=16777216