/**
  ******************************************************************************
  * @file           : RDA_5807.c
  * @date           : 8 April 2023
  * @author		    : NR
  * @brief          : Driver of the RDA 5807M chip (FM radio via I2c)
  *
  *                   Adapted from the RDA5807 library for STM32 developed by mhdfasilwyd
  *                   from the github : https://github.com/mhdfasilwyd/RDA5807_STM32
  *
  *                   the mhdfasilwyd version was adapted from those developed by
  *                   Ricardo Lima Caratti (Copyright (c) 2019 ) for Arduino
  *
  * @comment          Main adaptations : HAL_I2C function use and corrections
  *
  * @status : In course of validation
  *
  *
  ******************************************************************************
  */
#include <RDA_5807.h>
#include <stdlib.h>

#define WRITE_DELAY 3
#define MIN_DELAY 1

const uint16_t startBand[4] = {8700, 7600, 7600, 6500};
const uint16_t endBand[4] = {10800, 9100, 10800, 7600};
const uint16_t fmSpace[4] = {100, 200, 50, 25};

RDA_Handle handle = {};

typedef union {
    struct
    {
        uint8_t low;
        uint8_t high;
    } write;
    uint16_t all;
} wordToByte;

/** NR
  * @brief  Send the word sequence corresponding to
  *            1 : the register Number
  *            2 : the MSB of the 16 bits register value
  *            3 : the LSB of the 16 bits register value
  *
  * @param  hi2c			pointer to the i2c handler (2C_HandleTypeDef *)
  * @param  reg				addressed register number
  * @param  word_sequence 	pointer to a 16 bits value to store in the 16 bits register
  * @retval HAL status
  */
HAL_StatusTypeDef registerWrite(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t value)
{
	uint8_t word_sequence[3]; //   ==> | Reg_nb | value[15..8] | value [7..0] |

	word_sequence[0] = reg;
	word_sequence[1] =  (uint8_t)((value >> 8) & 0x00FF);
	word_sequence[2] =  (uint8_t)( value  & 0x00FF );

	return HAL_I2C_Master_Transmit(hi2c, I2C_ADDR_DIRECT_ACCESS, (uint8_t *)&word_sequence, 3, 1000);
}

/**
 * @ingroup GA03
 * @brief 	or RegisterRead : Gets the content of a given status register (from 0x0A to 0x0F)
 * @details This method update the first element of the shadowStatusRegisters linked to the register
 *
 * @param  hi2c		pointer to the i2c handler (2C_HandleTypeDef *)
 */
void getStatus(I2C_HandleTypeDef *hi2c, uint8_t reg)
{
    wordToByte rdata;
    uint8_t temp;

    // Send the Register number to read
    HAL_I2C_Master_Transmit(hi2c, I2C_ADDR_DIRECT_ACCESS, (uint8_t *)&reg, 1, 1000);

    // Read the 16 bits register
    HAL_I2C_Master_Receive(hi2c,  I2C_ADDR_DIRECT_ACCESS, (uint8_t *)&rdata, 2, 1000);

    // flip the MSB and LSB
    temp = rdata.write.low;
    rdata.write.low = rdata.write.high;
    rdata.write.high = temp;

    // Update the corresponding register in the local register structure (shadow structure)
    switch (reg)
    {
    case REG03:
    	handle.reg03 = (RDA_Reg03)rdata.all;//temp;
    	break;
    case REG05:
    	handle.reg05 = (RDA_Reg05)rdata.all;//temp;
    	break;
    case REG0A:
        handle.reg0A = (RDA_Reg0A)rdata.all;//temp;
        break;
    case REG0B:
        handle.reg0B = (RDA_Reg0B)rdata.all;//temp;
        break;
    case REG0C:
        handle.reg0C = (RDA_Reg0C)rdata.all;//temp;
        break;
    case REG0D:
        handle.reg0D = (RDA_Reg0D)rdata.all;//temp;
        break;
    case REG0E:
        handle.reg0E = (RDA_Reg0E)rdata.all;//temp;
        break;
    case REG0F:
        handle.reg0F = (RDA_Reg0F)rdata.all;//temp;
        break;
    default:
        break;
    }
    // nothing to return, the local structure is updated
}

