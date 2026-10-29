#if defined(__has_include)
#if __has_include("mqtt_demo.local.h")
#include "mqtt_demo.local.h"
#endif
#endif
#ifndef PROJECT_G_USERNAME
#define PROJECT_G_USERNAME "YOUR_MQTT_USERNAME"
#endif
#ifndef PROJECT_G_PASSWORD
#define PROJECT_G_PASSWORD "YOUR_MQTT_PASSWORD"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MQTTClient.h"
#include "MQTTClientPersistence.h"
#include "osal_debug.h"
#include "MQTTClient.h"
#include "los_memory.h"
#include "los_task.h"
#include "soc_osal.h"
#include "app_init.h"
#include "common_def.h"
#include "wifi_connect.h"
#include "watchdog.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "cjson_demo.h"
#include "aht20_test.h"
#include "aht20.h"
#include "pwm_demo.h"
#include "mqtt_demo.h"
/*
#include "environment/servo.h"
#include "environment/led.h"
#include "environment/beep.h"
#include "environment/soil_sensor.h"*/
#include "environment/servo.h"   // 舵机控制

//华为云MQTT配置参数
#define ADDRESS "tcp://67a708766a.st1.iotda-device.cn-north-4.myhuaweicloud.com:1883"
#define CLIENTID "69dd0da87f2e6c302f66b845_agri_hi3863_001_0_1_2026041512"
//设备唯一标识id
#define TOPIC "MQTT Examples"
#define PAYLOAD "Hello World!"
#define QOS 1

//wifi配置
#ifndef CONFIG_WIFI_SSID
#define CONFIG_WIFI_SSID "YOUR_WIFI_SSID"
#endif       // 要连接的WiFi 热点账号
#ifndef CONFIG_WIFI_PWD
#define CONFIG_WIFI_PWD "YOUR_WIFI_PASSWORD"
#endif // 要连接的WiFi 热点密码

//任务配置
#define MQTT_STA_TASK_PRIO 24
#define MQTT_STA_TASK_STACK_SIZE 0x1000//任务栈大小
#define TIMEOUT 10000L
#define MSG_MAX_LEN 28//消息队列每条消息最大长度
#define MSG_QUEUE_SIZE 32//消息队列大小

//全局变量
static unsigned long g_msg_queue;
volatile MQTTClient_deliveryToken deliveredtoken;
char *g_username = PROJECT_G_USERNAME; // deviceId or nodeId:46e3cbfe58f49e437f72816e9cac0b59
char *g_password = PROJECT_G_PASSWORD;
MQTTClient client;
extern int MQTTClient_init(void);

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    unused(context);
    printf("Message with token value %d delivery confirmed\r\n", dt);
    deliveredtoken = dt;
}
int msgArrved(void *context, char *topic_name, int topic_len, MQTTClient_message *message)
{
    unused(context);
    unused(topic_len);
    MQTT_msg *receive_msg = osal_kmalloc(sizeof(MQTT_msg), 0);
    printf("mqtt_message_arrive() success, the topic is %s, the payload is %s \n", topic_name, message->payload);
    receive_msg->msg_type = EN_MSG_PARS;
    receive_msg->receive_payload = message->payload;
    uint32_t ret = osal_msg_queue_write_copy(g_msg_queue, receive_msg, sizeof(MQTT_msg), OSAL_WAIT_FOREVER);
    if (ret != 0) {
        printf("ret = %#x\r\n", ret);
        osal_kfree(receive_msg);
        return -1;
    }
    return 1;
}

void connlost(void *context, char *cause)
{
    unused(context);
    printf("mqtt_connection_lost() error, cause: %s\n", cause);
}

int mqtt_subscribe(const char *topic)
{
    printf("subscribe start\r\n");
    MQTTClient_subscribe(client, topic, QOS);
    return 0;
}

int mqtt_publish(const char *topic, MQTT_msg *report_msg)
{
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc = 0;
    char *msg = make_json("sensor", report_msg->temp, report_msg->humi);
    pubmsg.payload = msg;
    pubmsg.payloadlen = (int)strlen(msg);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    rc = MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    if (rc != MQTTCLIENT_SUCCESS) {
        printf("mqtt_publish failed\r\n");
    }
    printf("mqtt_publish(), the payload is %s, the topic is %s\r\n", msg, topic);
    osal_kfree(msg);
    msg = NULL;
    return rc;
}


