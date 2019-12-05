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

void GPIOTE_IRQHandler(void) {
    NRF_GPIOTE->EVENTS_IN[0] = 0;
    printf("interrupt\n");
    // nrf_gpio_pin_toggle(BUCKLER_LED0);
    revolutions++;
}

int main(void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  printf("Log initialized!\n");
  NVIC_EnableIRQ(GPIOTE_IRQn);
  //NVIC_SetPriority(GPIOTE_IRQ0);

  NRF_GPIOTE->CONFIG[0] |= 0x1C00; // input
  NRF_GPIOTE->CONFIG[0] |= 0x30000; // toggle
  NRF_GPIOTE->CONFIG[0] |= 1;//event mode
  NRF_GPIOTE->INTENSET |=1;//interrupt

  nrf_gpio_cfg_input(NRF_GPIO_PIN_MAP(0,28),NRF_GPIO_PIN_PULLUP);

  // You can use the NRF GPIO library to test your interrupt
  //nrf_gpio_pin_dir_set(BUCKLER_LED0, NRF_GPIO_PIN_DIR_OUTPUT);

  nrf_delay_ms(1000); 

  // Don't forget to initialize your timer library
  virtual_timer_init();
  setup();

  // loop forever
   printf("Time: %d\n", oldTime);
  while (1) {
    //nrf_gpio_pin_write(BUCKLER_LED0,nrf_gpio_pin_read(NRF_GPIO_PIN_MAP(0,28)));
    loop();
    
  }
}