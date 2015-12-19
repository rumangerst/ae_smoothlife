#pragma once
#include <mpi/mpi.h>

/**
 * @brief RAII implementation of MPI
 * @author Ruman
 */
class mpi_manager
{
public:

    /**
     * @brief This MPI instance has no role
     */
    static uint ROLE_NONE = 0;
    /**
     * @brief This MPI instance is a simulator, sends data to the MASTER simulator
     */
    static uint ROLE_SIMULATOR_SLAVE = 1;
    /**
     * @brief This MPI instance is a simulator, collects all calculated data from the SLAVE simulators
     */
    static uint ROLE_SIMULATOR_MASTER = 2;
    /**
     * @brief This MPI instance gets obtains the data from the MASTER simulator
     */
    static uint ROLE_COLLECTOR = 3;


    mpi_manager(int argc, char** argv)
    {
        if (MPI_SUCCESS != MPI_Init(&argc, &argv ))
            throw std::runtime_error("called MPI_Init twice");
    }

    ~mpi_manager()
    {
        MPI_Finalize();
    }

    /**
     * @brief Returns the MPI comm rank of this MPI instance
     * @author Ruman
     * @return
     */
    int rank()
    {
        int r;
        MPI_Comm_rank(MPI_COMM_WORLD, &r);
        return r;
    }

    /**
     * @brief Returns the role of the given MPI rank
     * @param mpi_comm_rank
     * @author Ruman
     * @return
     */
    uint role_of(int mpi_comm_rank)
    {
        if(mpi_comm_rank == 0)
            return ROLE_COLLECTOR;
        else if(mpi_comm_rank == 1)
            return ROLE_SIMULATOR_MASTER;
        else
            return ROLE_SIMULATOR_SLAVE;
    }

    /**
     * @brief Returns the comm rank with given role
     * @param role
     * @return the comm rank >= 0, -1 if there are more than 1 theoretically possible ranks
     */
    int rank_with_role(uint role)
    {
        if(role == ROLE_COLLECTOR)
            return 0;
        else if(role == ROLE_SIMULATOR_MASTER)
            return 1;
        else
            return -1;
    }
};

