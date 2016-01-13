#pragma once

#include <vector>
#include "communication.h"
#include "aligned_vector.h"

using namespace std;

template <typename T>
/**
* @brief Contains a MPI connection between sender and reciever which sends a variable buffer from sender to reciever.
*/
class mpi_variable_buffer_connection
{
public:

    enum states
    {
        IDLE = 0, //The connection is idle. Sender awaits for data to be set, reciever has no open connections and data can be read
        PREPARE = 1, //The buffer size is sent. Sender waits until data is sent. Reciever waits for sending finished.
        DATA = 2, //The actual data is sent. Sender waits until data is sent. Reciever waits until this is finished.
    };
    
    mpi_variable_buffer_connection(int _sender, int _reciever, int _tag, MPI_Datatype _datatype, aligned_vector<T> initial) : 
        rank_sender(_sender),
        rank_reciever(_reciever),
        mpi_tag(_tag),
        datatype(_datatype),
        buffer_data(initial),
        buffer_prepare(0),
        current_state(states::IDLE),
        sender(mpi_rank() == _sender)
    
    {
        if(_sender < 0 || _reciever < 0 || _sender >= mpi_comm_size() || _reciever >= mpi_comm_size())
        {
            cerr << "Cannot initialize mpi_variable_buffer_connection with invalid sender and reciever" << endl;
            exit(EXIT_FAILURE);
        }
        if(_sender != mpi_rank() && _reciever != mpi_rank())
        {
            cerr << "Cannot initialize mpi_variable_buffer_connection without sender or reciever being current rank!" << endl;
            exit(EXIT_FAILURE);
        }
        if(initial.size() == 0)
        {
            cerr << "Cannot initialize mpi_variable_buffer_connection with empty buffer!" << endl;
            exit(EXIT_FAILURE);
        }
    }   
    
    mpi_variable_buffer_connection(int _sender, int _reciever, int _tag, int _buffer_size, MPI_Datatype _datatype) : 
    mpi_variable_buffer_connection(_sender,_reciever,_tag,aligned_vector<T>(_buffer_size),_datatype)
    
    {
    }   
        
    /**
    * @brief Returns the buffer if state is IDLE if sender and state is IDLE
    */
    aligned_vector<T> * get_buffer()
    {       
        if(current_state != states::IDLE)
        {
            cerr << "mpi_variable_buffer_connection: Buffer can only be returned in IDLE state!" << endl;
            exit(EXIT_FAILURE);
        }
    
        return &buffer_data;
    }
    
    /**
    * @brief Returns the data size contained in buffer if state is IDLE
    */
    int get_data_size()
    {
        if(current_state != states::IDLE)
        {
            cerr << "mpi_variable_buffer_connection: Data size can only be returned in IDLE state!" << endl;
            exit(EXIT_FAILURE);
        }
        
        return buffer_prepare;
    }
    
    /**
    * @brief Sets the size of send data. Can only be called by sender and if state is IDLE.
    */
    void set_data_size(int size)
    {
        if(!sender || current_state != states::IDLE)
        {
            cerr << "mpi_variable_buffer_connection: Cannot set send size!" << endl;
            exit(EXIT_FAILURE);
        }
        if(size <= 0)
        {
            cerr << "mpi_variable_buffer_connection: Send size should be at least 1!" << endl;
            exit(EXIT_FAILURE);
        }
        
        buffer_prepare = size;
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
                cerr << "mpi_variable_buffer_connection: flush() called on sender that is not IDLE" << endl;
                exit(EXIT_FAILURE);
            }
            
            MPI_Isend(&buffer_prepare,
                      1,
                      MPI_INT,
                      rank_reciever,
                      mpi_tag,
                      MPI_COMM_WORLD,
                      &request_prepare);
            
            current_state = states::PREPARE;
        }
        else
        {
            if(current_state != states::IDLE)
            {
                cerr << "mpi_variable_buffer_connection: flush() called on reciever that is not IDLE" << endl;
                exit(EXIT_FAILURE);
            }
            
            MPI_Irecv(&buffer_prepare,
                      1,
                      MPI_INT,
                      rank_sender,
                      mpi_tag,
                      MPI_COMM_WORLD,
                      &request_prepare);
            
            current_state = states::PREPARE;            
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
        mpi_cancel_if_needed(&request_prepare);
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
    
    int buffer_prepare; //Send data size buffer
    aligned_vector<T> buffer_data; //Data buffer
    
    MPI_Request request_prepare = MPI_REQUEST_NULL;
    MPI_Request request_data = MPI_REQUEST_NULL;
    
    states update_sender()
    {
        if (current_state == states::PREPARE && mpi_test(&request_prepare))
        {
            //Send the data now
            int send_size = buffer_prepare;
            
            MPI_Isend(buffer_data.data(),
                      send_size,
                      datatype,
                      rank_reciever,
                      mpi_tag,
                      MPI_COMM_WORLD,
                      &request_data);
                      
            current_state = states::DATA;
        }
        else if (current_state == states::DATA && mpi_test(&request_data))
        {
            //Data sent. Go to idle.
            
            current_state = states::IDLE;
        }
        
        return current_state;
    }
    
    states update_reciever()
    {
        if (current_state == states::PREPARE && mpi_test(&request_prepare))
        {
            //Got buffer size
            int recieve_size = buffer_prepare;
            
            //Request the data
            MPI_Irecv(buffer_data.data(),
                      recieve_size,
                      datatype,
                      rank_sender,
                      mpi_tag,
                      MPI_COMM_WORLD,
                      &request_data);
            
            current_state = states::DATA;
        }
        else if (current_state == states::DATA && mpi_test(&request_data))
        {
            //Got the data. Go to idle
            
            current_state = states::IDLE;
        }
        
        return current_state;
    }    
};
