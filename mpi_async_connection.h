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
        m_rank_sender(_sender),
        m_rank_reciever(_reciever),
        m_mpi_tag(_tag),
        m_datatype(_datatype),
        m_buffer_data(initial),
        m_current_state(states::IDLE),
        m_sender(mpi_rank() == _sender)
    
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
        if(m_current_state != states::IDLE)
        {
            cerr << "mpi_buffer_connection: Buffer can only be returned in IDLE state!" << " rank: " << mpi_rank() << endl;
            exit(EXIT_FAILURE);
        }
    
        return &m_buffer_data;
    }
    
    /**
    * @brief Sends the data to reciever if sender; Accept new connection if reciever
    */
    void flush()
    {
        if(m_sender)
        {
            if(m_current_state != states::IDLE)
            {
                cerr << "mpi_buffer_connection: flush() called on sender that is not IDLE"  << " rank: " << mpi_rank() << endl;
                exit(EXIT_FAILURE);
            }
            
            //Send the data now
            int send_size = m_buffer_data.size();
            
            MPI_Isend(m_buffer_data.data(),
                      send_size,
                      MPI_FLOAT,
                      m_rank_reciever,
                      m_mpi_tag,
                      MPI_COMM_WORLD,
                      &m_request_data);
                      
            m_current_state = states::DATA;
        }
        else
        {
            if(m_current_state != states::IDLE)
            {
                cerr << "mpi_connection: flush() called on reciever that is not IDLE"  << " rank: " << mpi_rank() << endl;
                exit(EXIT_FAILURE);
            }
            
            //Got buffer size
            int recieve_size = m_buffer_data.size();
            
            //Request the data
            MPI_Irecv(m_buffer_data.data(),
                      recieve_size,
                      MPI_FLOAT,
                      m_rank_sender,
                      m_mpi_tag,
                      MPI_COMM_WORLD,
                      &m_request_data);
            
            m_current_state = states::DATA;
        }
    }
    
    /**
    * @brief Updates the connection. Returns the state after update.
    */
    states update()
    {
        if(m_sender)
            return update_sender();
        else
            return update_reciever();
    }
        
    void cancel()
    {
        //Cancel all open connections
        mpi_cancel_if_needed(&m_request_data);
    }
    
    int get_rank_sender()
    {
        return m_rank_sender;
    }
    
    int get_rank_reciever()
    {
        return m_rank_reciever;
    }
    
private:

    const int m_rank_sender;
    const int m_rank_reciever;
    const int m_mpi_tag;
    
    const bool m_sender; //If this connection is a sender
    const MPI_Datatype m_datatype; //The MPI Datatype
    
    states m_current_state; // Current state of this connection
    
    aligned_vector<T> m_buffer_data; //Data buffer
    
    MPI_Request m_request_data = MPI_REQUEST_NULL;
    
    states update_sender()
    {
        if (m_current_state == states::DATA && mpi_test(&m_request_data))
        {
            //Data sent. Go to idle.
            
            m_current_state = states::IDLE;
        }
        
        return m_current_state;
    }
    
    states update_reciever()
    {
        if (m_current_state == states::DATA && mpi_test(&m_request_data))
        {
            //Got the data. Go to idle
            
            m_current_state = states::IDLE;
        }
        
        return m_current_state;
    }    
};
