export I_MPI_CXX=/home/xentrics/intel/bin/icpc
export MPICH_CXX=/home/xentrics/intel/bin/icpc
export I_MPI_CC=/home/xentrics/intel/bin/icc
export MPICH_CC=/home/xentrics/intel/bin/icc
export LD_LIBRARY_PATH=/home/xentrics/intel/compilers_and_libraries/linux/lib/intel64

rm -rf ./build
mkdir ./build
cd ./build
CC=/home/xentrics/intel/compilers_and_libraries/linux/mpi/bin64/mpicc CXX=/home/xentrics/intel/compilers_and_libraries/linux/mpi/bin64/mpicxx cmake -DBUILD_INTEL=ON -DGUI_SDL=ON ..
make
cd ..