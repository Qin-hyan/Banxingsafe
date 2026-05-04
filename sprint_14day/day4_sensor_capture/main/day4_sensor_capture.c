/**
 * @file day4_sensor_capture.c
 * @brief Day 4: 传感器数据采集模块实现
 *
 * 基于 ESP32-S3-EYE 板载 QMA7981 传感器。
 * 封装初始化 (day4_sensor_init) 和数据采集 (day4_sensor_capture_service)。
 *
 * 硬件连接 (ESP32-S3-EYE SUB V1.1 子板):
 *   SDA: GPIO8
 *   SCL: GPIO9
 *   I2C 地址: 0x12
 *   I2C 频率: 400kHz
 *
 * 资源占用:
 *   - I2C_NUM_0 总线
 *   - 采集任务堆栈: 2048 字节 (最小)
 *   - 采集任务优先级: 3 (低于主任务)
 *   - 采样率: 10Hz (100ms 周期)
 */

#include "day4_sensor_capture.h"

#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_err.h>

/*============================================================================
 * 日志标签
 *============================================================================*/

static const char *TAG = "DAYN_SENSOR";

/*============================================================================
 * 硬件配置 - 基于 ESP32-S3-EYE SUB V1.1 子板
 *============================================================================*/

#define SENSOR_I2C_ADDR         0x12        /*!< QMA7981 I2C 地址 (检测到的板载地址) */
#define SENSOR_I2C_FREQ_HZ      400000      /*!< I2C 总线频率 400kHz */
#define SENSOR_I2C_NUM          I2C_NUM_0   /*!< I2C 控制器编号 */
#define SENSOR_SDA_GPIO         GPIO_NUM_8  /*!< SDA 引脚 (ESP32-S3-EYE 主版) */
#define SENSOR_SCL_GPIO         GPIO_NUM_9  /*!< SCL 引脚 (ESP32-S3-EYE 主版) */
#define SENSOR_I2C_TIMEOUT_MS   100         /*!< I2C 超时时间 (ms) */

/*============================================================================
 * QMA7981 寄存器定义
 *============================================================================*/

#define QMA7981_REG_DEVID       0x0F        /*!< 设备 ID 寄存器 */
#define QMA7981_REG_CTRL        0x4B        /*!< 控制寄存器 */
#define QMA7981_REG_ACC_X_LSB   0x02        /*!< 加速度 X 轴低字节 (起始地址) */
#define QMA7981_CTRL_RESET      0xB6        /*!< 软复位命令 */
#define QMA7981_DEVID_VALUE     0x01        /*!< QMA7981 预期的设备 ID */

/*============================================================================
 * 采样配置
 *============================================================================*/

#define SAMPLE_PERIOD_MS        100         /*!< 采样周期 100ms = 10Hz */
#define SAMPLE_TASK_STACK       2048        /*!< 采集任务堆栈大小 (字节) */
#define SAMPLE_TASK_PRIORITY    3           /*!< 采集任务优先级 */

/*============================================================================
 * 全局变量
 *============================================================================*/

/** 最新传感器数据，由采集任务周期更新，供主任务读取 */
volatile day4_sensor_data_t g_sensor_data = {0};

/*============================================================================
 * 私有函数声明
 *============================================================================*/

static esp_err_t sensor_i2c_init(void);
static esp_err_t sensor_i2c_read_reg(uint8_t reg, uint8_t *data);
static esp_err_t sensor_i2c_write_reg(uint8_t reg, uint8_t data);
static esp_err_t sensor_i2c_read_regs(uint8_t reg, uint8_t *data, size_t len);
static esp_err_t sensor_read_accel(int16_t *x, int16_t *y, int16_t *z);
static void sensor_i2c_scan(void);

/*============================================================================
 * I2C 底层操作
 *============================================================================*/

/**
 * @brief 初始化 I2C 总线
 */
static esp_err_t sensor_i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SENSOR_SDA_GPIO,
        .scl_io_num = SENSOR_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = SENSOR_I2C_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(SENSOR_I2C_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C 参数配置失败: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(SENSOR_I2C_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C 驱动安装失败: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C 总线初始化完成 (SDA=%d, SCL=%d, %d kHz)",
             SENSOR_SDA_GPIO, SENSOR_SCL_GPIO, SENSOR_I2C_FREQ_HZ / 1000);
    return ESP_OK;
}

/**
 * @brief I2C 读取单个寄存器字节
 */