//————————————————————————MQTT主任务——————————————————————————
int mqtt_connect(void)
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    printf("start mqtt sync subscribe...\r\n");
    MQTTClient_init();
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 120; // 保持间隔为120秒，每120秒发送一次消息
    conn_opts.cleansession = 1;
    if (g_username != NULL) {
        conn_opts.username = g_username;
        conn_opts.password = g_password;
    }
    MQTTClient_setCallbacks(client, NULL, connlost, msgArrved, delivered);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        return -1;
    }
    printf("connect success\r\n");
    return rc;
}

int mqtt_task(void)
{
    MQTT_msg *report_msg = osal_kmalloc(sizeof(MQTT_msg), 0);
    int ret = 0;
    uint32_t resize = 32;
    char *beep_status = NULL;
    wifi_connect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
    ret = mqtt_connect();
    if (ret != 0) {
        printf("connect failed, result %d\n", ret);
    }
    osal_msleep(1000); // 1000等待连接成功
    char *cmd_topic = combine_strings(3, "$oc/devices/", g_username, "/sys/commands/#");
    ret = mqtt_subscribe(cmd_topic);
    if (ret < 0) {
        printf("subscribe topic error, result %d\n", ret);
    }
    char *report_topic = combine_strings(3, "$oc/devices/", g_username, "/sys/properties/report");
    while (1) {
        ret = osal_msg_queue_read_copy(g_msg_queue, report_msg, &resize, OSAL_WAIT_FOREVER);
        if (ret != 0) {
            printf("queue_read ret = %#x\r\n", ret);
            osal_kfree(report_msg);
            break;
        }
        if (report_msg != NULL) {
            printf("report_msg->msg_type = %d, report_msg->temp = %s\r\n", report_msg->msg_type, report_msg->temp);
            switch (report_msg->msg_type) {
                case EN_MSG_PARS:
                    beep_status = parse_json(report_msg->receive_payload);
                    pwm_task(beep_status);
                    break;
                case EN_MSG_REPORT:
                    mqtt_publish(report_topic, report_msg);
                    break;
                default:
                    break;
            }
            osal_kfree(report_msg);
        }
        osal_msleep(1000); // 1000等待连接成功
    }
    return ret;
}


//—————————————————————————传感器采集任务——————————————————————
/*20260414
void environment_task_entry(void)
{
    MQTT_msg *mqtt_msg;
    environment_msg aht_msg;
    //分配MQTT消息内存
    mqtt_msg = osal_kmalloc(sizeof(MQTT_msg), 0);
    // 检查内存分配
    if (mqtt_msg == NULL) {
        printf("Memory allocation failed\r\n");
    }

    //初始化硬件
    aht20_init();//初始化AHT20温湿度传感器
    pwm_init();//初始化PWM（用于控制蜂鸣器/舵机）

    //主循环
    while (1) {
        aht20_test_task(&aht_msg);
        mqtt_msg->msg_type = EN_MSG_REPORT;
        if ((mqtt_msg != NULL) && (aht_msg.temperature != 0) && (aht_msg.humidity != 0)) {
            sprintf(mqtt_msg->temp, "%.2f", aht_msg.temperature);
            sprintf(mqtt_msg->humi, "%.2f", aht_msg.humidity);
            printf("temperature = %s, humidity= %s\r\n", mqtt_msg->temp, mqtt_msg->humi);
            uint32_t ret = osal_msg_queue_write_copy(g_msg_queue, mqtt_msg, sizeof(MQTT_msg), OSAL_WAIT_FOREVER);
            if (ret != 0) {
                printf("ret = %#x\r\n", ret);
                osal_kfree(mqtt_msg);
                break;
            }
        }
        osal_msleep(1000); // 1000ms读取数据
    }
    osal_kfree(mqtt_msg);
}
*/

//20260414 真实读取空气温湿度，用空气温度模拟土壤温度，写死了土壤湿度的代码。目的是为了测试舵机的逻辑
// 传感器采集任务：读取温湿度、控制舵机、上报数据到华为云

