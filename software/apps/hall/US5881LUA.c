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
  NRF_TIMER4->PRESCALER = 4; 
  NRF_TIMER4->TASKS_CLEAR = 1;
  NRF_TIMER4->TASKS_START = 1;
}

uint32_t read_timer(void) {

    // Should return the value of the internal counter for TIMER4
    NRF_TIMER4->TASKS_CAPTURE[1] = 1;
    return NRF_TIMER4->CC[1];
}


void setup(void){
    revolutions = 0;
    oldTime = read_timer();
    rpm = 0;
}

void loop(void){
  if(revolutions>=20){
      printf("old time:%d\n",oldTime);
      rpm = 30*1000000*revolutions/(read_timer() - oldTime);
      oldTime = read_timer();
      printf("new time:%d\n",oldTime);
      revolutions = 0;
      printf("RPM: %d\n", rpm);
    }
}


