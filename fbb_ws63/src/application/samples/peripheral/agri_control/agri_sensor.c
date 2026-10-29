//传感器采集（只上报，不自动控制）
#include "agri_sensor.h"
#include <stdlib.h>

int agri_sensor_init(void)
{
    // 以后硬件到了这里加BH1750/ADC初始化
    return 0;
}

// 传感器采集（自动上报）
int agri_sensor_read(AgriSensor *s)
{
    if (!s) return -1;

    // 空气温湿度（学姐原有可替换）
    s->air_temp = 22.5f + (rand() % 50) / 10.0f;
    s->air_humi = 50.0f + (rand() % 300) / 10.0f;

    // 光照模拟
    s->light = 500 + rand() % 9500;

    // 土壤湿度模拟
    s->soil = 25.0f + (rand() % 400) / 10.0f;

    return 0;
}