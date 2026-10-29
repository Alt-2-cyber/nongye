/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pinctrl.h"
#include "i2c.h"
#include "osal_debug.h"
#include "ssd1306_fonts.h"
#include "ssd1306.h"
#include "soc_osal.h"
#include "aht20.h"
#include "app_init.h"

#define CONFIG_I2C_SCL_MASTER_PIN 15
#define CONFIG_I2C_SDA_MASTER_PIN 16
#define CONFIG_I2C_MASTER_PIN_MODE 2
#define I2C_MASTER_ADDR 0x0
#define I2C_SLAVE1_ADDR 0x38
#define I2C_SET_BANDRATE 400000
#define I2C_TASK_STACK_SIZE 0x1000
#define I2C_TASK_PRIO 17

const unsigned char headSize[] = {64, 64};

void app_i2c_init_pin(void)
{
    uapi_pin_set_mode(CONFIG_I2C_SCL_MASTER_PIN, CONFIG_I2C_MASTER_PIN_MODE);
    uapi_pin_set_mode(CONFIG_I2C_SDA_MASTER_PIN, CONFIG_I2C_MASTER_PIN_MODE);
}

/**
 * 汉字字模在线： https://www.23bei.com/tool-223.html
 * 数据排列：从左到右从上到下
 * 取模方式：横向8位左高位
 * 字体总类：[HZK1616宋体]
 **/
void TempHumChinese(void)
{
    const uint32_t W = 16;
    uint8_t fonts[][32] = {
        {/* -- ID:0,字符:"温",ASCII编码:CEC2,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节 */
         0x00, 0x08, 0x43, 0xFC, 0x32, 0x08, 0x12, 0x08, 0x83, 0xF8, 0x62, 0x08, 0x22, 0x08, 0x0B, 0xF8,
         0x10, 0x00, 0x27, 0xFC, 0xE4, 0xA4, 0x24, 0xA4, 0x24, 0xA4, 0x24, 0xA4, 0x2F, 0xFE, 0x20, 0x00},
        {/* -- ID:0,字符:"度",ASCII编码:B6C8,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节 */
         0x01, 0x00, 0x00, 0x84, 0x3F, 0xFE, 0x22, 0x20, 0x22, 0x28, 0x3F, 0xFC, 0x22, 0x20, 0x23, 0xE0,
         0x20, 0x00, 0x2F, 0xF0, 0x22, 0x20, 0x21, 0x40, 0x20, 0x80, 0x43, 0x60, 0x8C, 0x1E, 0x30, 0x04}};
    uint8_t fonts2[][32] = {
        {/* -- ID:0,字符:"湿",ASCII编码:CEC2,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节 */
         0x00, 0x08, 0x47, 0xFC, 0x34, 0x08, 0x14, 0x08, 0x87, 0xF8, 0x64, 0x08, 0x24, 0x08, 0x0F, 0xF8,
         0x11, 0x20, 0x21, 0x20, 0xE9, 0x24, 0x25, 0x28, 0x23, 0x30, 0x21, 0x24, 0x3F, 0xFE, 0x20, 0x00},
        {/* -- ID:0,字符:"度",ASCII编码:B6C8,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节 */
         0x01, 0x00, 0x00, 0x84, 0x3F, 0xFE, 0x22, 0x20, 0x22, 0x28, 0x3F, 0xFC, 0x22, 0x20, 0x23, 0xE0,
         0x20, 0x00, 0x2F, 0xF0, 0x22, 0x20, 0x21, 0x40, 0x20, 0x80, 0x43, 0x60, 0x8C, 0x1E, 0x30, 0x04}};
    for (size_t i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++) {
        ssd1306_DrawRegion(i * W, 3, W, fonts[i], sizeof(fonts[0])); // x轴坐标i*w，y轴坐标3，宽度为16
    }
    for (size_t j = 0; j < sizeof(fonts2) / sizeof(fonts2[0]); j++) {
        ssd1306_DrawRegion(j * W, 35, W, fonts2[j], sizeof(fonts2[0])); // x轴坐标i*w，y轴坐标35，宽度为16
    }
}

