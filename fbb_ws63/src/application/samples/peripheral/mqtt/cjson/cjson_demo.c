#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cJSON.h"

/**
 * @brief 计算字符串长度（安全版本，处理 NULL 指针）
 * @param str 输入字符串
 * @return 字符串长度，如果是 NULL 返回 0
 */
int string_length(char *str)
{
    if (str == NULL) {
        return 0;
    }
    int len = 0;
    char *temp_str = str;
    while (*temp_str++ != '\0') {
        len++;
    }
    return len;
}

/**
 * @brief 拼接多个字符串
 * 作用：把多个字符串拼成一个，用于生成 MQTT 主题
 * 例如：combine_strings(3, "$oc/devices/", g_username, "/sys/properties/report")
 * @param str_amount 要拼接的字符串数量
 * @param str1 第一个字符串
 * @param ... 后续字符串（可变参数）
 * @return 拼接后的新字符串，调用者需要 free
 */
char *combine_strings(int str_amount, char *str1, ...)
{
    int length = string_length(str1) + 1;
    if (length == 1) {
        return NULL;
    }

    char *result = malloc(length);
    if (result == NULL) {
        return NULL;
    }
    strcpy(result, str1);

    va_list args;
    va_start(args, str1);

    char *tem_str;
    while (--str_amount > 0) {
        tem_str = va_arg(args, char *);
        if (tem_str == NULL) {
            continue;
        }
        length += string_length(tem_str);
        result = realloc(result, length);
        if (result == NULL) {
            return NULL;
        }
        strcat(result, tem_str);
    }
    va_end(args);

    return result;
}

/**
 * @brief 生成上报华为云的 JSON 字符串
 * 作用：把传感器数据打包成华为云要求的格式
 * @param service_id 服务 ID，你的物模型服务名是 "sensor"
 * @param temperature 空气温度（字符串格式，如 "28.53"）
 * @param humidity 空气湿度（字符串格式，如 "33.40"）
 * @return JSON 字符串，调用者需要 free
 * 
 * 上报的属性包括（必须和华为云物模型属性名完全一致）：
 * - air_temp: 空气温度（真实数据）
 * - air_humi: 空气湿度（真实数据）
 * - soil_temp: 土壤温度（用空气温度模拟）
 * - soil_humi: 土壤湿度（用空气湿度模拟）
 * - co2: 二氧化碳浓度（暂为 0）
 * - light: 光照强度（暂为 0）
 */
char *make_json(char *service_id, char *temperature, char *humidity)
{
    // 创建根 JSON 对象 {}
    cJSON *root = cJSON_CreateObject();
    
    // 创建 services 数组 []
    cJSON *services = cJSON_CreateArray();
    
    // 创建单个 service 对象 {}
    cJSON *service = cJSON_CreateObject();
    // 添加 service_id 字段，值就是传入的 "sensor"
    cJSON_AddStringToObject(service, "service_id", service_id);

    // 创建 properties 对象 {}
    cJSON *properties = cJSON_CreateObject();
    
    // ========== 以下是上报的属性，属性名必须和华为云物模型一致 ==========
    
    // 空气温度（真实数据）- 属性名 "air_temp"
    cJSON_AddStringToObject(properties, "air_temp", temperature);
    
    // 空气湿度（真实数据）- 属性名 "air_humi"
    cJSON_AddStringToObject(properties, "air_humi", humidity);
    
    // 土壤温度（用空气温度模拟）- 属性名 "soil_temp"
    cJSON_AddStringToObject(properties, "soil_temp", temperature);
    
    // 土壤湿度（用空气湿度模拟）- 属性名 "soil_humi"
    cJSON_AddStringToObject(properties, "soil_humi", humidity);
    
    // 二氧化碳浓度（暂时填 0）- 属性名 "co2"
    cJSON_AddNumberToObject(properties, "co2", 0);
    
    // 光照强度（暂时填 0）- 属性名 "light"
    cJSON_AddNumberToObject(properties, "light", 0);

    // ========== 组装 JSON 结构 ==========
    
    // 把 properties 添加到 service
    cJSON_AddItemToObject(service, "properties", properties);
    // 把 service 添加到 services 数组
    cJSON_AddItemToArray(services, service);
    // 把 services 添加到根对象
    cJSON_AddItemToObject(root, "services", services);

    // 把 JSON 对象转换成字符串
    char *json_string = cJSON_Print(root);
    
    // 释放 cJSON 对象占用的内存
    cJSON_Delete(root);
    
    printf("=== NEW make_json is running! ===\r\n");//20260414 测试用
    // 返回 JSON 字符串
    return json_string;
}

/**
 * @brief 解析华为云下发的命令 JSON，提取 beep 字段
 * 作用：当用户在华为云控制台点击命令按钮时，云端会下发 JSON，这个函数从中提取命令参数
 * @param json_string 云端下发的 JSON 字符串
 * @return beep 字段的值（"ON" 或 "OFF"），失败返回 NULL
 */
char *parse_json(char *json_string)
{
    char *string = NULL;
    
    // 解析 JSON 字符串，转成 cJSON 对象
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        printf("Error parsing JSON\n");
        return NULL;
    }
    
    // 获取 paras 对象
    cJSON *paras = cJSON_GetObjectItem(root, "paras");
    // 获取 paras 里的 beep 字段
    cJSON *beep = cJSON_GetObjectItem(paras, "beep");
    
    // 提取 beep 的值
    if (beep && cJSON_IsString(beep)) {
        printf("beep: %s\n", beep->valuestring);
        string = beep->valuestring;
    } else {
        printf("beep not found or is not a string\n");
    }
    
    // 释放 cJSON 对象
    cJSON_Delete(root);
    
    return string;
}