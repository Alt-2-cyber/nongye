/*
 * 蜂鸣器（PWM）
 */

#ifndef BEEP_H
#define BEEP_H

// 初始化蜂鸣器
void beep_init(void);

// 控制蜂鸣器，on=1响，on=0停
void beep_alarm(int on);

#endif