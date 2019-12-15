#pragma once

void software_interrupt_init(void) {
    NRF_EGU1->INTENSET = 0x1;
    NVIC_EnableIRQ(SWI1_EGU1_IRQn);
}

void software_interrupt_generate(void) {
    NRF_EGU1->TASKS_TRIGGER[0] = 1;
}

void timer_init(void) {
  //timer initialization code here

  NRF_TIMER4->BITMODE = 3; 
  NRF_TIMER4->PRESCALER = 0xF; 
  NRF_TIMER4->TASKS_CLEAR = 1;
  NRF_TIMER4->TASKS_STOP = 1;  
  NRF_TIMER4->INTENSET|=1<<16;
  NRF_TIMER4->CC[0] = 0xF240;
  NVIC_EnableIRQ(TIMER4_IRQn);
  NVIC_SetPriority(TIMER4_IRQn,5);
}

void timer_start(void){
	NRF_TIMER4->CC[0] = 0xF240;
	NRF_TIMER4->TASKS_START = 1;  
}

void timer_stop(void){
	NRF_TIMER4->TASKS_STOP = 1;  
}