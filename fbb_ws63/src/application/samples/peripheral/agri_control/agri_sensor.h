#ifndef AGRI_SENSOR_H
#define AGRI_SENSOR_H

// 传感器数据结构（你要的4类全部齐全）
typedef struct {
    // 1. 土壤温湿度
    float soil_temp;      // 土壤温度
    float soil_humi;      // 土壤湿度

    // 2. 空气温湿度
    float air_temp;       // 空气温度
    float air_humi;       // 空气湿度

    // 3. 光照强度
    float light;

    // 4. 二氧化碳浓度
    float co2;
} AgriSensor;

// 初始化（模拟版）
void agri_sensor_init(void);

// 读取所有传感器（模拟随机数）
void agri_sensor_read(AgriSensor *s);

#endif