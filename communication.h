#pragma once
#include "mpi_manager.h"

using namespace std;

/**
 * MPI communication tags
 */
#define APP_MPI_TAG_COMMUNICATION 1 // Used for status communication
#define APP_MPI_TAG_DATA_PREPARE 2 // Used for buffer preallocation of communication recipient
#define APP_MPI_TAG_DATA_DATA 4 // Used for sent data


/**
 * Defines relevant for application communication channel
 */
#define APP_COMMUNICATION_RUNNING 1 // The "running" signal sent by GUI

/**
 * Defines for data sending/recieving channel
 */
#define APP_MPI_STATE_DATA_IDLE 0 //send/recieve data is in idle mode
#define APP_MPI_STATE_DATA_PREPARE 1 //send/recieve data syncs buffer size
#define APP_MPI_STATE_DATA_DATA 2 // send/recieve data is sending/recieving data

