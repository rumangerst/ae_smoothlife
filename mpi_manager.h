#pragma once
#include <mpi.h>

/**
 * @brief RAII implementation of MPI
 * @author Ruman
 */
class mpi_manager
{
public:
  
  enum mpi_role
  {
    /**
     * @brief This MPI instance has no role
     */
    NONE = 0,
    
    /**
     * @brief This MPI instance gets obtains the data from the MASTER simulator
     */
    USER_INTERFACE = 1,
    
    /**
     * @brief This MPI instance is a simulator, collects all calculated data from the SLAVE simulators
     */
    SIMULATOR_MASTER = 2,
    
    /**
     * @brief This MPI instance is a simulator, sends data to the MASTER simulator
     */
    SIMULATOR_SLAVE = 3 
    
  };

    mpi_manager(int argc, char** argv)
    {
        if (MPI_SUCCESS != MPI_Init(&argc, &argv ))
            throw std::runtime_error("called MPI_Init twice");
        
        check_role();
    }

    ~mpi_manager()
    {
        MPI_Finalize();
    }
    
    /**
     * @brief checks if this rank is consistent with its associated role
     */
    void check_role()
    {
        if(role() == mpi_role::USER_INTERFACE)
        {
            if(!APP_GUI)
            {                
                throw std::runtime_error("rank is known as GUI, but executable is not compiled as GUI!");
            }
        }
        else
        {
            if(!APP_SIM)
            {                
                throw std::runtime_error("rank is known as simulator, but executable is not compiled as simulator!");
            }
        }
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
    
    mpi_role role()
    {
      return role_of(rank());
    }

    /**
     * @brief Returns the role of the given MPI rank
     * @param mpi_comm_rank
     * @author Ruman
     * @return
     */
    mpi_role role_of(int mpi_comm_rank)
    {
        if(mpi_comm_rank == 0)
            return mpi_role::USER_INTERFACE;
        else if(mpi_comm_rank == 1)
            return mpi_role::SIMULATOR_MASTER;
        else
            return mpi_role::SIMULATOR_SLAVE;
    }

    /**
     * @brief Returns the comm rank with given role
     * @param role
     * @return the comm rank >= 0, -1 if there are more than 1 theoretically possible ranks
     */
    int rank_with_role(mpi_role role)
    {
        if(role == mpi_role::USER_INTERFACE)
            return 0;
        else if(role == mpi_role::SIMULATOR_SLAVE)
            return 1;
        else
            return -1;
    }
};

