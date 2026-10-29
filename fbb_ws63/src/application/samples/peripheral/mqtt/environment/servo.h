/*
 * 舵机（PWM模拟水阀）
 */

#ifndef SERVO_H
#define SERVO_H

// 初始化舵机
void servo_init(void);

// 设置舵机角度（0~180度）
void servo_set_angle(int angle);

#endif