/**
 * @ingroup GA03
 * @brief Waits for Seek or Tune finish
 * @param hi2c	pointer to the i2c handler (2C_HandleTypeDef *)
 * @return 		Status of the operation (RDA_5807_return_t )
 */
RDA_5807_return_t waitAndFinishTune(I2C_HandleTypeDef *hi2c)
{
    do
	{
        //getStatus(I2Cx, REG0A);
    	getStatus(hi2c, REG0A);
    	HAL_Delay(5);
    	// NR : to add a timeout
    }
	while (handle.reg0A.refined.STC == 0);
    return RDA_OK;
}

/**
 * @ingroup RDA_API
 * @brief Init the RDA chip
 * @param hi2c	pointer to the i2c handler (2C_HandleTypeDef *)
 * @return 		Status of the operation (RDA_5807_return_t )
 */
RDA_5807_return_t RDA_Init(I2C_HandleTypeDef *hi2c)
{
	// Do a software reset
	handle.reg02.raw = 0x02;
	if (HAL_OK != registerWrite(hi2c, REG02, handle.reg02.raw) )
	{
		return RDA_Init_Err1;
	}

	// reset the reg02 structure
    handle.reg02.raw = 0x0;
    // initialize the given bits
    handle.reg02.refined.NEW_METHOD = 1;
    handle.reg02.refined.RDS_EN = 1; // NR change RDS disable
    handle.reg02.refined.CLK_MODE = CLOCK_32K;
    handle.reg02.refined.RCLK_DIRECT_IN = 1; // NR change OSCILLATOR_TYPE_CRYSTAL;
    handle.reg02.refined.MONO = 0; // Force mono
    handle.reg02.refined.DMUTE = 1; // Normal operation
    handle.reg02.refined.DHIZ = 1; // Normal operation
    handle.reg02.refined.ENABLE = 1;
    handle.reg02.refined.BASS = 1;
    handle.reg02.refined.SEEK = 0;

    // initialize the corresponding register
    //registerWrite(I2Cx, REG02, handle.reg02.raw);
    if (HAL_OK != registerWrite(hi2c, REG02, handle.reg02.raw) )
    {
    	return RDA_Init_Err1;
    }

    handle.reg04.raw = 0x0400;
    if (HAL_OK != registerWrite(hi2c, REG04, handle.reg04.raw) )
	{
		return RDA_Init_Err2;
	}

    // reset the reg05 structure
    handle.reg05.raw = 0x0;
    // intialize the given bits
    handle.reg05.refined.INT_MODE = 1; // NR change
    handle.reg05.refined.LNA_PORT_SEL = 2;
    handle.reg05.refined.LNA_ICSEL_BIT = 0;
    handle.reg05.refined.SEEK_MODE = 2; // Seekmode 0 = normal mode ; 2 (10)_2 =using the RSSI (old seek mode)
    handle.reg05.refined.SEEKTH = 1;    // initial = 0B1000, niveau descendu
    handle.reg05.refined.VOLUME = 12;
    // transfer the constructed word to the register

    //registerWrite(I2Cx, REG05, handle.reg05.raw);
    if (HAL_OK != registerWrite(hi2c, REG05, handle.reg05.raw) )
    {
    	return RDA_Init_Err3;
    }

    // Initialize the direct variables of the local structure
    handle.currentFrequency = 8700;
    handle.currentFMBand    = 0;
    handle.currentFMSpace   = 0;
    handle.currentVolume    = 8;

    // NO problem : all is OK
   	return RDA_OK;
}

/**
 * @ingroup RDA_API
 * @brief Disable the Power on the RDA chip
 * @param hi2c	pointer to the i2c handler (2C_HandleTypeDef *)
 */
