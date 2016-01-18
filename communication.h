#pragma once
#include <mpi.h>
#include <iostream>
#include <vector>

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
#define APP_COMMUNICATION_REINITIALIZE 2 //Sent by master for reinitialization command

enum mpi_role
{
    /**
     * @brief This MPI instance has no role
     */
    NONE = 0,
    
    /**
     * @brief This MPI instance is a simulator, collects all calculated data from the SLAVE simulators. Is also the GUI.
     */
    SIMULATOR_MASTER = 1,

    /**
     * @brief This MPI instance is a simulator, sends data to the MASTER simulator
     */
    SIMULATOR_SLAVE = 2

};

/**
 * C++ version of MPI_TEST with simplified function call
 * @param request
 * @return true if operation was completed
 */
inline bool mpi_test(MPI_Request * request)
{
    MPI_Status status;
    int flag;

    MPI_Test(request, &flag, &status);

    return flag == 1;
}

/**
 * @brief Cancel MPI_Request if mpi_test returns false
 * @param request
 */
inline void mpi_cancel_if_needed(MPI_Request * request)
{
    if(!mpi_test(request))
        MPI_Cancel(request);
}

/**
 * @brief Returns the MPI comm rank of this MPI instance
 * @author Ruman
 * @return
 */
inline int mpi_rank()
{
    int r;
    MPI_Comm_rank(MPI_COMM_WORLD, &r);
    return r;
}

/**
 * @brief Returns the size of MPI_COMM_WORLD
 * @return 
 */
inline int mpi_comm_size()
{
    int s;
    MPI_Comm_size(MPI_COMM_WORLD, &s);
    return s;
}

/**
 * @brief Returns the role of the given MPI rank
 * @param mpi_comm_rank
 * @author Ruman
 * @return
 */
inline mpi_role mpi_get_role_of(int mpi_comm_rank)
{
    if (mpi_comm_rank == 0)        
        return mpi_role::SIMULATOR_MASTER;
    else
        return mpi_role::SIMULATOR_SLAVE;
}

inline mpi_role mpi_get_role()
{
    return mpi_get_role_of(mpi_rank());
}

inline vector<int> mpi_get_ranks_with_role(mpi_role role)
{
    if(role == mpi_role::SIMULATOR_MASTER)
        return vector<int> { 0 };
    else
    {
        vector<int> d;
        
        for(int i = 1; i < mpi_comm_size(); ++i)
        {
            d.push_back(i);
        }
        
        return d;
    }
}

inline int mpi_get_slave_count()
{
    return mpi_comm_size() - 1;
}

/**
 * @brief checks if this rank is consistent with its associated role
 * @throws std::runtime_error if role check fails
 */
inline void mpi_check_role()
{
    if (mpi_role() == mpi_role::SIMULATOR_MASTER)
    {
        if (!APP_GUI)
        {
            throw std::runtime_error("rank is known as GUI, but executable is not compiled as GUI!");
        }
    }
}

/**
 * @brief RAII implementation of MPI
 * @author Ruman
 */
class mpi_manager
{
public:

    mpi_manager(int argc, char** argv)
    {
        if (MPI_SUCCESS != MPI_Init(&argc, &argv))
            throw std::runtime_error("called MPI_Init twice");

        mpi_check_role();
    }

    ~mpi_manager()
    {
        MPI_Finalize();
    }
};


