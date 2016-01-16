#pragma once

#include <vector>
#include "communication.h"

using namespace std;

template <typename T>
/**
 * @brief Contains an MPI connection between sender and reciever where this rank can be a sender, reciever or both. Send and recieve buffer are the same type and size.
 */
class mpi_dual_connection
{
public:

    mpi_dual_connection(int _other_rank, bool _is_sender, bool _is_reciever, int _tag, MPI_Datatype _datatype, aligned_vector<T> initial) :
    other_rank(_other_rank),
    mpi_tag(_tag),
    datatype(_datatype),
    buffer_send(initial),
    buffer_recieve(initial),
    is_sender(_is_sender),
    is_reciever(_is_reciever)

    {
        if (!(is_sender | is_reciever) || _other_rank >= mpi_comm_size() || _other_rank < 0)
        {
            cerr << "Cannot initialize mpi_dual_connection with invalid sender and reciever" << endl;
            exit(EXIT_FAILURE);
        }
        if (initial.size() == 0)
        {
            cerr << "Cannot initialize mpi_dual_connection with empty buffer!" << endl;
            exit(EXIT_FAILURE);
        }
    }

    mpi_dual_connection(int _other_rank, bool _is_sender, bool _is_reciever, int _tag, int _buffer_size, MPI_Datatype _datatype) :
    mpi_dual_connection(_other_rank, _is_sender, _is_reciever, _tag, _datatype, aligned_vector<T>(_buffer_size))
 { }

    ~mpi_dual_connection() { }

    /**
     * @brief Returns the buffer if state is IDLE if sender and state is IDLE
     */
    aligned_vector<T> * get_buffer_send()
    {
        return &buffer_send;
    }

    aligned_vector<T> * get_buffer_recieve()
    {
        return &buffer_recieve;
    }

    /**
     * @brief Sends the data to reciever if sender; Accept new connection if reciever
     */
    void flush()
    {
        if (is_sender && is_reciever)
        {
            MPI_Sendrecv(buffer_send.data(),
                         buffer_send.size(),
                         datatype,
                         other_rank,
                         mpi_tag,
                         buffer_recieve.data(),
                         buffer_recieve.size(),
                         datatype,
                         other_rank,
                         mpi_tag,
                         MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
        }
        else if (is_sender)
        {
            MPI_Send(buffer_send.data(),
                     buffer_send.size(),
                     datatype,
                     other_rank,
                     mpi_tag,
                     MPI_COMM_WORLD);
        }
        else if (is_reciever)
        {
            MPI_Recv(buffer_recieve.data(),
                     buffer_recieve.size(),
                     datatype,
                     other_rank,
                     mpi_tag,
                     MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
        }
        else
        {
            cerr << "mpi_dual_connection that is no sender and no reciever ?!" << endl;
            exit(EXIT_FAILURE);
        }
    }

    int get_other_rank()
    {
        return other_rank;
    }

private:

    const bool is_sender;
    const bool is_reciever;

    const int other_rank;
    const int mpi_tag;

    const MPI_Datatype datatype; //The MPI Datatype

    aligned_vector<T> buffer_send; //Data buffer
    aligned_vector<T> buffer_recieve;
};
