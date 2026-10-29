/*
 * 土壤湿度传感器（ADC）- 暂用模拟数据
 */
#include "soil_sensor.h"
#include "osal_debug.h"
#include <stdio.h>

void soil_sensor_init(void)
{
    printf("[SOIL] init success (模拟模式)\r\n");
}

float soil_sensor_read(void)
{
    static float mock_value = 45.0f;
    // 模拟数据，手动修改测试
    printf("[SOIL] mock humidity=%.2f%%\r\n", mock_value);
    return mock_value;
}