void Aht20TestTask(void)
{
    uint32_t retval = 0;

    //20260414 传感器变量定义
    float air_temp = 0.0f;//空气温度（真实数据）
    float air_humi = 0.0f;//空气湿度（真实数据）
    float soil_temp = 0.0f;//土壤温度（目前模拟）
    float soil_humi = 0.0f;//土壤湿度（目前模拟）
    int co2 = 450;//CO2浓度（模拟值：正常大气约400-500ppm）
    int light = 5000;//光照强度（模拟值：正常室内光线 单位：lux）

    //OLED显示用的字符缓冲区
    static char templine[32] = {0};
    static char humiline[32] = {0};
    //I2C总线配置参数
    uint32_t baudrate = I2C_SET_BANDRATE;
    uint32_t hscode = I2C_MASTER_ADDR;

    //各种硬件的初始化
    //配置I2C引脚（SCL和SDA）
    app_i2c_init_pin();
    //初始化I2C主机
    errcode_t ret = uapi_i2c_master_init(1, baudrate, hscode);
    if (ret != 0) {
        printf("i2c init failed, ret = %0x\r\n", ret);
    }
    //初始化OLED屏幕
    ssd1306_Init();
    ssd1306_Fill(Black);//清屏
    ssd1306_SetCursor(0, 0);//光标在左上角
    //初始化AHT20传感器（如果失败就每100ms重试一次）
    while (AHT20_Calibrate() != 0) {
        printf("AHT20 sensor init failed!\r\n");
        osal_mdelay(100); // 10ms在判断设备是否复位成功
    }

    //主循环
    while (1) {
        TempHumChinese();//在OLED上面显示汉字
        //读取空气温湿度
        retval = AHT20_StartMeasure();//测量
        printf("AHT20_StartMeasure: %d\r\n", retval);
        retval = AHT20_GetMeasureResult(&air_temp, &air_humi);//获取结果
        if (retval != 0) {
            printf("get humidity data failed!\r\n");
        }

        //20260414 读取土壤温湿度
        //用空气温湿度暂时代替土壤
        soil_temp = air_temp;
        soil_humi = air_humi;

        //20260414 读取co2和光强
        //给数据先写死了
        co2 = 450;
        light = 5000;

        //20260414 串口打印所有传感器数据
        printf("\r\n========== Sensor Data ==========\r\n");
        printf("air_temp（空气温度，真实）  : %.2f °C\r\n", air_temp);
        printf("air_humi（空气湿度，真实）  : %.2f %%\r\n", air_humi);
        printf("soil_temp（土壤温度，模拟） : %.2f °C\r\n", soil_temp);
        printf("soil_humi（土壤湿度，模拟） : %.2f %%\r\n", soil_humi);
        printf("co2（二氧化碳浓度，模拟）    : %d ppm\r\n", co2);
        printf("light（光照强度，模拟）     : %d lux\r\n", light);
        printf("==================================\r\n\r\n");

        //OLED显示空气温湿度
        //显示温度
        ssd1306_SetCursor(32, 8); /* x坐标为32，y轴坐标为8 */
        int ret = sprintf(templine, ": %.2f", air_temp);
        if (ret < 0) {
            printf("air_temp failed\r\n");
        }
        ssd1306_DrawString(templine, Font_7x10, White);
        //显示湿度
        ret = sprintf(humiline, ": %.2f", air_humi);
        if (ret < 0) {
            printf("air_humi failed\r\n");
        }
        printf("air_temp = %s, air_humi = %s\r\n", templine, humiline);
        ssd1306_SetCursor(32, 40); /* x坐标为32，y轴坐标为40 */
        ssd1306_DrawString(humiline, Font_7x10, White);
        //刷新屏幕
        ssd1306_UpdateScreen();




        
        osal_mdelay(1000); // 1s监测一次
    }
}

void Aht20Test(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)Aht20TestTask, 0, "PwmTask", I2C_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, I2C_TASK_PRIO);
        osal_kfree(task_handle);
    }
    osal_kthread_unlock();
}

app_run(Aht20Test);