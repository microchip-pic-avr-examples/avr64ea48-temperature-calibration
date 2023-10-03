 /*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.0
*/

/*
© [2023] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/
// includes
#include "mcc_generated_files/system/system.h"
#include <stdio.h>
#include <util/delay.h>

//defines
#define USART_ON                // enable output to terminal
#define SCALING_FACTOR 4096     // 12-bit: 2^12 = 4096

// EEPROM address space starts at 0x001400, these addresses are relative to that
#define CALCULATED_OFFSET_HIGH_ADDRESS 0x18 // Calculated offset high byte address
#define CALCULATED_OFFSET_LOW_ADDRESS (CALCULATED_OFFSET_HIGH_ADDRESS+1)  // Calculated offset low byte address
#define EXTERNAL_TEMPERATURE_CHECK_ADDRESS 0x20 // External temperature Check byte address
#define EXTERNAL_TEMPERATURE_HIGH_ADDRESS (EXTERNAL_TEMPERATURE_CHECK_ADDRESS+1)  // External temperature High byte address
#define EXTERNAL_TEMPERATURE_LOW_ADDRESS (EXTERNAL_TEMPERATURE_HIGH_ADDRESS+1) // External temperature Low byte address


// global variables
char string_buffer[10];

//functions
void USART1_SendString(const char *strptr);
int16_t ReadInternalTemperature(int16_t offset, int16_t gain);
void EEPROM_ReadRange(uint16_t addr_start, uint16_t addr_end);
int16_t ReadExternalTemperature(void);
int16_t GetSigrowOffset(void);
int16_t GetSigrowGain(void);
int16_t CalculateOffset(int16_t temp);


/*
 *  USART1_send_string
 * 
 *  Send a string using USART1 Tx
 *  C automatically adds a NULL at the end of a string, so when 
 *  the NULL value is reached the while(*strptr) loop ends
 */
void USART1_SendString(const char *strptr)
{
    while(*strptr)      // loop until reaching NULL
    {
        USART1_Write(*strptr);          // send byte
        strptr++;                       // increment string pointer
        while(!(USART1_IsTxDone()))     // wait for Tx buffer empty
            ;
        USART1.STATUS |= USART_TXCIF_bm;    // clear Transmit complete flag
    }
}


/*
 *  Read_int_temp
 * 
 *  Make a reading of the internal temperature
 *  Return temperature as signed 16-bit integer
 */
int16_t ReadInternalTemperature(int16_t offset, int16_t gain)
{
    
    while(ADC0.STATUS)      // wait for ADC0 ready
        ;
    ADC0_StartConversion(ADC_MUXPOS_TEMPSENSE_gc);  // start temperature measurement    
    while(!ADC0_IsConversionDone())     // wait for conversion to complete
        ;     
            
    uint16_t adc_reading = ADC0.RESULT;                 // ADC conversion result
    int32_t temp = ((int32_t) adc_reading) + offset;    // add offset to ADC result
    temp *= gain;                   // result can overflow 16-bit value
    temp += SCALING_FACTOR / 2;     // ensures correct rounding on division below
    temp /= SCALING_FACTOR;         // round to the nearest integer in Kelvin
    uint16_t temp_in_K = (uint16_t) temp;       // absolute temperature
    int16_t temp_in_C = temp_in_K - 273;        // temperature in C°
    
#ifdef USART_ON
    sprintf(string_buffer, "%d°C\n", temp_in_C);
    USART1_SendString("Internal temperature reading: ");
    USART1_SendString(string_buffer);
#endif
    
    return temp_in_C;
}


/*
 *  Read_EEPROM
 * 
 *  Read out the contents of the EEPROM from addr_start (int16_t) to addr_end (int16_t)
 *  and print to terminal (over USART1)
 */
