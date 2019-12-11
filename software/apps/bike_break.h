
#pragma once

#include "mpu9250.h"

typedef struct {
  float w;
  float x;
  float y;
  float z;
} quaternion_t;

//read the integrated angle and calculate it to quaternion system
//
//the quaternion coordinate is already initialized
void convert_angle_to_quaternion(mpu9250_measurement_t angle);

//read the 3 axies from accelerometer and the 4 points from quaternion
//rotate the axies of acceleration to remove gravity factor
//
//return the rotated acceleration in float point
mpu9250_measurement_t rotate_axes(mpu9250_measurement_t acc, quaternion_t q);

//read the 3 axies from accelerometer samples_s number of times
//and calculate the average acceleration
//
//return the average acceleration of 3 axies in float points
mpu9250_measurement_t read_smoothed();

//use gyroscope, accelerometer, and quaternion to calcuate the forward
//acceleration without gravity factor, and find the bike state
//
//return 0 if bike is breaking, otherwise return 1
int read_bike_state();

//read the number of samples and sensitivity, and initialze gyroscope
//
//return ret error code
ret_code_t init_bike_state(int samples, int sensitivity);
