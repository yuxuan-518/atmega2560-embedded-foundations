#include "mpu6050.h"

uint8_t mpu6050_config_value(uint8_t dlpf)
{
    return dlpf & 0x07;
}

uint8_t mpu6050_accel_config_value(uint8_t fs)
{
    return (uint8_t)((fs & 0x03) << 3);
}

uint16_t mpu6050_accel_lsb_per_g(uint8_t fs)
{
    return (uint16_t)(16384U >> (fs & 0x03));
}

uint8_t mpu6050_accel_range_g(uint8_t fs)
{
    return (uint8_t)(2U << (fs & 0x03));
}

int16_t mpu6050_be16(const uint8_t *bytes)
{
    uint16_t value = ((uint16_t)bytes[0] << 8) | bytes[1];
    return (int16_t)value;
}

void mpu6050_parse_accel(const uint8_t *bytes, int16_t accel[3])
{
    for(uint8_t i = 0; i < 3; i++){
        accel[i] = mpu6050_be16(&bytes[i * 2]);
    }
}

int32_t mpu6050_accel_mg(int16_t raw, uint8_t fs)
{
    int32_t lsb = mpu6050_accel_lsb_per_g(fs);
    int32_t scaled = (int32_t)raw * 1000L;

    if(scaled >= 0){
        return (scaled + lsb / 2) / lsb;
    }
    return (scaled - lsb / 2) / lsb;
}

bool mpu6050_check_config(const uint8_t *config3, uint8_t dlpf, uint8_t fs)
{
    if((config3[0] & 0x07) != mpu6050_config_value(dlpf)){
        return false;
    }
    return (config3[2] & 0x18) == mpu6050_accel_config_value(fs);
}
