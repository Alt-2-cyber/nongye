#ifndef AGRI_CONTROL_H
#define AGRI_CONTROL_H

#include <stdbool.h>
#include "agri_sensor.h"

// 控制模式
#define MODE_AUTO     0  // 自动
#define MODE_REMOTE   1  // 远程

// 初始化LED、水阀(PWM)
int agri_init(void);

// 设置模式
void agri_set_mode(int mode);
int agri_get_mode(void);

// 远程控制（云端下发调用）
void agri_remote_set_led(int brightness);
void agri_remote_set_valve(bool open);

// 自动控制（主循环调用）
void agri_auto_run(AgriSensor *s);

// 获取当前状态（上报云端）
int agri_get_led(void);
bool agri_get_valve(void);

#endif