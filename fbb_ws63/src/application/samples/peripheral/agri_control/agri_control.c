//LED+水阀执行器(自动 + 远程控制)
#include "agri_control.h"
#include "drivers/pwm.h"
#include <stdio.h>

// ==================== 引脚定义 ====================
#define LED_PIN     5
#define VALVE_PIN   6

// ==================== 自动控制阈值 ====================
#define LIGHT_LOW       1000
#define SOIL_DRY         30
#define SOIL_WET         50

// ==================== 全局状态 ====================
static int g_mode = MODE_AUTO;         // 默认自动
static int g_led = 0;
static bool g_valve = false;

// ==================== PWM初始化 ====================
int agri_init(void)
{
    // LED PWM
    pwm_config_t cfg_led = {0};
    cfg_led.freq = 500;
    cfg_led.duty = 0;
    pwm_init(LED_PIN, &cfg_led);

    // 水阀舵机 PWM
    pwm_config_t cfg_valve = {0};
    cfg_valve.freq = 50;
    cfg_valve.duty = 0;
    pwm_init(VALVE_PIN, &cfg_valve);

    // 初始状态
    agri_remote_set_led(0);
    agri_remote_set_valve(false);
    printf("水阀控制模块初始化完成\n");
    return 0;
}

// ==================== 模式设置 ====================
void agri_set_mode(int mode)
{
    g_mode = (mode == MODE_AUTO || mode == MODE_REMOTE) ? mode : MODE_AUTO;
    printf("模式切换: %s\n", g_mode ? "远程控制" : "自动控制");
}

int agri_get_mode(void)
{
    return g_mode;
}

// ==================== 远程控制 ====================
void agri_remote_set_led(int brightness)
{
    if (brightness < 0) brightness = 0;
    if (brightness > 100) brightness = 100;
    g_led = brightness;
    pwm_set_duty(LED_PIN, (float)g_led);
}

void agri_remote_set_valve(bool open)
{
    g_valve = open;
    if (open) {
        pwm_set_angle(VALVE_PIN, 90);  // 开
    } else {
        pwm_set_angle(VALVE_PIN, 0);   // 关
    }
}

// ==================== 自动控制逻辑 ====================
void agri_auto_run(AgriSensor *s)
{
    if (g_mode != MODE_AUTO) return;  // 不是自动就直接退出

    // 自动调光
    if (s->light < LIGHT_LOW) {
        g_led = 100;
    } else {
        g_led = 0;
    }

    // 自动浇水
    if (s->soil < SOIL_DRY) {
        g_valve = true;
    } else if (s->soil > SOIL_WET) {
        g_valve = false;
    }

    // 输出到硬件
    pwm_set_duty(LED_PIN, (float)g_led);
    pwm_set_angle(VALVE_PIN, g_valve ? 90 : 0);
}

// ==================== 获取状态 ====================
int agri_get_led(void) { return g_led; }
bool agri_get_valve(void) { return g_valve; }