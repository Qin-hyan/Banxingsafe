/**
 * @file bma400.c
 * @brief Bosch BMA400 低功耗加速度计驱动实现
 * 
 * 12 位三轴加速度计驱动
 * 基于 ESP-IDF I2C 驱动
 */

#include "bma400.h"
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2c.h"

/*============================================================================
 * 私有宏定义
 *============================================================================*/

#define TAG "BMA400"

#define BMA400_I2C_TIMEOUT_MS       (100)           /*!< I2C 超时时间 */
#define BMA400_INIT_DELAY_MS        (10)            /*!< 初始化后延迟 */
#define BMA400_CMD_SOFT_RESET       (0xB6)          /*!< 软复位命令 */

/** 12 位数据转换为 mg 的系数 (±2g 时) */
#define BMA400_12BIT_TO_MG(x)       ((int16_t)((x) * 1000 / 2048))

/*============================================================================
 * 私有数据结构
 *============================================================================*/

/**
 * @brief 驱动私有上下文结构体
 */
typedef struct {
    bool initialized;                     /*!< 是否已初始化 */
    i2c_port_t i2c_num;                   /*!< I2C 控制器编号 */
    uint8_t i2c_addr;                     /*!< I2C 设备地址 */
    gpio_num_t int_gpio;                  /*!< 中断引脚 */
    bma400_config_t config;               /*!< 当前配置 */
    SemaphoreHandle_t mutex;              /*!< 互斥锁 */
    StaticSemaphore_t mutex_buffer;       /*!< 互斥锁缓冲区 */
    uint8_t chip_id;                      /*!< 实际芯片 ID */
} bma400_handle_t;

/*============================================================================
 * 私有全局变量
 *============================================================================*/

static bma400_handle_t s_bma400 = {0};

/*============================================================================
 * 私有函数声明
 *============================================================================*/

/**
 * @brief I2C 写寄存器
 */
static esp_err_t i2c_write_reg(uint8_t reg, uint8_t value);

/**
 * @brief I2C 读寄存器
 */
static esp_err_t i2c_read_reg(uint8_t reg, uint8_t *value);

/**
 * @brief I2C 写寄存器数组
 */
static esp_err_t i2c_write_regs(uint8_t reg, const uint8_t *data, size_t len);

/**
 * @brief I2C 读寄存器数组
 */
static esp_err_t i2c_read_regs(uint8_t reg, uint8_t *data, size_t len);

/**
 * @brief 检查 I2C 设备是否存在
 */
static esp_err_t i2c_check_device(i2c_port_t i2c_num, uint8_t addr);

/*============================================================================
 * 私有函数实现
 *============================================================================*/

static esp_err_t i2c_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_master_write_to_device(
        s_bma400.i2c_num,
        s_bma400.i2c_addr,
        buf,
        2,
        pdMS_TO_TICKS(BMA400_I2C_TIMEOUT_MS)
    );
}

static esp_err_t i2c_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_master_write_read_device(
        s_bma400.i2c_num,
        s_bma400.i2c_addr,
        &reg, 1,
        value, 1,
        pdMS_TO_TICKS(BMA400_I2C_TIMEOUT_MS)
    );
}

static esp_err_t i2c_write_regs(uint8_t reg, const uint8_t *data, size_t len)
{
    uint8_t *buf = malloc(len + 1);
    if (!buf) {
        return ESP_ERR_NO_MEM;
    }
    
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    
    esp_err_t ret = i2c_master_write_to_device(
        s_bma400.i2c_num,
        s_bma400.i2c_addr,
        buf, len + 1,
        pdMS_TO_TICKS(BMA400_I2C_TIMEOUT_MS)
    );
    
    free(buf);
    return ret;
}

static esp_err_t i2c_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(
        s_bma400.i2c_num,
        s_bma400.i2c_addr,
        &reg, 1,
        data, len,
        pdMS_TO_TICKS(BMA400_I2C_TIMEOUT_MS)
    );
}

static esp_err_t i2c_check_device(i2c_port_t i2c_num, uint8_t addr)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

/*============================================================================
 * 公共函数实现
 *============================================================================*/

