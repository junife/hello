/*
*********************************************************************************************************

* Copyright 2008 The Watt Stopper / Legrand
* as an unpublished work.
* All Rights Reserved.
*
* The information contained herein is confidential
* property of The Watt Stopper / Legrand.
* The use, copying, transfer or disclosure of such
* information is prohibited except by express written
* agreement with The Watt Stopper / Legrand.
*
* Project:PW/DSW 31x
* MCU:Atmega168P
* Compile condition: AVR Studio 6
* First written on March 2015, by Corner Hu.
*
* Module Description:
* This file include C enter function "main function",main call all initialize functions.Call core function
* and drivers function in each power cycles in main loop
*********************************************************************************************************
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "button.h"
#include "io_cfg.h"
#include "global_x.h"
#include "avr_lib.h"
#include  "three_way.h"
#include  "led.h"
#include  "dimmer.h"
#include "eeprom_x.h"
#include  <avr/wdt.h>
#include  "appl.h"
#include "VoltageDetect.h"
extern uint16_t ADC_Data[8];
void ConfigureWatchdogTimer( unsigned char newWDTCSRValue );

int main(void)
{
	//OSCCAL =  pgm_read_byte(0x3fff);
	MCU_Init();
	Appl_Init();
	
#if   (BAUD > 0)
    uartInit();
    uartSetBaudRate(BAUD);
    rprintfInit(uartSendByte);
	rprintf("printf\n");
#endif

#if (BAUD == 0)	/* printf active for debug firmware, MCU can't sleep */	
	/* Set up watchdog for interrupt-and-system-reset mode, with a timeout of 32K cycles = 0.25S */
    ConfigureWatchdogTimer(WDTO_250MS);
#endif

	sei();
	
	while(1)
    {
		WaitZeroCross();		
		TW_CycleUpdate();
        RelayCycleUpdate();
		DimmerCycleUpdate();
		DipCycleUpdate();
		abc();
		
#if 0		
		UltraCycleUpdate();
		func();
		VolDetCycleUpdate();
#endif		
    }
}

/*
*********************************************************************************************************
*                                         PwrLossFun
*
* Description : This function save data to eeprom and process power loss function
*
* Arguments   : none
*
* Returns    : none
*********************************************************************************************************
*/
void PwrLossFun(void)
{
	cli();									/* Global interrupt disable */
	
	eepromSave();
	
	//ioRED_LED	= 0;		/* Trun on two LEDs speed up power consume */
	//ioGREEN_LED = 0;

	if(appl.AddFeature[1].ServiceM == DIP_OFF){	/* If service mode disable will turn on/off load according to Power Loss Mode Setting */
		/* Power Loss Mode process */
		if(appl.AddFeature[1].PwrLossMode == FAIL_ON){
			ioRELAY_ON	 = 1;		/* turn on load after power off  */
		}else if(appl.AddFeature[1].PwrLossMode == FAIL_OFF){
			ioRELAY_OFF   = 1;		/* turn off load after power off  */
		}else{						/* keep last state */
			/* See eepromSave function, dimmer.levelCurH store in eeprom */
		}
	}
	while(1){
	}
}

/*
*********************************************************************************************************
*                                         WATCGDOG INTERRUPT ISR
*
* Description : Exteranl interrupt is triggered by zero-crossing signal.
*
* Arguments   : none
*
* Returns    : none
*********************************************************************************************************
*/

ISR (WDT_vect)
{
	PwrLossFun();
}


/*
 | ConfigureWatchdogTimer
 |
 | This has been made a separate subroutine to prevent cross-call optimization from
 | inserting instructions in between the setting of the WDCE bit and the subsequent
 | secondary write of the WDTCSR register, as this sequence must occur within 4 machine
 | cycles.
 */
void ConfigureWatchdogTimer(uint8_t newWDTCSRValue)
{
	/* Start timed equence */
	uint8_t reg = SREG;
	cli();
	wdt_reset();
	WDTCSR = (1 << WDCE) | (1 << WDE);
	SREG= reg;
	WDTCSR = (1 << WDIF) | (1<<WDIE) | (1<<WDE) | newWDTCSRValue;		/* Interrupt and system reset mode */
}


