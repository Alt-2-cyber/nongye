/*
 * LED补光灯（GPIO）
 */

#ifndef LED_H
#define LED_H

// 初始化LED
void led_init(void);

// 控制LED开关，on=1开，on=0关
void led_control(int on);

#endif