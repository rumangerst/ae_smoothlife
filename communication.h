#pragma once
#include "mpi_manager.h"

using namespace std;

/**
 * MPI communication tags
 */
#define APP_MPI_TAG_COMMUNICATION 1 // Used for status communication
#define APP_MPI_TAG_SPACE 2 // Complete space
#define APP_MPI_TAG_BORDER_RANGE 100 //Begin of border tag


/**
 * Defines relevant for application communication channel
 */
#define APP_COMMUNICATION_RUNNING 1 // The "running" signal sent by GUI