void RDA_PowerDown(I2C_HandleTypeDef *hi2c)
{
    handle.reg02.refined.SEEK = 0;
	handle.reg02.refined.ENABLE = 0;	// 0 = disable the Power-Up

	registerWrite(hi2c, REG02, handle.reg02.raw);
}

/**
 * @ingroup RDA_API
 * @brief Soft reset the RDA chip
 * @param hi2c	pointer to the i2c handler (2C_HandleTypeDef *)
 */
RDA_5807_return_t  RDA_SoftReset(I2C_HandleTypeDef *hi2c)
{
    handle.reg02.refined.SOFT_RESET = 1;

    if (HAL_OK != registerWrite(hi2c, REG02, handle.reg02.raw))
    	return RDA_Soft_Reset_Err;
    else
    	return RDA_OK;
}

/**
 * @ingroup RDA_API (Internal)
 * @brief Set channel (not the frequency) on RDA chip
 * @param hi2c 		pointer to the i2c handler (2C_HandleTypeDef *)
 * @param channel 	integer value to push in the adapted register for frequency change  (uint16_t)
 */
RDA_5807_return_t RDA_SetChannel(I2C_HandleTypeDef *hi2c, uint16_t channel)
{
    handle.reg03.refined.CHAN = channel;
    handle.reg03.refined.TUNE = 1;
    handle.reg03.refined.BAND = RDA_FM_BAND_USA_EU;
    handle.reg03.refined.SPACE = 0;
    handle.reg03.refined.DIRECT_MODE = 0;
    //registerWrite(I2Cx, REG03, handle.reg03.raw);

    if (HAL_OK != registerWrite(hi2c, REG03, handle.reg03.raw))
    	return RDA_SetChannel_Err;
    else{
    	 //waitAndFinishTune(I2Cx);
//    	 if ( RDA_OK != waitAndFinishTune(hi2c) )
//    		 return RDA_Wait_Tune_Err;
//    	 else
    		 return RDA_OK;
    }
}

/**
 * @ingroup RDA_API
 * @brief Set frequency on RDA chip
 * @param hi2c 		pointer to the i2c handler (2C_HandleTypeDef *)
 * @param frequency the radio frequency in kHZ/10 to play (9900 for 99.00 MHz)
 */
RDA_5807_return_t RDA_Tune(I2C_HandleTypeDef *hi2c, uint16_t frequency)
{
    uint16_t channel = (frequency - startBand[handle.currentFMBand] ) / (fmSpace[handle.currentFMSpace] / 10.0);

    if (RDA_OK != RDA_SetChannel(hi2c, channel))
    	return RDA_Tune_Err;
    else // Every thing is OK : update the local RDA5807 structure
    {
    	handle.currentFrequency = frequency;
    	return RDA_OK;
    }

}

/**
 * @ingroup RDA_API
 * @brief Call manual seek down on RDA chip
 * @param hi2c	pointer to the i2c handler (2C_HandleTypeDef *)
 */
void RDA_ManualUp(I2C_HandleTypeDef *hi2c)
{
    if (handle.currentFrequency < endBand[handle.currentFMBand])
    {
        handle.currentFrequency += (fmSpace[handle.currentFMSpace] / 10.0);
    }
    else
    {
        handle.currentFrequency = startBand[handle.currentFMBand];
    }

    RDA_Tune(hi2c, handle.currentFrequency);
}

/**
 * @ingroup RDA_API
 * @brief Call manual seek up on RDA chip
 * @param hi2c	pointer to the i2c handler (2C_HandleTypeDef *)
 */
void RDA_ManualDown(I2C_HandleTypeDef *hi2c)
{
    if (handle.currentFrequency > startBand[handle.currentFMBand])
    {
        handle.currentFrequency -= (fmSpace[handle.currentFMSpace] / 10.0);
    }
    else
    {
        handle.currentFrequency = endBand[handle.currentFMBand];
    }

    RDA_Tune(hi2c, handle.currentFrequency);
}

/**
 * @ingroup RDA_API (Internal)
 * @brief Get real channel on RDA chip
 * @param I2Cx I2C Port
 */
