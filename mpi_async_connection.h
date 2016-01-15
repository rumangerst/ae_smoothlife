#pragma once

#include <vector>
#include "communication.h"

using namespace std;

template <typename T>
/**
* @brief Contains an async MPI connection between sender and reciever.
*/
class mpi_async_connection
{
public:

    enum states
    {
        IDLE = 0, //The connection is idle. Sender awaits for data to be set, reciever has no open connections and data can be read
        DATA = 1  //The buffer is sent. Sender waits until data is sent. Reciever waits for sending finished.
    };
    
    mpi_async_connection(int _sender, int _reciever, int _tag, MPI_Datatype _datatype, aligned_vector<T> initial) : 
        rank_sender(_sender),
        rank_reciever(_reciever),
        mpi_tag(_tag),
        datatype(_datatype),
        buffer_data(initial),
        current_state(states::IDLE),
        sender(mpi_rank() == _sender)
    
    {
        if(_sender < 0 || _reciever < 0 || _sender >= mpi_comm_size() || _reciever >= mpi_comm_size())
        {
            cerr << "Cannot initialize mpi_connection with invalid sender and reciever" << endl;
            exit(EXIT_FAILURE);
        }
        if(_sender != mpi_rank() && _reciever != mpi_rank())
        {
            cerr << "Cannot initialize mpi_connection without sender or reciever being current rank!" << endl;
            exit(EXIT_FAILURE);
        }
        if(initial.size() == 0)
        {
            cerr << "Cannot initialize mpi_connection with empty buffer!" << endl;
            exit(EXIT_FAILURE);
        }        
    }   
    
    mpi_async_connection(int _sender, int _reciever, int _tag, int _buffer_size, MPI_Datatype _datatype) : 
    mpi_async_connection(_sender,_reciever,_tag,_datatype,aligned_vector<T>(_buffer_size))
    
    {
    }    
    
    ~mpi_async_connection()
    {
        cancel();
    }

    
    /**
    * @brief Returns the buffer if state is IDLE if sender and state is IDLE
    */
    aligned_vector<T> * get_buffer()
    {       
        if(current_state != states::IDLE)
        {
            cerr << "mpi_buffer_connection: Buffer can only be returned in IDLE state!" << " rank: " << mpi_rank() << endl;
            exit(EXIT_FAILURE);
        }
    
        return &buffer_data;
    }
    
    /**
    * @brief Sends the data to reciever if sender; Accept new connection if reciever
    */
    void flush()
    {
        if(sender)
        {
            if(current_state != states::IDLE)
            {
                cerr << "mpi_buffer_connection: flush() called on sender that is not IDLE"  << " rank: " << mpi_rank() << endl;
                exit(EXIT_FAILURE);
            }
            
            //Send the data now
            int send_size = buffer_data.size();
            
            MPI_Isend(buffer_data.data(),
                      send_size,
                      MPI_FLOAT,
                      rank_reciever,
                      mpi_tag,
                      MPI_COMM_WORLD,
                      &request_data);
                      
            current_state = states::DATA;
        }
        else
        {
            if(current_state != states::IDLE)
            {
                cerr << "mpi_connection: flush() called on reciever that is not IDLE"  << " rank: " << mpi_rank() << endl;
                exit(EXIT_FAILURE);
            }
            
            //Got buffer size
            int recieve_size = buffer_data.size();
            
            //Request the data
            MPI_Irecv(buffer_data.data(),
                      recieve_size,
                      MPI_FLOAT,
                      rank_sender,
                      mpi_tag,
                      MPI_COMM_WORLD,
                      &request_data);
            
            current_state = states::DATA;
        }
    }
    
    /**
    * @brief Updates the connection. Returns the state after update.
    */
    states update()
    {
        if(sender)
            return update_sender();
        else
            return update_reciever();
    }
        
    void cancel()
    {
        //Cancel all open connections
        mpi_cancel_if_needed(&request_data);
    }
    
    int get_rank_sender()
    {
        return rank_sender;
    }
    
    int get_rank_reciever()
    {
        return rank_reciever;
    }
    
private:

    const int rank_sender;
    const int rank_reciever;
    const int mpi_tag;
    
    const bool sender; //If this connection is a sender
    const MPI_Datatype datatype; //The MPI Datatype
    
    states current_state; // Current state of this connection
    
    aligned_vector<T> buffer_data; //Data buffer
    
    MPI_Request request_data = MPI_REQUEST_NULL;
    
    states update_sender()
    {
        if (current_state == states::DATA && mpi_test(&request_data))
        {
            //Data sent. Go to idle.
            
            current_state = states::IDLE;
        }
        
        return current_state;
    }
    
    states update_reciever()
    {
        if (current_state == states::DATA && mpi_test(&request_data))
        {
            //Got the data. Go to idle
            
            current_state = states::IDLE;
        }
        
        return current_state;
    }    
};
