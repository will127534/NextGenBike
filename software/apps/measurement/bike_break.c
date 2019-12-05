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

mpu9250_measurement_t init_measurement;
mpu9250_measurement_t acc_measurement;
mpu9250_measurement_t gyro_measurement;
quaternion_t quaternion;
float prev_x = 0;
int samples_s = 5;
int sensitivity_s = 50;
  
  void convert_angle_to_quaternion(mpu9250_measurement_t angle){
    float yaw = angle.z_axis / 180 * 3.1415;
    float pitch = angle.y_axis / 180 * 3.1415;
    float roll = angle.x_axis / 180 * 3.1415;
    float cy = cos(yaw / 2);
    float sy = sin(yaw / 2);
    float cp = cos(pitch / 2);
    float sp = sin(pitch / 2);
    float cr = cos(roll / 2);
    float sr = sin(roll / 2);
    quaternion.w = cy * cp * cr + sy * sp * sr;
    quaternion.x = cy * cp * sr - sy * sp * cr;
    quaternion.y = sy * cp * sr + cy * sp * cr;
    quaternion.z = sy * cp * cr - cy * sp * sr;
  }


  float rotate_axes(mpu9250_measurement_t acc, quaternion_t q){
    float r00 = 1 - 2 * (q.y * q.y + q.z * q.z);
    float r01 = 2 * (q.x * q.y - q.z * q.w);
    float r02 = 2 * (q.x * q.z + q.y * q.w);
    /*
    float r10 = 2 * (q.x * q.y + q.z * q.w);
    float r11 = 1 - 2 * (q.x * q.x + q.z * q.z);
    float r12 = 2 * (q.y * q.z - q.x * q.w);
    float r20 = 2 * (q.x * q.z - q.y * q.w);
    float r21 = 2 * (q.y * q.z + q.x * q.w);
    float r22 = 1 - 2 * (q.x * q.x + q.y * q.y);
    */
    float x = r00 * acc.x_axis + r01 * acc.y_axis + r02 * acc.z_axis;
    /*
    float y = r10 * acc.x_axis + r11 * acc.y_axis + r12 * acc.z_axis;
    float z = r20 * acc.x_axis + r21 * acc.y_axis + r22 * acc.z_axis;
    acc.y_axis = y;
    acc.z_axis = z - 1;
    acc.x_axis = x;
    */
    return x;
  }

  mpu9250_measurement_t read_smoothed(){
    acc_measurement.x_axis = 0;
    acc_measurement.y_axis = 0;
    acc_measurement.z_axis = 0;
    mpu9250_measurement_t measurement;
    for(int i = 0; i<samples_s; i++){
      measurement = mpu9250_read_accelerometer();
      acc_measurement.x_axis += measurement.x_axis;
      acc_measurement.y_axis += measurement.y_axis;
      acc_measurement.z_axis += measurement.z_axis;
    }
    acc_measurement.x_axis = acc_measurement.x_axis / samples_s;
    acc_measurement.y_axis = acc_measurement.y_axis / samples_s;
    acc_measurement.z_axis = acc_measurement.z_axis / samples_s;
    return acc_measurement;
  }

  //return 0 for break, otherwise return 1
  int read_bike_state(){
    acc_measurement = read_smoothed();
    gyro_measurement = mpu9250_read_gyro_integration();
    convert_angle_to_quaternion(gyro_measurement);
    float x = rotate_axes(acc_measurement, quaternion);
    float diff = (prev_x - x) / x * 100;
    prev_x = x;
 
    if(diff < -sensitivity_s){
    	return 0;
    } else {
    	return 1;
    }
  }

  ret_code_t init_bike_state(int samples, int sensitivity){
  	if(sensitivity < 10){
  		sensitivity = 10;
  	}
  	if(samples < 1){
  		samples = 1;
  	}
  	prev_x = 0;
    samples_s = samples;
    sensitivity_s = sensitivity;
    init_measurement = read_smoothed();
    mpu9250_stop_gyro_integration();
    ret_code_t error_code = mpu9250_start_gyro_integration();
    return error_code;
  }