uint16_t RDA_GetRealChannel(I2C_HandleTypeDef *hi2c)
{
	//getStatus(hi2c, REG0A);  // NR : trouble with READCHAN : to study what it is the problem
    //return handle.reg0A.refined.READCHAN;
	getStatus(hi2c, REG03);
	return handle.reg03.refined.CHAN;
}

/**
 * @ingroup RDA_API
 * @brief Get real frequency on RDA chip
 * | Band   | Formula |
 * | ------ | ------- | 
 * |    0   | Frequency = Channel Spacing (kHz) x READCHAN[9:0]+ 87.0 MHz |
 * | 1 or 2 | Frequency = Channel Spacing (kHz) x READCHAN[9:0]+ 76.0 MHz |
 * |    3   | Frequency = Channel Spacing (kHz) x READCHAN[9:0]+ 65.0 MHz |
 * @param I2Cx I2C Port
 */
uint16_t RDA_GetRealFrequency(I2C_HandleTypeDef *hi2c)
{
   // Debug Version : recall the local stored value, don't ask to the radio chip
	//return handle.currentFrequency;
	return (RDA_GetRealChannel(hi2c) * (fmSpace[handle.currentFMSpace] / 10.0) + startBand[handle.currentFMBand]);
}

/**
 * @ingroup RDA_API
 * @brief  Call seek on RDA chip
 * @param  I2Cx I2C Port
 * @param  seek_mode if 0, wrap at the upper or lower band limit and continue seeking;
 *                     1 = stop seeking at the upper or lower band limit
 * @param  direction if 0, seek down; if 1, seek up.
 * @param  showfreq() : NULL or the function displaying a slider or the current test frequency (ShowFreq in I2C_tools.c for example)
 * @return RDA_5807_status
 *
 * @details 	Documentation :
 *				The Seek procedure ENDS WHEN a channel is found OR entire bands has been searched.
 *				At the end, the SEEK bit is set low  AND the STC bit is set high when the seek operation completes.
 *				Bit SF (reg $0A)   : Seek Fail = 0 if seek successfull, 1 if failure (the seek fail flag is set when the
 *									 seek operation fails to find a channel with an RSSI >= SEEKTH (reg $05)
 * 				Bit STC :(reg $0A) : Seek/Tune Complete: 0= not complete, 1=complete; the STC Flag is set when the seek
 *									 or tune operation completes
 *				Bit Seek_mode(Reg $05) :  3 = seek using the RSSI seek mode, 0 by default
 *				SeekTH (Reg $05)	: Seek SNR threshold value
 *				Seek_TH_OLD			: Seek threshold for old seek mode, valid if Seek_Mode = 2 (b10)
 */
RDA_5807_return_t RDA_Seek(I2C_HandleTypeDef *hi2c, uint8_t seek_mode, uint8_t direction, void (*showfreq)() )
{
	// repeat the search until the Seek is incompleted
	do
	{
		//-----------------------------------------------------------------------------------
		// Step 1: initialize and run the Seek mode ==> Reg 02
		//-----------------------------------------------------------------------------------
		handle.reg02.refined.SEEK = 1;			// 0 = Disable or stop seek; 1 = Enable seek. 			                                        //
		handle.reg02.refined.SEEKUP = direction;// 1 = SeekUp, 0= Seekdown
		handle.reg02.refined.SKMODE = seek_mode;// 1 = stop seeking at the upper or lower band limit
											    // 0 : 0 wrap at the upper or lower band limit and continue seeking
		registerWrite(hi2c, REG02, handle.reg02.raw); // Start Seeking after the Reg02 update
		// The Seek bit will go down to 0 and the STC  set to high when the seek operation will be complete

		//-----------------------------------------------------------------------------------
		// Step 2: Show the progression of the frequency search using the transmit function
		//-----------------------------------------------------------------------------------
		if (showfreq != NULL)  showfreq(); 			// showfreq will use the Reg0A : READCHAN[9:0]
													// or GetRealChannel
													// function defined in I2C_tools.c for the radio application
		//-----------------------------------------------------------------------------------
		// Step 3: assess the STC bit to know if the seek is complete
		//-----------------------------------------------------------------------------------
		HAL_Delay(10); 			// wait a certain time for the seek operation
		getStatus(hi2c, REG0A); // Check Reg0A including the Seek/ Tune Complete bit and Seek Fail bit
	}
	while (handle.reg0A.refined.STC ==0); // continue if STC = 0 (Not complete)


	//-----------------------------------------------------------------------------------
	// Step 4: Set the new station as the new current frequency or return RDA_Seek_Failed
	//-----------------------------------------------------------------------------------


	if (handle.reg0A.refined.SF == 1) // Seek Failed
		return RDA_Seek_Failed;

	else	// Seek Succesfull
	{
		RDA_SetChannel(hi2c, RDA_GetRealChannel(hi2c));	// Select the new frequency as the new current radio station
		return RDA_OK;
	}
}

