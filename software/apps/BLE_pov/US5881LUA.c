// Virtual timers
//
// Uses a single hardware timer to create an unlimited supply of virtual
//  software timers

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "app_timer.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_serial.h"

#include "buckler.h"
#include "US5881LUA.h"


void virtual_timer_init(void) {
  //timer initialization code here

  NRF_TIMER4->BITMODE = 3; 
  NRF_TIMER4->PRESCALER = 1; 
  NRF_TIMER4->TASKS_CLEAR = 1;
  NRF_TIMER4->TASKS_START = 1;  
  NRF_TIMER4->INTENSET|=1<<16;
  NRF_TIMER4->CC[0] = 0xFFFE;
  NVIC_EnableIRQ(TIMER4_IRQn);
  NVIC_SetPriority(TIMER4_IRQn,5);
}

uint32_t read_timer(void) {
    // Should return the value of the internal counter for TIMER4
    NRF_TIMER4->TASKS_CAPTURE[1] = 1;
    return NRF_TIMER4->CC[1];
}


void setup(void){
    degreeWidth = 0;//time to rotate 1 degree
    degreeCount = 0;//current degree passed by
    // revolutions = 0;
    oldTime = read_timer();
    rpm = 0;
    originalRPM = 0;
    picWidth=360;//36*35
}


// void loop(void){
//   if(revolutions>=2){//1 revolution
//       printf("old time:%ld\n",oldTime);
//       rpm = 30*1000000*revolutions/(read_timer() - oldTime);
//       degreeWidth = (read_timer() - oldTime)/(picWidth*revolutions);
//       printf("total time for one revolution:%ld\n",(read_timer()-oldTime)/revolutions);
//       degreeCount = 0;
//       NRF_TIMER4->TASKS_CLEAR = 1;
//       NRF_TIMER4->TASKS_START = 1;
//       oldTime = read_timer();//clear timer
//       revolutions = 0;
//       NRF_TIMER4->CC[0]=degreeWidth-1;
//       printf("RPM: %ld\n", rpm);
//     }
// }



