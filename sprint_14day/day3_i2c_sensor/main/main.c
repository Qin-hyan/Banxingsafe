/**
 * @file main.c
 * @brief Day 3: I2C 传感器驱动测试
 * @description 初始化 I2C 总线并读取板载传感器数据
 * 
 * 任务目标：
 * 1. 使用 ESP-IDF I2C Driver 初始化总线
 * 2. 读取设备 ID 寄存器
 * 3. 输出原始加速度值至串口
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "Day3_I2C";

// 传感器配置 (基于 ESP32-S3-EYE SUB V1.1 子板)
#define SENSOR_I2C_ADDR         0x12    // 检测到的传感器 I2C 地址
#define SENSOR_I2C_FREQ_HZ      400000  // 400kHz

// I2C 引脚配置 (ESP32-S3-EYE 主版板载传感器)
#define I2C_SDA_PIN             8       // SDA 引脚 (GPIO8)
#define I2C_SCL_PIN             9       // SCL 引脚 (GPIO9)
#define I2C_NUM                 I2C_NUM_0

// 传感器寄存器地址 (尝试多种可能的配置)
#define SENSOR_REG_DEVID        0x0F    // 设备 ID 寄存器 (QMA7981/BMA42x)
#define SENSOR_REG_CTRL         0x4B    // 控制寄存器
#define SENSOR_REG_ACC_X_LSB    0x02    // 加速度 X 低字节

// I2C 超时时间 (ms)
#define I2C_TIMEOUT_MS          100

// 前向声明
static esp_err_t i2c_read_reg(uint8_t reg, uint8_t *data);
static esp_err_t i2c_write_reg(uint8_t reg, uint8_t data);
static esp_err_t i2c_read_regs(uint8_t reg, uint8_t *data, size_t len);

/**
 * @brief I2C 初始化
 */
static esp_err_t i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = SENSOR_I2C_FREQ_HZ,
    };
    
    esp_err_t ret = i2c_param_config(I2C_NUM, &conf);
    if (ret != ESP_OK) {
        return ret;
    }
    
    return i2c_driver_install(I2C_NUM, conf.mode, 0, 0, 0);
}

/**
 * @brief I2C 读取单个字节 (寄存器)
 */
static esp_err_t i2c_read_reg(uint8_t reg, uint8_t *data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SENSOR_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SENSOR_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

/**
 * @brief I2C 写入单个字节
 */
static esp_err_t i2c_write_reg(uint8_t reg, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SENSOR_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

/**
 * @brief I2C 读取多个字节
 */
static esp_err_t i2c_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SENSOR_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SENSOR_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

/**
 * @brief 读取传感器设备 ID
 */
static esp_err_t sensor_read_device_id(uint8_t *device_id)
{
    return i2c_read_reg(SENSOR_REG_DEVID, device_id);
}

/**
 * @brief 读取三轴加速度数据
 */
static esp_err_t sensor_read_acceleration(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buffer[6];
    
    // 读取 6 字节加速度数据 (X, Y, Z 各 2 字节，小端格式)
    esp_err_t ret = i2c_read_regs(SENSOR_REG_ACC_X_LSB, buffer, 6);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 解析数据 (16 位有符号整数，小端格式)
    *x = (int16_t)((buffer[1] << 8) | buffer[0]);
    *y = (int16_t)((buffer[3] << 8) | buffer[2]);
    *z = (int16_t)((buffer[5] << 8) | buffer[4]);
    
    return ESP_OK;
}

/**
 * @brief I2C 扫描检测设备
 */
static void i2c_scan(void)
{
    ESP_LOGI(TAG, "Performing I2C bus scan...");
    printf("\nI2C Bus Scan:\n");
    printf("   00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F\n");
    
    for (int i = 0; i < 128; i += 16) {
        printf("%02X: ", i);
        for (int j = 0; j < 16; j++) {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (i + j) << 1 | I2C_MASTER_WRITE, true);
            i2c_master_stop(cmd);
            
            esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 50 / portTICK_PERIOD_MS);
            i2c_cmd_link_delete(cmd);
            
            if (ret == ESP_OK) {
                printf("%02X ", i + j);
            } else {
                printf("-- ");
            }
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * @brief 打印欢迎信息
 */
static void print_banner(void)
{
    printf("\n");
    printf("===========================================================\n");
    printf("  Day 3: I2C 传感器驱动测试\n");
    printf("===========================================================\n");
    printf("  任务：初始化 I2C 总线并读取姿态传感器数据\n");
    printf("  传感器：Bosch QMA7981/BMA42x (尝试检测)\n");
    printf("  I2C 频率：400kHz\n");
    printf("  SDA 引脚：GPIO8\n");
    printf("  SCL 引脚：GPIO9\n");
    printf("===========================================================\n\n");
}

/**
 * @brief 主任务
 */
void app_main(void)
{
    print_banner();
    
    // 初始化 I2C
    ESP_LOGI(TAG, "Initializing I2C bus (SDA=%d, SCL=%d)...", I2C_SDA_PIN, I2C_SCL_PIN);
    esp_err_t ret = i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "I2C bus initialized successfully");
    
    // I2C 扫描检测设备
    i2c_scan();
    
    // 尝试多个可能的传感器地址 (0x12 是 ESP32-S3-EYE SUB V1.1 子板上检测到的地址)
    uint8_t possible_addresses[] = {0x12, 0x18, 0x19, 0x68, 0x69};
    uint8_t device_id;
    bool sensor_found = false;
    
    printf("\nTrying possible sensor addresses...\n");
    for (int i = 0; i < sizeof(possible_addresses)/sizeof(possible_addresses[0]); i++) {
        uint8_t addr = possible_addresses[i];
        
        // 临时修改传感器地址
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, SENSOR_REG_DEVID, true);
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
        i2c_master_read_byte(cmd, &device_id, I2C_MASTER_NACK);
        i2c_master_stop(cmd);
        
        ret = i2c_master_cmd_begin(I2C_NUM, cmd, 50 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        
        if (ret == ESP_OK) {
            printf("Address 0x%02X: Device ID = 0x%02X\n", addr, device_id);
            // 接受 0x00 作为有效响应 (某些传感器在未初始化时返回 0x00)
            if (device_id != 0xFF) {
                ESP_LOGI(TAG, "Sensor found at 0x%02X! Device ID: 0x%02X", addr, device_id);
                sensor_found = true;
                break;
            }
        } else {
            printf("Address 0x%02X: No response\n", addr);
        }
    }
    
    if (!sensor_found) {
        ESP_LOGW(TAG, "No valid sensor detected. Please check hardware connection.");
    }
    
    printf("\n");
    printf("===========================================================\n");
    printf("Starting acceleration data reading...\n");
    printf("===========================================================\n\n");
    
    int16_t accel_x, accel_y, accel_z;
    uint32_t sample_count = 0;
    
    while (1) {
        // 读取加速度数据
        ret = sensor_read_acceleration(&accel_x, &accel_y, &accel_z);
        if (ret != ESP_OK) {
            printf("\r[ERROR] Read failed: %s                    \n", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        // 打印数据
        printf("\r[%05lu] X: %7d  Y: %7d  Z: %7d    ", 
               (unsigned long)sample_count++, accel_x, accel_y, accel_z);
        fflush(stdout);
        
        // 延迟 100ms (10Hz 采样率)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}