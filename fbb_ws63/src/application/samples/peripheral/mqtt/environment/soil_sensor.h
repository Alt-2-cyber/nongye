/*
 * 土壤湿度传感器（ADC）
 */

#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

// 初始化土壤湿度传感器
void soil_sensor_init(void);

// 读取土壤湿度，返回百分比（0~100%）
float soil_sensor_read(void);

#endif