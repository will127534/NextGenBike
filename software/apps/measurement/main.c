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
#include "bike_break.h"



 
// I2C manager
NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

int main (void) {
  ret_code_t error_code = NRF_SUCCESS;

  // initialize RTT library
  error_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(error_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();

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
  error_code = init_bike_state(5, 20);
  APP_ERROR_CHECK(error_code);

  // initialization complete
  printf("Buckler initialized!\n");

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
	printf("%d\n", x);
    nrf_delay_ms(10);
  }        
}