void EEPROM_ReadRange(uint16_t addr_start, uint16_t addr_end)
{
    uint16_t i = 0;
    uint8_t new_line = 0;
    uint16_t EEPROM_address;
    uint8_t EEPROM_data;
    
    for(i = addr_start; i < addr_end + 1; i++)
    {
        if(new_line == 0) 
        {
            EEPROM_address = EEPROM_START + i;          // fetch EEPROM address
            sprintf(string_buffer, "\n%#x: ", EEPROM_address);      // convert hex address to string
            USART1_SendString(string_buffer);                  // print hex address

        }
        
        new_line++;                     // increment new_line counter
        EEPROM_data = EEPROM_Read(i);   // read EEPROM byte at address i
        while(EEPROM_IsBusy())
            ;                           // waint for EEPROM to complete
        
        sprintf(string_buffer, "%02x ", EEPROM_data); // convert EEPROM hex data to string
        USART1_SendString(string_buffer);      // send to terminal
        
        if(new_line == 8) {     // after 8 bytes, new line
            new_line = 0;
        }
    }
    USART1_SendString("\n\n");     // send to terminal
}


/*
 *  Read_Ext_Temp
 * 
 *  Read out the two bytes of external temperature value stored in 
 *  EEPROM address "EXTERNAL TEMPERATURE" + 1 and print to terminal (over USART1)
 */
int16_t ReadExternalTemperature(void)
{
    int8_t t_hi, t_lo, temp_saved;
    int16_t temp;
    
    // if temperature is stored, first byte should be 0x5c
    temp_saved = EEPROM_Read(EXTERNAL_TEMPERATURE_CHECK_ADDRESS); 
    while(EEPROM_IsBusy())      
        ;
    
    // if external temperature value is stored
    if(temp_saved == 0x5c) {
        t_hi = EEPROM_Read(EXTERNAL_TEMPERATURE_HIGH_ADDRESS);       // read high byte
        while(EEPROM_IsBusy())
            ;
        t_lo = EEPROM_Read(EXTERNAL_TEMPERATURE_LOW_ADDRESS);       // read low byte
        while(EEPROM_IsBusy())
            ;
        temp = (t_hi << 8) | t_lo;      // convert into 16-bit

#ifdef USART_ON
        sprintf(string_buffer,"%d°C\n",temp);   
        USART1_SendString("\nExternal temperature measurement is: ");
        USART1_SendString(string_buffer);
#endif
        
    }
    // if external temperature is not stored
    else {
#ifdef USART_ON
        USART1_SendString("\nNo external temperature stored! \n\n");
#endif
        temp = 0x7fff;      // set to max to indicate no temp stored
    }
    return temp;
}


/*
 *  Get_Sigrow_Offset
 * 
 *  Read out the Offset from SIGROW
 *  and print to terminal (over USART1) and returns value (int16_t)
 */
int16_t GetSigrowOffset(void)
{
    int16_t offset;
   
    offset = (int16_t) SIGROW.TEMPSENSE1;  // signed offset from signature row

#ifdef USART_ON
    sprintf(string_buffer, "%d\n", offset);
    USART1_SendString("\nFactory calibrated Offset: ");
    USART1_SendString(string_buffer);
#endif
    
    return offset;
}


/*
 *  Get_Sigrow_Offset
 * 
 *  Read out the Offset from SIGROW
 *  and print to terminal (over USART1)
 */
int16_t GetSigrowGain(void)
{
    int16_t gain;
    
    gain = (int16_t) SIGROW.TEMPSENSE0;  // signed offset from signature row

#ifdef USART_ON
    sprintf(string_buffer, "%d\n", gain);
    USART1_SendString("\nFactory calibrated Gain: ");
    USART1_SendString(string_buffer);
 #endif
    
    return gain;
}


/*
 *  Calculate_offset
 * 
 *  Calculate a new offset using external temperature reading (int16_t) as input
 *  Stores new offset as int16_t in EEPROM address "CALCULATED_OFFSET" (see defines)
 */
