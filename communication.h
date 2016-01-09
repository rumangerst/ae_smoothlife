#pragma once
#include "mpi_manager.h"

using namespace std;

#define APP_MPI_TAG_COMMUNICATION 1 // Used for status communication
#define APP_MPI_TAG_DATA_DATA 2 // Used for sent data
#define APP_MPI_TAG_DATA_PREPARE 3 // Used for buffer preallocation of communication recipient

#define APP_COMMUNICATION_RUNNING 1 // The "running" signal sent by GUI