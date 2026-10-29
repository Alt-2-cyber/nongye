/*
 * LED补光灯（GPIO）
 */
#include "led.h"
#include "pinctrl.h"
#include "gpio.h"
#include "soc_osal.h"
#include "osal_debug.h"

#define LED_PIN        9
#define LED_PIN_MODE   0

void led_init(void)
{
    uapi_pin_set_mode(LED_PIN, LED_PIN_MODE);
    uapi_gpio_init();
    uapi_gpio_set_dir(LED_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(LED_PIN, GPIO_LEVEL_LOW);   // 注意：set_val 不是 set_value
    printf("[LED] init success\r\n");
}

void led_control(int on)
{
    if (on) {
        uapi_gpio_set_val(LED_PIN, GPIO_LEVEL_HIGH);
        printf("[LED] ON\r\n");
    } else {
        uapi_gpio_set_val(LED_PIN, GPIO_LEVEL_LOW);
        printf("[LED] OFF\r\n");
    }
}