//测试串口是否正常输出的代码
/*
void environment_task_entry(void)
{
    printf("\r\n========== Hello World ==========\r\n");
    while (1) {
        printf("Running...\r\n");
        osal_mdelay(1000);
    }
}
*/

void environment_task_entry(void)
{
    MQTT_msg *mqtt_msg;          // MQTT消息结构体指针，用于上报数据
    environment_msg aht_msg;     // AHT20传感器数据结构体，存储温湿度
    
    // 传感器变量
    float air_temp = 0.0f;       // 空气温度
    float air_humi = 0.0f;       // 空气湿度
    
    // 硬件初始化
    printf("\r\n========== 硬件初始化 ==========\r\n");
    aht20_init();                // 初始化AHT20传感器和OLED显示屏
    servo_init();                // 初始化舵机PWM
    printf("========== 初始化完成 ==========\r\n\r\n");
    
    // 分配MQTT消息内存
    mqtt_msg = osal_kmalloc(sizeof(MQTT_msg), 0);
    if (mqtt_msg == NULL) {
        printf("Memory allocation failed\r\n");
        return;
    }
    
    while (1) {
        // 步骤1：读取空气温湿度
        aht20_test_task(&aht_msg);
        air_temp = aht_msg.temperature;
        air_humi = aht_msg.humidity;
        
        // 步骤2：串口打印传感器数据
        printf("\r\n========== 传感器数据 ==========\r\n");
        printf("air_temp = %.2f C\r\n", air_temp);
        printf("air_humi = %.2f %%\r\n", air_humi);
        
        // 步骤3：根据湿度控制舵机（模拟灌溉）
        if (air_humi < 40.0f) {
            servo_set_angle(90);     // 湿度低于40%，打开水阀
            printf("[舵机] 湿度%.2f%% < 40%%，打开水阀 -> 90度\r\n", air_humi);
        } else {
            servo_set_angle(0);      // 湿度正常，关闭水阀
            printf("[舵机] 湿度%.2f%% >= 40%%，关闭水阀 -> 0度\r\n", air_humi);
        }
        
        // 步骤4：将温湿度数据放入消息队列，供MQTT任务上报到华为云
        mqtt_msg->msg_type = EN_MSG_REPORT;              // 消息类型为"上报"
        sprintf(mqtt_msg->temp, "%.2f", air_temp);       // 温度转为字符串
        sprintf(mqtt_msg->humi, "%.2f", air_humi);       // 湿度转为字符串
        uint32_t ret = osal_msg_queue_write_copy(g_msg_queue, mqtt_msg, sizeof(MQTT_msg), OSAL_WAIT_FOREVER);
        if (ret != 0) {
            printf("queue write failed, ret = %#x\r\n", ret);
        }
        
        printf("================================\r\n\r\n");
        
        // 步骤5：延时3秒
        osal_mdelay(3000);

    }
    osal_kfree(mqtt_msg);    // 释放内存（实际上不会执行到这里）
}


static void mqtt_sample_entry(void)
{
    uint32_t ret;
    uapi_watchdog_disable();
    ret = osal_msg_queue_create("name", MSG_QUEUE_SIZE, &g_msg_queue, 0, MSG_MAX_LEN);
    if (ret != OSAL_SUCCESS) {
        printf("create queue failure!,error:%x\n", ret);
    }
    printf("create the queue success! queue_id = %d\n", g_msg_queue);
    osal_task *task_handle = NULL;
    osal_task *env_task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)mqtt_task, 0, "MqttDemoTask", MQTT_STA_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, MQTT_STA_TASK_PRIO);
        osal_kfree(task_handle);
    }
    env_task_handle =
        osal_kthread_create((osal_kthread_handler)environment_task_entry, 0, "EnvDemoTask", MQTT_STA_TASK_STACK_SIZE);
    if (env_task_handle != NULL) {
        osal_kthread_set_priority(env_task_handle, MQTT_STA_TASK_PRIO);
        osal_kfree(env_task_handle);
    }
    osal_kthread_unlock();
}

/* Run the udp_client_sample_task. */
app_run(mqtt_sample_entry);