static esp_err_t sensor_i2c_read_reg(uint8_t reg, uint8_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SENSOR_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SENSOR_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(SENSOR_I2C_NUM, cmd,
                                          pdMS_TO_TICKS(SENSOR_I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    return ret;
}

/**
 * @brief I2C 写入单个寄存器字节
 */
static esp_err_t sensor_i2c_write_reg(uint8_t reg, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SENSOR_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(SENSOR_I2C_NUM, cmd,
                                          pdMS_TO_TICKS(SENSOR_I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    return ret;
}

/**
 * @brief I2C 读取多个连续寄存器字节
 */
static esp_err_t sensor_i2c_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SENSOR_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SENSOR_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(SENSOR_I2C_NUM, cmd,
                                          pdMS_TO_TICKS(SENSOR_I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    return ret;
}

/**
 * @brief I2C 总线扫描 (调试用)
 */
static void sensor_i2c_scan(void)
{
    ESP_LOGI(TAG, "I2C 总线扫描开始...");
    printf("\n     ");
    for (int i = 0; i < 16; i++) {
        printf(" %02X  ", i);
    }
    printf("\n");

    int found = 0;
    for (int addr = 0; addr < 128; addr++) {
        if (addr % 16 == 0) {
            printf("%02X:  ", addr);
        }

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(SENSOR_I2C_NUM, cmd,
                                              pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            printf("%02X  ", addr);
            found++;
        } else {
            printf("--  ");
        }

        if ((addr + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n扫描完成: 发现 %d 个设备\n\n", found);
}

/*============================================================================
 * 传感器操作
 *============================================================================*/

/**
 * @brief 读取三轴加速度数据
 */
static esp_err_t sensor_read_accel(int16_t *x, int16_t *y, int16_t *z)
{
    if (x == NULL || y == NULL || z == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer[6];
    esp_err_t ret = sensor_i2c_read_regs(QMA7981_REG_ACC_X_LSB, buffer, 6);
    if (ret != ESP_OK) {
        return ret;
    }

    /* 小端格式: [L, H, L, H, L, H] */
    *x = (int16_t)((buffer[1] << 8) | buffer[0]);
    *y = (int16_t)((buffer[3] << 8) | buffer[2]);
    *z = (int16_t)((buffer[5] << 8) | buffer[4]);

    return ESP_OK;
}

/*============================================================================
 * 公共函数: 初始化
 *============================================================================*/

esp_err_t day4_sensor_init(void)
{
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Day 4: 传感器数据采集初始化");
    ESP_LOGI(TAG, "传感器: QMA7981 @ I2C 0x%02X", SENSOR_I2C_ADDR);
    ESP_LOGI(TAG, "引脚: SDA=GPIO%d, SCL=GPIO%d", SENSOR_SDA_GPIO, SENSOR_SCL_GPIO);
    ESP_LOGI(TAG, "采样率: %d Hz", 1000 / SAMPLE_PERIOD_MS);
    ESP_LOGI(TAG, "============================================");

    /* 1. 初始化 I2C 总线 */
    esp_err_t ret = sensor_i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C 初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 2. I2C 扫描确认设备 */
    sensor_i2c_scan();

    /* 3. 读取设备 ID 确认传感器 */
    uint8_t device_id = 0;
    ret = day4_sensor_read_device_id(&device_id);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "读取设备 ID 失败: %s，尝试继续...", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "QMA7981 设备 ID: 0x%02X", device_id);
        if (device_id != QMA7981_DEVID_VALUE && device_id != 0x00) {
            ESP_LOGW(TAG, "设备 ID 不匹配 (预期 0x%02X)，但仍尝试继续",
                     QMA7981_DEVID_VALUE);
        }
    }

    /* 4. 软复位传感器 (可选，某些配置需要) */
    ret = sensor_i2c_write_reg(QMA7981_REG_CTRL, QMA7981_CTRL_RESET);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "软复位失败: %s (非致命)", esp_err_to_name(ret));
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "Day 4 传感器初始化完成");
    return ESP_OK;
}

/*============================================================================
 * 公共函数: 设备 ID 读取
 *============================================================================*/

esp_err_t day4_sensor_read_device_id(uint8_t *id)
{
    return sensor_i2c_read_reg(QMA7981_REG_DEVID, id);
}

/*============================================================================
 * 公共函数: 单次读取
 *============================================================================*/

esp_err_t day4_sensor_read_once(day4_sensor_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int16_t ax = 0, ay = 0, az = 0;
    esp_err_t ret = sensor_read_accel(&ax, &ay, &az);

    if (ret == ESP_OK) {
        data->accel_x = ax;
        data->accel_y = ay;
        data->accel_z = az;
        data->timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000);
        data->valid = true;
    } else {
        data->valid = false;
        ESP_LOGW(TAG, "读取加速度数据失败: %s", esp_err_to_name(ret));
    }

    return ret;
}

/*============================================================================
 * 公共函数: 采集服务 (FreeRTOS 任务入口)
 *============================================================================*/

void day4_sensor_capture_service(void *arg)
{
    ESP_LOGI(TAG, "传感器数据采集服务启动 (堆栈: %u 字节, 周期: %d ms)",
             (unsigned int)SAMPLE_TASK_STACK, SAMPLE_PERIOD_MS);

    uint32_t sample_count = 0;
    day4_sensor_data_t local_data;

    while (1) {
        /* 读取传感器数据 */
        esp_err_t ret = day4_sensor_read_once(&local_data);

        if (ret == ESP_OK && local_data.valid) {
            /* 更新全局变量 (volatile 保证多任务可见性) */
            g_sensor_data.accel_x = local_data.accel_x;
            g_sensor_data.accel_y = local_data.accel_y;
            g_sensor_data.accel_z = local_data.accel_z;
            g_sensor_data.timestamp_ms = local_data.timestamp_ms;
            g_sensor_data.valid = true;

            /* 每 10 个样本 (1 秒) 输出一次 INFO 日志，减少串口刷屏 */
            if (sample_count % 10 == 0) {
                ESP_LOGI(TAG, "[%05lu] ACCEL X=%+7d Y=%+7d Z=%+7d LSB",
                         (unsigned long)sample_count,
                         local_data.accel_x,
                         local_data.accel_y,
                         local_data.accel_z);
            }

            sample_count++;
        } else {
            g_sensor_data.valid = false;
            ESP_LOGW(TAG, "[%05lu] 数据读取失败: %s",
                     (unsigned long)sample_count,
                     esp_err_to_name(ret));
        }

        /* 固定采样周期 */
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}