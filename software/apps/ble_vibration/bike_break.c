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

static mpu9250_measurement_t rotated_measurement;
static mpu9250_measurement_t acc_measurement;
static mpu9250_measurement_t gyro_measurement;
static quaternion_t quaternion;
static float prev_y = 0.01;
static int samples_s = 5;
static float break_sensitivity_s = 50;
static float turn_sensitivity_s = 20;
static const float acc_threshold = 0.2;
static const float still_threshold = 0.05;
  
  void convert_angle_to_quaternion(mpu9250_measurement_t angle){
    float yaw = angle.z_axis * 3.1415 / 180;
    float pitch = angle.y_axis * 3.1415 / 180;
    float roll = angle.x_axis * 3.1415 / 180;
    float cy = cos(yaw / 2);
    float sy = sin(yaw / 2);
    float cp = cos(pitch / 2);
    float sp = sin(pitch / 2);
    float cr = cos(roll / 2);
    float sr = sin(roll / 2);
    quaternion.w = cy * cp * cr - sy * sp * sr;
    quaternion.x = cy * cp * sr + sy * sp * cr;
    quaternion.z = sy * cp * cr + cy * sp * sr;
    quaternion.y = cy * sp * cr - sy * cp * sr;
  }


  mpu9250_measurement_t rotate_axes(mpu9250_measurement_t acc, quaternion_t q){
    float xx = q.x * q.x;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float xw = q.x * q.w;

    float yy = q.y * q.y;
    float yz = q.y * q.z;
    float yw = q.y * q.w;

    float zz = q.z * q.z;
    float zw = q.z * q.w;

    float r00 = 1 - 2 * (yy + zz);
    float r01 = 2 * (xy - zw);
    float r02 = 2 * (xz + yw);
    float r10 = 2 * (xy + zw);
    float r11 = 1 - 2 * (xx + zz);
    float r12 = 2 * (yz - xw);
    float r20 = 2 * (xz - yw);
    float r21 = 2 * (yz + xw);
    float r22 = 1 - 2 * (xx + yy);

    /*
    printf("rotateion matrix\n");
    printf("%.6f    ", r00);
    printf("%.6f    ", r01);
    printf("%.6f    \n", r02);

    printf("%.6f    ", r10);
    printf("%.6f    ", r11);
    printf("%.6f    \n", r12);

    printf("%.6f    ", r20);
    printf("%.6f    ", r21);
    printf("%.6f    \n", r22);
    */

    float x = r00 * acc.x_axis + r01 * acc.y_axis + r02 * acc.z_axis;
    float y = r10 * acc.x_axis + r11 * acc.y_axis + r12 * acc.z_axis;
    float z = r20 * acc.x_axis + r21 * acc.y_axis + r22 * acc.z_axis;
    rotated_measurement.y_axis = y;
    rotated_measurement.z_axis = z - 1;
    rotated_measurement.x_axis = x;
    return rotated_measurement;
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

  bool is_still(mpu9250_measurement_t acc){
    if(acc.x_axis > still_threshold || acc.x_axis < -still_threshold || 
       acc.y_axis > still_threshold || acc.y_axis < -still_threshold || 
       acc.z_axis-1 > still_threshold || acc.z_axis-1 < -still_threshold){
        return false;
    } else {
        return true;
    }
  }

  //return 10 for break
  //return 00 for forward, 01 for left, 02 for right
  int read_bike_state(){
    acc_measurement = read_smoothed();

    if(is_still(acc_measurement)){
        mpu9250_stop_gyro_integration();
        mpu9250_start_gyro_integration();
    }
    gyro_measurement = mpu9250_read_gyro_integration();
    convert_angle_to_quaternion(gyro_measurement);
    rotate_axes(acc_measurement, quaternion);
    //float diff = (rotated_measurement.y_axis - prev_y) / acc_threshold * 100;
    //bool isDecrease = rotated_measurement.y_axis < prev_y;
    prev_y = rotated_measurement.y_axis;
    
    /*
    printf("gyro\n");
    printf("%.6f    ", gyro_measurement.x_axis);
    printf("%.6f    ", gyro_measurement.y_axis);
    printf("%.6f    \n", gyro_measurement.z_axis);
    
    printf("q\n");
    printf("%.6f    ", quaternion.w);
    printf("%.6f    ", quaternion.x);
    printf("%.6f    ", quaternion.y);
    printf("%.6f    \n", quaternion.z);
    
    printf("acc\n");
    printf("%.6f    ", acc_measurement.x_axis);
    printf("%.6f    ", acc_measurement.y_axis);
    printf("%.6f    \n", acc_measurement.z_axis);
    
    printf("rotated\n");
    printf("%.6f    \n", rotated_measurement.x_axis);
    printf("%.6f    \n", rotated_measurement.y_axis);
    printf("%.6f    \n", rotated_measurement.z_axis);
    */

    int state = 0;
    if(rotated_measurement.y_axis < -acc_threshold / 100.0 * break_sensitivity_s){
    //if(isDecrease && abs(diff) > abs(break_sensitivity_s)){
        state += 10;
    }
    if(gyro_measurement.y_axis > turn_sensitivity_s){
        state += 2;
    } else if(gyro_measurement.y_axis < -turn_sensitivity_s){
        state += 1;
    }
    return state;
  }

  ret_code_t init_bike_state(int samples, float break_sensitivity, float turn_sensitivity){
  	if(break_sensitivity < 10){
  		break_sensitivity = 10;
  	}
  	if(samples < 1){
  		samples = 1;
  	}
    if(turn_sensitivity < 5){
        turn_sensitivity = 5;
    }

  	prev_y = 0;
    samples_s = samples;
    break_sensitivity_s = break_sensitivity;
    turn_sensitivity_s = turn_sensitivity;
    //mpu9250_calibrate_gyro(samples * 4);
    mpu9250_calibrate_accel(samples * 4);
    mpu9250_stop_gyro_integration();
    ret_code_t error_code = mpu9250_start_gyro_integration();
    return error_code;
  }
