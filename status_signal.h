#pragma once
#include "mpi_manager.h"

/**
 * @brief This enum contains all signals that are sent between MPI ranks. Multiple signals are sent by logical OR signal1 | signal2 | ...
 */
enum status_signal
{
  /**
   * @brief No signal
   */
  NONE = 0,
  
  /**
   * @brief The simulation should run. If this signal is not recieved, the simulator will immediately stop.
   */
  SIM_RUNNING = 1,
    
  /**
   * @brief The GUI accepts states. The master simulator is allowed to send a state from its queue.
   */
  GUI_ACCEPTS_DATA = 2,
  
  /**
   * @brief The master simulator accepts a partial state from the slave simulator.
   */
  SIM_MASTER_ACCEPTS_DATA = 4,
  
  /**
   * @brief The slave simulator accepts partial state from master simulator. Used for synching borders.
   */
  SIM_SLAVE_ACCEPTS_DATA_PARTIAL = 8,
  
  /**
   * @brief The slave simulator accepts the full state from master simulator. Used for initialization.
   */
  SIM_SLAVE_ACCEPTS_DATA_FULL = 16,
  
  /**
   * @brief The GUI requests a reinitialization of the space from master simulator. The master simulator also sends this signal to the slave simulators to force initialization.
   */
  COMMAND_REINITIALIZE = 32
}

/*
 * 
 * Ablauf:
 * 
 * 1. Master initialisiert
 * 2. Master sendet COMMAND_REINITIALIZE an slaves
 * 3. [Slaves stoppen laufende simulation]
 * 4. Slaves senden SIM_SLAVE_ACCEPTS_DATA_FULL an master
 * 5. Master sendet alle nötigen daten an slaves (komplettes feld? etc)
 * 6. Master und slaves starten simulation
 * 
 * Wiederhole:
 * 1. Slaves rechnen und pausieren
 * 2. Slave empfängt SIM_MASTER_ACCEPTS_DATA und sendet daten an master
 * *. (somewhere) Slave rechnet schon mal weiter wo es geht
 * 3. Slaves senden SIM_SLAVE_ACCEPTS_DATA_PARTIAL
 * 4. Slaves bekommen ränder
 * 
 * Wiederhole:
 * 1. Master rechnet und pausiert dann
 * 2. Master sendet SIM_MASTER_ACCEPTS_DATA
 * 3. Master bekommt daten von slaves
 * 4. Master setzt feld zusammen -> in queue?
 * 
 */