int16_t CalculateOffset(int16_t temp)
{
    while(ADC0.STATUS)      // wait for ADC0 ready
        ;
    
    ADC0_StartConversion(ADC_MUXPOS_TEMPSENSE_gc);  // start temperature measurement    
    while(!ADC0_IsConversionDone())     // wait for conversion to complete
        ;
    
    int16_t sigrow_gain = (int16_t) SIGROW.TEMPSENSE0;    // signed gain from signature row
    uint16_t adc_reading = ADC0.RESULT;     // ADC conversion result
    
    int16_t ext_temp_K = temp + 273;        // convert temperature from Celsius to Kelvin
    // Calculate offset based on known external temperature
    float calculated_offset = ( ((float) ext_temp_K * SCALING_FACTOR) / (float) sigrow_gain ) - (float) adc_reading;
    calculated_offset += 0.5;     // add 0.5 to fix rounding errors
    calculated_offset = (int16_t) calculated_offset;        // recast value to int16_t
  
    return calculated_offset;
}



/*
    Main application
*/
int main(void)
{
    int16_t external_temperature;
    int16_t sigrow_offset;
    int16_t sigrow_gain;
    int16_t internal_temperature;
    int16_t new_offset;
    uint8_t data;

    SYSTEM_Initialize();    // init system
    
#ifdef USART_ON
    USART1_SendString("\n\n\n\n\nStarting...\n");
    
    // Show EEPROM contents
    USART1_SendString("\nListing EEPROM contents:\n");
    EEPROM_ReadRange(0, 511);       // EEPROM is 512 bytes
    
    USART1_SendString("\nChecking for external temperature stored in EEPROM...\n");
#endif
    
    external_temperature = ReadExternalTemperature();     // get external temperature
    sigrow_offset = GetSigrowOffset();        // get factory calibration value for offset
    sigrow_gain = GetSigrowGain();        // get factory calibration value for gain

    // read out internal temperature using factory calibration
    internal_temperature = ReadInternalTemperature(sigrow_offset, sigrow_gain);
    
    // calculate new offset using external temperature reading
    new_offset = CalculateOffset(external_temperature);
    
    // write high byte of new offset to EEPROM
    data = new_offset >> 8;     // note the use of uint8_t when sending signed 2 byte value,
                                // else the sign bit may be changed
    nvm_status_t EEstatus = EEPROM_Write(CALCULATED_OFFSET_HIGH_ADDRESS, data);
    while(EEPROM_IsBusy())
            ;
    if(EEstatus == NVM_ERROR)
      {
#ifdef USART_ON
        USART1_SendString("\nSomething went wrong! (New Offset high byte)\n");
#endif
    }
    
    // write low byte of new offset to EEPROM
    data = new_offset;
    EEstatus = EEPROM_Write(CALCULATED_OFFSET_LOW_ADDRESS, data);
    while(EEPROM_IsBusy())
            ;
    if(EEstatus == NVM_ERROR)
    {
#ifdef USART_ON
        USART1_SendString("\nSomething went wrong! (New Offset low byte)\n");
#endif
    }

#ifdef USART_ON
    USART1_SendString("\n\nNew offset calculated!\n");
    USART1_SendString("\nValue is...........");
    sprintf(string_buffer, "%d\n\n", new_offset);
    USART1_SendString("\nNew calculated Offset: ");
    USART1_SendString(string_buffer);
#endif
    
    while(1)
    {
       if(external_temperature != 0x7fff)       // if external temperature is stored, check temperature
       {                                        
            IO_PB3_SetDigitalOutput();          // turn on LED0
            // Now do a new temperature reading using adjusted offset
            internal_temperature = ReadInternalTemperature(new_offset, sigrow_gain);
            
            IO_PB3_SetDigitalInput();           // turn off CNANO LED0 (setting pin to input, turn off input buffer)
            
            _delay_ms(5000);     // wait 5 seconds between measurements
       }     
    }
}
