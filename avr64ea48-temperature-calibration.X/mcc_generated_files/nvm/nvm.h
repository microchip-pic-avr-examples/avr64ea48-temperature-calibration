/**
 * 
 * @file nvm.h
 *
 * @defgroup nvm_driver  Non-Volatile Memory
 *
 * @brief This file contains API prototypes and other data types for the Non-Volatile Memory (NVM) driver.
 *
 * @version NVM Driver Version 2.1.0
 */

#ifndef NVM_H
#define NVM_H

#include <string.h>

/**
 * @ingroup nvm_driver
 * @brief Datatypes for NVM address and data
 */

/**
 * @ingroup nvm_driver
 * @brief Data type for the EEPROM data.
 */
typedef uint8_t eeprom_data_t;
/**
 * @ingroup nvm_driver
 * @brief Data type for the EEPROM address.
 */
typedef uint16_t eeprom_address_t;



/**
 * @ingroup nvm_driver
 * @enum nvm_status_t
 * @brief Contains the return codes for the NVM driver APIs.
 */
typedef enum {
    NVM_OK, /**<0 - The NVM operation succeeded.*/
    NVM_ERROR, /**<1 - The NVM operation failed.*/
    NVM_BUSY  /**<2 - The NVM write operation ongoing.*/
} nvm_status_t;

/**
 * @ingroup nvm_driver
 * @brief Initializes the NVM driver.
 * @param None.
 * @return None.
 */
void NVM_Initialize(void);

/**
 * @ingroup nvm_driver
 * @brief Returns the status of last the NVM operation.
 * @param None.
 * @retval NVM_OK - The NVM operation succeeded.
 * @retval NVM_ERROR - The NVM operation failed.
 */
nvm_status_t NVM_StatusGet(void);

/**
 * @ingroup nvm_driver
 * @brief Clears the status of last the NVM operation.
 * @param None.
 * @retval None.
 */
void NVM_StatusClear(void);


/**
 * @ingroup nvm_driver
 * @brief Reads one byte from the given EEPROM address.
 * @pre NVM must be initialized with @ref NVM_Initialize() before calling this API.
 * @param [in] address - Address of the EEPROM location to be read.
 * @return Byte read from the given EEPROM address.
 */
eeprom_data_t EEPROM_Read(eeprom_address_t address);

/**
 * @ingroup nvm_driver
 * @brief Writes one byte to the given EEPROM address.
 *      The EEPROM busy status must be checked using the @ref EEPROM_IsBusy() API to know if the write operation is completed.  
 *      Use the @ref NVM_StatusGet() API to see the result of the write operation.
 * @param [in] address - Address of the EEPROM location to be written.
 * @param [in] data - Byte to be written to the given EEPROM location.
 * @return None.
 */
nvm_status_t EEPROM_Write(eeprom_address_t address, eeprom_data_t data);

/**
 * @ingroup nvm_driver
 * @brief Checks if the EEPROM is busy.
 * @pre NVM must be initialized with @ref NVM_Initialize() before calling this API.
 * @param None.
 * @retval True - The EEPROM operation is being performed.
 * @retval False - The EEPROM operation is not being performed.
 */
bool EEPROM_IsBusy(void);


#endif  /* NVM_H */