esp_err_t bma400_init(const bma400_config_t *config)
{
    if (!config) {
        ESP_LOGE(TAG, "配置参数为空");
        return ESP_ERR_INVALID_ARG;
    }
    
    // 检查是否已初始化
    if (s_bma400.initialized) {
        bma400_deinit();
    }
    
    // 保存配置
    memcpy(&s_bma400.config, config, sizeof(bma400_config_t));
    s_bma400.i2c_num = config->i2c_num;
    s_bma400.i2c_addr = BMA400_I2C_ADDR;
    s_bma400.int_gpio = config->int_gpio;
    
    // 创建互斥锁
    s_bma400.mutex = xSemaphoreCreateMutexStatic(&s_bma400.mutex_buffer);
    if (!s_bma400.mutex) {
        return ESP_ERR_NO_MEM;
    }
    
    // 配置 I2C 总线
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_gpio,
        .scl_io_num = config->scl_gpio,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BMA400_I2C_FREQ,
    };
    
    esp_err_t ret = i2c_param_config(s_bma400.i2c_num, &i2c_conf);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = i2c_driver_install(s_bma400.i2c_num, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 检查设备是否存在
    ret = i2c_check_device(s_bma400.i2c_num, s_bma400.i2c_addr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C 设备 0x%02X 未响应：%s", s_bma400.i2c_addr, esp_err_to_name(ret));
        return ret;
    }
    
    // 读取芯片 ID
    ret = bma400_read_chip_id(&s_bma400.chip_id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取芯片 ID 失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    // 验证芯片 ID
    if (s_bma400.chip_id != BMA400_CHIP_ID_VALUE) {
        ESP_LOGE(TAG, "芯片 ID 不匹配！期望 0x%02X，实际 0x%02X", 
                 BMA400_CHIP_ID_VALUE, s_bma400.chip_id);
        return ESP_ERR_INVALID_VERSION;
    }
    
    // 软复位
    ret = i2c_write_reg(BMA400_REG_CMD, BMA400_CMD_SOFT_RESET);
    if (ret != ESP_OK) {
        // 继续执行，不立即失败
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // 配置量程
    ret = bma400_set_range(config->range);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 配置输出数据率
    ret = bma400_set_odr(config->odr);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 配置加速度计：12 位分辨率，正常模式
    uint8_t acc_conf = (config->resolution << 4) | 0x01;  // OSR1 = 1 (正常模式)
    ret = i2c_write_reg(BMA400_REG_ACC_CONF, acc_conf);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 配置功耗：低功耗模式
    ret = i2c_write_reg(BMA400_REG_PWR_CONF, 0x00);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 启用传感器：正常模式
    ret = i2c_write_reg(BMA400_REG_PWR_CTRL, 0x01);
    if (ret != ESP_OK) {
        return ret;
    }
    
    s_bma400.initialized = true;
    
    return ESP_OK;
}

esp_err_t bma400_init_default(void)
{
    bma400_config_t config = {
        .i2c_num = BMA400_I2C_NUM,
        .sda_gpio = BMA400_I2C_SDA_GPIO,
        .scl_gpio = BMA400_I2C_SCL_GPIO,
        .int_gpio = BMA400_INT_GPIO,
        .range = BMA400_RANGE_4G,
        .odr = BMA400_ODR_100HZ,
        .resolution = BMA400_RES_12BIT,
        .mode = BMA400_MODE_NORMAL,
    };
    
    return bma400_init(&config);
}

esp_err_t bma400_read_chip_id(uint8_t *chip_id)
{
    if (!chip_id) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return i2c_read_reg(BMA400_REG_CHIP_ID, chip_id);
}

esp_err_t bma400_read_reg(uint8_t reg, uint8_t *value)
{
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_read_reg(reg, value);
}

esp_err_t bma400_write_reg(uint8_t reg, uint8_t value)
{
    return i2c_write_reg(reg, value);
}

bool bma400_is_ready(void)
{
    if (!s_bma400.initialized) {
        return false;
    }
    
    uint8_t chip_id;
    esp_err_t ret = bma400_read_chip_id(&chip_id);
    return (ret == ESP_OK && chip_id == BMA400_CHIP_ID_VALUE);
}

esp_err_t bma400_read_accel(bma400_accel_data_t *data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_bma400.initialized) {
        ESP_LOGE(TAG, "设备未初始化");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 读取加速度数据 (6 字节，12 位精度，左对齐)
    // X: 0x04 (低 8 位), 0x05 (高 4 位)
    // Y: 0x06 (低 8 位), 0x07 (高 4 位)
    // Z: 0x08 (低 8 位), 0x09 (高 4 位)
    uint8_t buffer[6];
    esp_err_t ret = i2c_read_regs(BMA400_REG_ACC_X_L, buffer, 6);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取加速度数据失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    // 解析 12 位数据 (左对齐，高 12 位有效)
    // 数据格式：[D11:D4][D3:D0|0000]
    int16_t x_raw = (int16_t)((buffer[1] << 4) | (buffer[0] >> 4));
    int16_t y_raw = (int16_t)((buffer[3] << 4) | (buffer[2] >> 4));
    int16_t z_raw = (int16_t)((buffer[5] << 4) | (buffer[4] >> 4));
    
    // 符号扩展 (12 位补码)
    if (x_raw & 0x0800) x_raw |= 0xF000;
    if (y_raw & 0x0800) y_raw |= 0xF000;
    if (z_raw & 0x0800) z_raw |= 0xF000;
    
    // 转换为 mg (基于当前量程)
    // 12 位分辨率，满量程 2048 LSB/g
    int16_t range_g = 1 << (s_bma400.config.range + 1);  // 2g, 4g, 8g, 16g
    float scale = (float)range_g * 1000.0f / 2048.0f;    // mg/LSB
    
    data->x = (int16_t)(x_raw * scale);
    data->y = (int16_t)(y_raw * scale);
    data->z = (int16_t)(z_raw * scale);
    
    return ESP_OK;
}

esp_err_t bma400_set_range(uint8_t range)
{
    // 检查量程值是否有效 (0x00=16g, 0x01=2g, 0x02=4g, 0x03=8g)
    if (range > BMA400_RANGE_8G) {
        return ESP_ERR_INVALID_ARG;
    }
    
    s_bma400.config.range = range;
    
    // 写入量程配置寄存器
    return i2c_write_reg(BMA400_REG_RANGE, range);
}

esp_err_t bma400_set_odr(uint8_t odr)
{
    if (odr > BMA400_ODR_100HZ) {
        return ESP_ERR_INVALID_ARG;
    }
    
    s_bma400.config.odr = odr;
    
    // 写入 ODR 配置寄存器
    return i2c_write_reg(BMA400_REG_ODR, odr);
}

esp_err_t bma400_set_mode(uint8_t mode)
{
    if (mode > BMA400_MODE_SUSPEND) {
        return ESP_ERR_INVALID_ARG;
    }
    
    s_bma400.config.mode = mode;
    
    // 更新 PWR_CTRL 寄存器
    uint8_t pwr_ctrl = (mode == BMA400_MODE_SUSPEND) ? 0x00 : 0x01;
    return i2c_write_reg(BMA400_REG_PWR_CTRL, pwr_ctrl);
}

esp_err_t bma400_suspend(void)
{
    if (!s_bma400.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "进入暂停模式");
    return i2c_write_reg(BMA400_REG_PWR_CTRL, 0x00);
}

esp_err_t bma400_resume(void)
{
    if (!s_bma400.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "从暂停模式恢复");
    return i2c_write_reg(BMA400_REG_PWR_CTRL, 0x01);
}

esp_err_t bma400_deinit(void)
{
    if (!s_bma400.initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "去初始化 BMA400");
    
    // 关闭传感器
    i2c_write_reg(BMA400_REG_PWR_CTRL, 0x00);
    
    // 释放互斥锁
    if (s_bma400.mutex) {
        vSemaphoreDelete(s_bma400.mutex);
        s_bma400.mutex = NULL;
    }
    
    // 释放 I2C 驱动
    i2c_driver_delete(s_bma400.i2c_num);
    
    memset(&s_bma400, 0, sizeof(s_bma400));
    
    ESP_LOGI(TAG, "BMA400 已去初始化");
    return ESP_OK;
}