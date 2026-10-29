/*
 * 舵机（PWM模拟水阀）
 */
#include "servo.h"
#include "pinctrl.h"
#include "pwm.h"
#include "soc_osal.h"
#include "osal_debug.h"

#define SERVO_PWM_CHANNEL   2
#define SERVO_PWM_GROUP     3
#define SERVO_PWM_PIN       10
#define SERVO_PIN_MODE      1

void servo_init(void)
{
    pwm_config_t cfg = {640000, 0, 0, 0, true};  // 50Hz周期
    uapi_pin_set_mode(SERVO_PWM_PIN, SERVO_PIN_MODE);
    uapi_pwm_init();
    uapi_pwm_open(SERVO_PWM_CHANNEL, &cfg);
    uint8_t channel_id = SERVO_PWM_CHANNEL;
    uapi_pwm_set_group(SERVO_PWM_GROUP, &channel_id, 1);
    uapi_pwm_start_group(SERVO_PWM_GROUP);
    printf("[SERVO] init success\r\n");
}

void servo_set_angle(int angle)
{
    // 这个SDK的PWM不支持动态改占空比，需要用其他方式
    // 暂时只做开关控制：角度>90 开，否则关
    if (angle > 90) {
        uapi_pwm_start_group(SERVO_PWM_GROUP);
    } else {
        uapi_pwm_stop_group(SERVO_PWM_GROUP);
    }
    printf("[SERVO] angle=%d\r\n", angle);
}