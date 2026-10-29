/*
 * 蜂鸣器（PWM）
 */
#include "beep.h"
#include "pinctrl.h"
#include "pwm.h"
#include "soc_osal.h"
#include "osal_debug.h"

#define BEEP_PWM_CHANNEL    0
#define BEEP_PWM_GROUP      2
#define BEEP_PWM_PIN        8
#define BEEP_PIN_MODE       1

void beep_init(void)
{
    pwm_config_t cfg = {20000, 0, 0, 0, true};  // 周期20000，高电平0（静音）
    uapi_pin_set_mode(BEEP_PWM_PIN, BEEP_PIN_MODE);
    uapi_pwm_init();
    uapi_pwm_open(BEEP_PWM_CHANNEL, &cfg);
    uint8_t channel_id = BEEP_PWM_CHANNEL;
    uapi_pwm_set_group(BEEP_PWM_GROUP, &channel_id, 1);
    printf("[BEEP] init success\r\n");
}

void beep_alarm(int on)
{
    if (on) {
        uapi_pwm_start_group(BEEP_PWM_GROUP);
        printf("[BEEP] ON\r\n");
    } else {
        uapi_pwm_stop_group(BEEP_PWM_GROUP);
        printf("[BEEP] OFF\r\n");
    }
}