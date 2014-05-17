//****************************************************************************
//*                    MSP430 Popcorn Popper Coffee Roaster                  *
//****************************************************************************
//
//           module:  interrupts.c
//      description:  interrupts for Device
//  original author:  Andrew Schomin
//    creation date:  05-09-2014
//
//****************************************************************************

// === INCLUDES ==============================================================

#include "interrupts.h"

// === PRIVATE VARIABLES =====================================================

bool isLowBits = true;      //!< Used to tell interrupt what bits of the int
                            //!< Being transfered we are on
unsigned int combinedBits = 0;

// === GLOBAL INTERRUPTS =====================================================

//-----------------------------------------------------------------------------
//
//  PORT1_ISR
//! \brief   Description:  This is an interrupt for port1.
//
//  Entry:
//!   \param
//!   This routine does not have any input parameters.
///
//  Exit:
//!   \return NONE ( Does not return any values )
//-----------------------------------------------------------------------------
interrupt(PORT1_VECTOR) PORT1_ISR(void) {

  if((P1IFG & BTN) && !RoastStarted){

    LED_OUT |= LED1;

    P1IFG = P1IFG & ~BTN;


    dint();

    InitRoast();
    HostStarted = false;
    StartRoast();

    eint();

  }

} //PORT1_ISR

//-----------------------------------------------------------------------------
//
//  TIMERA0_ISR
//! \brief   Description:  This is an interrupt for Timer A0.
//
//  Entry:
//!   \param
//!   This routine does not have any input parameters.
///
//  Exit:
//!   \return NONE ( Does not return any values )
//-----------------------------------------------------------------------------
interrupt(TIMER0_A0_VECTOR) TIMERA0_ISR(void) {

  LED_OUT ^= LED1;

} //TIMERA0_ISR

//-----------------------------------------------------------------------------
//
//  USCI0RX_ISR
//! \brief   Description:  This is an interrupt for UART RX. It will interrupt
//!                        when a process is not happening
//
//  Entry:
//!   \param
//!   This routine does not have any input parameters.
///
//  Exit:
//!   \return NONE ( Does not return any values )
//-----------------------------------------------------------------------------
interrupt(USCIAB0RX_VECTOR) USCI0RX_ISR(void)
{
  unsigned char data = UCA0RXBUF;

  //UARTSendArray(&data, 1);

  if(CurveSaveStarted){

    if(isLowBits){

      combinedBits = (unsigned int)data;

      isLowBits = false;

    }
    else{

      combinedBits = (data << 8) | combinedBits;
      UARTSendArray(&combinedBits, 2);


      //UARTSendArray(&combinedBits, 2);

      if(combinedBits == 0xFFFF){

        CurveSaveStarted = false;
        //Enable the interrupt for TACCR0 match. We are now doing somethings
        //TACCTL0 = CCIE;

      }else{

        // Check if the current address is a multiple of 0x200 for eraseing
        if((unsigned int)(&CurrentCurvePoint)%(ERASE_BLOCK_SIZE) == 0){

          FlashErase(CurrentCurvePoint);
        }

        FlashProgram(CurrentCurvePoint++, combinedBits);

      }

      isLowBits = true;

    }

  }
  else{

    switch(data){
     // Host wants to save a new curve to flash
     case 's': {
       // Respond with k char
       UARTSendArray("k", 1);
       CurveSaveStarted = true;
       CurrentCurvePoint = (unsigned int *)CURVE_START_ADDRESS;
       //Disable the interrupt for TACCR0 match. We are now doing somethings
       //TACCTL0 = ~(CCIE);
       break;
     }
     // Host wants to get a roast curve
     case 'g': {
       dint();
       // Respond with k char
       UARTSendArray("k", 1);
       GetRoastCurve();
       eint();
     }
     // Host wants to start the roast
     case 'r': {
       // Respond with k char
       UARTSendArray("k", 1);
       dint();
       InitRoast();
       //Disable the interrupt for TACCR0 match. We are now doing somethings
       TACCTL0 = ~(CCIE);
       HostStarted = false;
       StartRoast();
       eint();
     }
     default: {
       UARTSendArray("Unknown Command: ", 17);
       UARTSendArray(&data, 1);
       UARTSendArray("\n\r", 2);
       break;
     }
     }
   }
} //USCI0RX_ISR