/**
 * @ingroup RDA_API
 * @brief Set seek threshold on RDA chip
 * @param I2Cx I2C Port
 * @param value RSSI threshold
 */
void RDA_SetSeekThreshold(I2C_HandleTypeDef *hi2c, uint8_t value)
{
    handle.reg05.refined.SEEKTH = value;

    registerWrite(hi2c, REG05, handle.reg05.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set FM band on RDA chip
 * @param I2Cx I2C Port
 * @param band FM band
 * | Value | Description                 |
 * | ----- | --------------------------- |
 * | 00    | 87–108 MHz (US/Europe)      |
 * | 01    | 76–91 MHz (Japan)           |
 * | 10    | 76–108 MHz (world wide)     |
 * | 11    | 65 –76 MHz (East Europe) or 50-65MHz (see bit 9 of gegister 0x06) |
 */
void RDA_SetBand(I2C_HandleTypeDef *hi2c, uint8_t band)
{
    handle.reg03.refined.BAND = band;

    registerWrite(hi2c, REG03, handle.reg03.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set FM space on RDA chip
 * @param I2Cx I2C Port
 * @param space FM space
 * | Value | Description |
 * | ----- | ----------- |
 * | 00    | 100KHz      |
 * | 01    | 200KHz      |
 * | 10    | 50KHz       |
 * | 11    | 25KHz       |
 */
void RDA_SetSpace(I2C_HandleTypeDef *hi2c, uint8_t space)
{
    handle.reg03.refined.SPACE = space;

    registerWrite(hi2c, REG03, handle.reg03.raw);
}

/**
 * @ingroup RDA_API
 * @brief Get the current RSSI
 * @param I2Cx I2C Port
 * @details RSSI - 000000(Min) 111111(Max) RSSI scale is logarithmic.
 * @return uint8_t
 */
uint8_t RDA_GetQuality(I2C_HandleTypeDef *hi2c)
{
	getStatus(hi2c, REG0B);
    return handle.reg0B.refined.RSSI;
}

/**
 * @ingroup RDA_API
 * @brief Set FM soft mute on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetSoftMute(I2C_HandleTypeDef *hi2c, BOOL value)
{
    handle.reg04.refined.SOFTMUTE_EN = value;

    registerWrite(hi2c, REG04, handle.reg04.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set FM mute on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */

void RDA_SetMute(I2C_HandleTypeDef *hi2c, BOOL value)
{
    handle.reg02.refined.SEEK = 0;    
    handle.reg02.refined.DHIZ = !value;

    registerWrite(hi2c, REG02, handle.reg02.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set mono on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetMono(I2C_HandleTypeDef *hi2c, BOOL value)
{
    handle.reg02.refined.SEEK = 0;
    handle.reg02.refined.MONO = value;

    registerWrite(hi2c, REG02, handle.reg02.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set bass on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetBass(I2C_HandleTypeDef *hi2c, BOOL value)
{
    handle.reg02.refined.SEEK = 0;
    handle.reg02.refined.BASS = value;

    registerWrite(hi2c, REG02, handle.reg02.raw);
}

/**
 * @ingroup RDA_API
 * @brief Get mono status on RDA chip
 * @param I2Cx I2C Port
 * @return value TRUE/FALSE
 */
BOOL RDA_GetStereoStatus(I2C_HandleTypeDef *hi2c)
{
	getStatus(hi2c, REG0A);
    return handle.reg0A.refined.ST;
}

/**
 * @ingroup RDA_API
 * @brief Set volume on RDA chip
 * @param I2Cx I2C Port
 * @param value 0-15 levels
 */
void RDA_SetVolume(I2C_HandleTypeDef *hi2c, uint8_t value)
{
    value > 15 ? value = 15 : value;
    handle.reg05.refined.VOLUME = handle.currentVolume = value;

    registerWrite(hi2c, REG05, handle.reg05.raw);
}

/**
 * @ingroup RDA_API
 * @brief Get internal volume on RDA chip
 * @param I2Cx I2C Port
 * @return uint8_t 0-15 levels
 */
uint8_t RDA_GetVolume(I2C_HandleTypeDef *hi2c)
{
    return(handle.currentVolume);
}

/**
 * @ingroup RDA_API
 * @brief Call volume up on RDA chip
 * @param I2Cx I2C Port
 */
void RDA_SetVolumeUp(I2C_HandleTypeDef *hi2c)
{
    if (handle.currentVolume < 15)
    {
        handle.currentVolume++;

        RDA_SetVolume(hi2c, handle.currentVolume);
    }
}

/**
 * @ingroup RDA_API
 * @brief Call volume down on RDA chip
 * @param I2Cx I2C Port
 */
void RDA_SetVolumeDown(I2C_HandleTypeDef *hi2c)
{
    if (handle.currentVolume > 0)
    {
        handle.currentVolume--;

        RDA_SetVolume(hi2c, handle.currentVolume);
    }
}

/**
 * @ingroup RDA_API
 * @brief Set FM De-Emphasis on RDA chip
 * @param I2Cx I2C Port
 * @param deEmphasis deEmphasis
 */
void RDA_SetFMDeEmphasis(I2C_HandleTypeDef *hi2c, uint8_t deEmphasis)
{
    handle.reg04.refined.DE = deEmphasis;

    registerWrite(hi2c, REG04, handle.reg04.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set RDS on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetRDS(I2C_HandleTypeDef *hi2c, BOOL value)
{
    handle.reg02.refined.SEEK = 0;
    handle.reg02.refined.RDS_EN = value;

    registerWrite(hi2c, REG02, handle.reg02.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set RBDS on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetRBDS(I2C_HandleTypeDef *hi2c, BOOL value)
{
    handle.reg02.refined.SEEK = 0;
    handle.reg02.refined.RDS_EN = 1;
    registerWrite(hi2c, REG02, handle.reg02.raw);

    handle.reg04.refined.RBDS = value;
    registerWrite(hi2c, REG04, handle.reg04.raw);
}

/**
 * @ingroup RDA_API
 * @brief Get RDS Ready on RDA chip
 * @param I2Cx I2C Port
 * @return TRUE/FALSE
 */
BOOL RDA_GetRDSReady(I2C_HandleTypeDef *hi2c)
{
	getStatus(hi2c, REG0A);
    return(handle.reg0A.refined.RDSR);
}

/**
 * @ingroup RDA_API
 * @brief Get RDS Sync on RDA chip
 * @param I2Cx I2C Port
 * @return TRUE/FALSE
 */
BOOL RDA_GetRDSSync(I2C_HandleTypeDef *hi2c)
{
	getStatus(hi2c, REG0A);
    return handle.reg0A.refined.RDSS;
}

/**
 * @ingroup RDA_API
 * @brief Get Block ID on RDA chip
 * @param I2Cx I2C Port
 * @return uint8_t
 */
uint8_t RDA_GetBlockId(I2C_HandleTypeDef *hi2c)
{
 	getStatus(hi2c, REG0B);
    return handle.reg0B.refined.ABCD_E;
}

/**
 * @ingroup RDA_API
 * @brief Set volume down on RDA chip
 * @param I2Cx I2C Port
 * @return uint8_t
 */
uint8_t RDA_GetErrorBlockB(I2C_HandleTypeDef *hi2c)
{
 	getStatus(hi2c, REG0B);
    return handle.reg0B.refined.BLERB;
}

/**
 * @ingroup RDA_API
 * @brief Get RDS info state on RDA chip
 * @param I2Cx I2C Port
 * @return TRUE/FALSE
 */
BOOL RDA_GetRDSInfoState(I2C_HandleTypeDef *hi2c)
{
	getStatus(hi2c, REG0B);
    return(handle.reg0A.refined.RDSS && handle.reg0B.refined.ABCD_E == 0 && handle.reg0B.refined.BLERB == 0);
}

/**
 * @ingroup RDA_API
 * @brief Set RDS FIFO on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetRDSFifo(I2C_HandleTypeDef *hi2c, BOOL value)
{
    handle.reg04.refined.RDS_FIFO_EN = value;
    registerWrite(hi2c, REG04, handle.reg04.raw);
}

/**
 * @ingroup RDA_API
 * @brief Call clear RDS FIFO on RDA chip
 * @param I2Cx I2C Port
 */
void RDA_ClearRDSFifo(I2C_HandleTypeDef *hi2c)
{
    handle.reg04.refined.RDS_FIFO_CLR = 1;
    registerWrite(hi2c, REG04, handle.reg04.raw);
}

// Déclaration de la variable globale (initialisée par défaut à "Station ")
char rda_station_name[9] = "Station ";

/**
 * @brief Remet le nom par défaut lors d'un changement de fréquence
 */
void RDA_ResetRDSName(void)
{
    rda_station_name[0] = 'S'; rda_station_name[1] = 't'; rda_station_name[2] = 'a';
    rda_station_name[3] = 't'; rda_station_name[4] = 'i'; rda_station_name[5] = 'o';
    rda_station_name[6] = 'n'; rda_station_name[7] = ' '; rda_station_name[8] = '\0';
}

/**
 * @brief Machine d'états pour assembler le nom de la station
 */
void RDA_UpdateRDSName(I2C_HandleTypeDef *hi2c)
{
    // 1. On vérifie si un nouveau groupe de données RDS est prêt dans le registre 0x0A
    getStatus(hi2c, REG0A);
    if (handle.reg0A.refined.RDSR == 1)
    {
        // 2. On charge les registres utiles : Bloc de Statut (0B), Bloc B (0D) et Bloc D (0F)
        getStatus(hi2c, REG0B);
        getStatus(hi2c, REG0D);
        getStatus(hi2c, REG0F);

        // 3. On ignore la trame si le taux d'erreur (BLERB) est trop élevé
        if (handle.reg0B.refined.BLERB < 2)
        {
            uint16_t blockB = handle.reg0D.RDSB;
            uint16_t groupType = blockB >> 11; // Extraction des 5 bits de poids fort (Type de groupe)

            // 4. Le nom de la station est diffusé dans les Groupes de type "0" (0A ou 0B)
            if ((groupType >> 1) == 0)
            {
                // L'adresse de l'emplacement (0 à 3) se trouve dans les 2 derniers bits du Bloc B
                uint8_t textSegment = blockB & 0x0003;

                // Les 2 lettres se trouvent dans le Bloc D
                uint16_t blockD = handle.reg0F.RDSD;
                char char1 = (blockD >> 8) & 0xFF;
                char char2 = blockD & 0xFF;

                // 5. Filtrage ASCII pour éviter les caractères corrompus (parasites radio)
                if (char1 >= 0x20 && char1 <= 0x7E) rda_station_name[textSegment * 2] = char1;
                if (char2 >= 0x20 && char2 <= 0x7E) rda_station_name[textSegment * 2 + 1] = char2;
            }
        }
    }
}
