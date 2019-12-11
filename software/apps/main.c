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

// LED array
static uint8_t LEDS[3] = {BUCKLER_LED0, BUCKLER_LED1, BUCKLER_LED2};

 
// I2C manager
NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

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
  for (int i=0; i<3; i++) {
    error_code = nrfx_gpiote_out_init(LEDS[i], &out_config);
    APP_ERROR_CHECK(error_code);
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
  APP_ERROR_CHECK(error_code);
  mpu9250_init(&twi_mngr_instance);
  printf("IMU initialized!\n");

  //init bike state
  error_code = init_bike_state(5, 80);
  APP_ERROR_CHECK(error_code);

  // initialization complete
  printf("Buckler initialized!\n");

  int x_prev = 1;
  // loop forever
  while (1) {
  	/*
    acc_measurement = read_smoothed(5);
    gyro_measurement = mpu9250_read_gyro_integration();
    quaternion = convert_angle_to_quaternion(gyro_measurement);
    acc_measurement = rotate_axes(acc_measurement, quaternion);
    // display results
    printf("%.6f    ", acc_measurement.x_axis);
    printf("%.6f    ", acc_measurement.y_axis);
    printf("%.6f    \n", acc_measurement.z_axis);
    //printf("%.6f    ", gyro_measurement.x_axis);
    //printf("%.6f    ", gyro_measurement.y_axis);
    //printf("%.6f    \n", gyro_measurement.z_axis);
    printf("%.6f    ", quaternion.x);
    printf("%.6f    ", quaternion.y);
    printf("%.6f    ", quaternion.z);
    printf("%.6f    \n", quaternion.w);
	*/
  	int x = read_bike_state();
    //display_write("abc", DISPLAY_LINE_1);
    if(x != x_prev) {
      for (int i=0; i<3; i++) {
        nrf_gpio_pin_toggle(LEDS[i]);
      }
      if(x == 0){
          display_write("BREAK", DISPLAY_LINE_1);
      } else {
          display_write("FORWARD", DISPLAY_LINE_1);
      }
      x_prev = x;
    }
    nrf_delay_ms(50);
    }        
}