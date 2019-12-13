// Analog accelerometer app
//
// Reads data from the ADXL327 analog accelerometer

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>


#include "app_error.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_serial.h"
#include "nrfx_gpiote.h"
#include "nrfx_saadc.h"

#include "buckler.h"
#include "display.h"
#include "bike_break.h"

// LED and button array
//static uint8_t LEDS[2] = {BUCKLER_LED0, BUCKLER_LED1};
static uint8_t LEDS[2] = {NRF_GPIO_PIN_MAP(0, 3), NRF_GPIO_PIN_MAP(0, 4)};
static uint8_t BUTTONS[2] = {NRF_GPIO_PIN_MAP(0, 28), NRF_GPIO_PIN_MAP(0, 29)};
static bool turned = false;
 
// I2C manager
NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

// handler called whenever an input pin BUTTON changes
void pin_change_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  switch(pin) {
    case NRF_GPIO_PIN_MAP(0, 28): {
      if (!nrfx_gpiote_in_is_set(BUTTONS[0])) {
        if(nrf_gpio_pin_out_read(LEDS[0]) == 1) {
          nrfx_gpiote_out_clear(LEDS[0]);
        } else {
          nrfx_gpiote_out_set(LEDS[0]);
        }
      }
      break;
    }

    case NRF_GPIO_PIN_MAP(0, 29): {
      if (!nrfx_gpiote_in_is_set(BUTTONS[1])) {
        if(nrf_gpio_pin_out_read(LEDS[1]) == 1) {
          nrfx_gpiote_out_clear(LEDS[1]);
        } else {
          nrfx_gpiote_out_set(LEDS[1]);
        }
      }
      break;
    }
  }
}

int main (void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();

  // initialize GPIO driver
  if (!nrfx_gpiote_is_init()) {
    error_code = nrfx_gpiote_init();
  }
  APP_ERROR_CHECK(error_code);

  // configure leds
  // manually-controlled (simple) output, initially set
  nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
  for (int i=0; i<2; i++) {
    error_code = nrfx_gpiote_out_init(LEDS[i], &out_config);
    APP_ERROR_CHECK(error_code);
    nrfx_gpiote_out_clear(LEDS[i]);
  }

   // initialize display
  nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(1);
  nrf_drv_spi_config_t spi_config = {
    .sck_pin = BUCKLER_LCD_SCLK,
    .mosi_pin = BUCKLER_LCD_MOSI,
    .miso_pin = BUCKLER_LCD_MISO,
    .ss_pin = BUCKLER_LCD_CS,
    .irq_priority = NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY,
    .orc = 0,
    .frequency = NRF_DRV_SPI_FREQ_4M,
    .mode = NRF_DRV_SPI_MODE_2,
    .bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST
  };
  error_code = nrf_drv_spi_init(&spi_instance, &spi_config, NULL, NULL);
  APP_ERROR_CHECK(error_code);
  display_init(&spi_instance);
  display_write("Bike State", DISPLAY_LINE_0);
  printf("Display initialized!\n");

  // initialize i2c master (two wire interface)
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = BUCKLER_SENSORS_SCL;
  i2c_config.sda = BUCKLER_SENSORS_SDA;
  i2c_config.frequency = NRF_TWIM_FREQ_100K;
  error_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
  mpu9250_init(&twi_mngr_instance);
  APP_ERROR_CHECK(error_code);

  //init bike state
  error_code = init_bike_state(3, 70, 5);
  APP_ERROR_CHECK(error_code);
  printf("IMU andd Bike State initialized!\n");

  // configure two gpio buttons
  // input pin, trigger on releasing
  nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
  in_config.pull = NRF_GPIO_PIN_NOPULL;
  error_code = nrfx_gpiote_in_init(BUTTONS[0], &in_config, pin_change_handler);
  nrfx_gpiote_in_event_enable(BUTTONS[0], true);

  in_config.pull = NRF_GPIO_PIN_NOPULL;
  error_code = nrfx_gpiote_in_init(BUTTONS[1], &in_config, pin_change_handler);
  nrfx_gpiote_in_event_enable(BUTTONS[1], true);

  // initialization complete
  printf("Buckler initialized!\n");

  // loop forever
  while (1) {
  	int x = read_bike_state();
    
    if(x >= 10) {
      display_write("BREAK", DISPLAY_LINE_0);
    } else {
      display_write("", DISPLAY_LINE_0);
    }
    if(x % 10 == 0){
      display_write("", DISPLAY_LINE_1);
      if(turned){
        nrfx_gpiote_out_clear(LEDS[0]);
        nrfx_gpiote_out_clear(LEDS[1]);
      }
      turned = false;
    } else if (x % 10 == 1){
      display_write("LEFT", DISPLAY_LINE_1);
      turned = true;
    } else if (x % 10 == 2){
      display_write("RIGHT", DISPLAY_LINE_1);
      turned = true;
    }
    
    nrf_delay_ms(10);
  }        
}