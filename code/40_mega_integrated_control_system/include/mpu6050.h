#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include <stdbool.h>

#define MPU6050_ADDRESS_AD0_LOW 0x68
#define MPU6050_ADDRESS_AD0_HIGH 0x69

#define MPU6050_REG_CONFIG 0x1A
#define MPU6050_REG_GYRO_CONFIG 0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_WHO_AM_I 0x75

#define MPU6050_WHO_AM_I_VALUE 0x68
#define MPU6050_PWR_RESET 0x80
#define MPU6050_PWR_CLOCK_PLL_X 0x01
#define MPU6050_ACCEL_BYTES 6

uint8_t mpu6050_config_value(uint8_t dlpf);
uint8_t mpu6050_accel_config_value(uint8_t fs);
uint16_t mpu6050_accel_lsb_per_g(uint8_t fs);
uint8_t mpu6050_accel_range_g(uint8_t fs);
int16_t mpu6050_be16(const uint8_t *bytes);
void mpu6050_parse_accel(const uint8_t *bytes, int16_t accel[3]);
int32_t mpu6050_accel_mg(int16_t raw, uint8_t fs);
bool mpu6050_check_config(const uint8_t *config3, uint8_t dlpf, uint8_t fs);

#endif
