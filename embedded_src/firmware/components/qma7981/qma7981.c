/**
 * @file qma7981.c
 * @brief QMA7981 姿态传感器驱动实现
 * 
 * 6 轴 IMU (加速度计 + 陀螺仪) 驱动实现
 * 基于 ESP-IDF I2C 和 GPIO 驱动
 */

#include "qma7981.h"
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

/*============================================================================
 * 私有宏定义
 *============================================================================*/

#define TAG "QMA7981"

#define QMA7981_I2C_TIMEOUT_MS      (100)           /*!< I2C 超时时间 */

/** 传感器类型枚举 */
typedef enum {
    SENSOR_TYPE_UNKNOWN = 0,
    SENSOR_TYPE_QMA7981,
    SENSOR_TYPE_ICM42607,
    SENSOR_TYPE_ICM42688P,
} sensor_type_t;

/** 传感器类型名称数组 */
static const char *s_sensor_type_names[] = {
    "UNKNOWN",
    "QMA7981",
    "ICM-42607",
    "ICM-42688-P",
};

#define QMA7981_RESET_DELAY_MS      (10)            /*!< 复位后延迟 (增加至 10ms) */
#define QMA7981_INIT_DELAY_MS       (10)            /*!< 初始化后延迟 */
#define QMA7981_I2C_SCAN_RETRY      (3)             /*!< I2C 扫描重试次数 */

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
    qma7981_config_t config;              /*!< 当前配置 */
    SemaphoreHandle_t mutex;              /*!< 互斥锁 */
    StaticSemaphore_t mutex_buffer;       /*!< 互斥锁缓冲区 */
    sensor_type_t sensor_type;            /*!< 检测到的传感器类型 */
    uint8_t device_id;                    /*!< 实际设备 ID */
} qma7981_handle_t;

/*============================================================================
 * 私有全局变量
 *============================================================================*/

static qma7981_handle_t s_qma7981 = {0};

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
 * @brief I2C 扫描设备
 */
static void i2c_scan(i2c_port_t i2c_num);

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
        s_qma7981.i2c_num,
        s_qma7981.i2c_addr,
        buf,
        2,
        pdMS_TO_TICKS(QMA7981_I2C_TIMEOUT_MS)
    );
}

static esp_err_t i2c_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_master_write_read_device(
        s_qma7981.i2c_num,
        s_qma7981.i2c_addr,
        &reg, 1,
        value, 1,
        pdMS_TO_TICKS(QMA7981_I2C_TIMEOUT_MS)
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
        s_qma7981.i2c_num,
        s_qma7981.i2c_addr,
        buf, len + 1,
        pdMS_TO_TICKS(QMA7981_I2C_TIMEOUT_MS)
    );
    
    free(buf);
    return ret;
}

static esp_err_t i2c_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(
        s_qma7981.i2c_num,
        s_qma7981.i2c_addr,
        &reg, 1,
        data, len,
        pdMS_TO_TICKS(QMA7981_I2C_TIMEOUT_MS)
    );
}

/**
 * @brief I2C 扫描设备
 */
static void i2c_scan(i2c_port_t i2c_num)
{
    ESP_LOGI(TAG, "扫描 I2C 总线 %d...", i2c_num);
    
    for (int addr = 0; addr < 128; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  找到设备：0x%02X", addr);
        }
    }
}

/**
 * @brief 检查 I2C 设备是否存在
 */
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

esp_err_t qma7981_init(const qma7981_config_t *config)
{
    ESP_LOGI(TAG, "初始化 QMA7981...");
    
    if (!config) {
        ESP_LOGE(TAG, "配置参数为空");
        return ESP_ERR_INVALID_ARG;
    }
    
    // 检查是否已初始化
    if (s_qma7981.initialized) {
        ESP_LOGW(TAG, "QMA7981 已初始化，先去初始化");
        qma7981_deinit();
    }
    
    // 保存配置
    memcpy(&s_qma7981.config, config, sizeof(qma7981_config_t));
    s_qma7981.i2c_num = config->i2c_num;
    s_qma7981.i2c_addr = QMA7981_I2C_ADDR;
    s_qma7981.int_gpio = config->int_gpio;
    
    // 创建互斥锁
    s_qma7981.mutex = xSemaphoreCreateMutexStatic(&s_qma7981.mutex_buffer);
    if (!s_qma7981.mutex) {
        ESP_LOGE(TAG, "创建互斥锁失败");
        return ESP_ERR_NO_MEM;
    }
    
    // 配置 I2C 总线（仅当未配置时）
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_gpio,
        .scl_io_num = config->scl_gpio,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = QMA7981_I2C_FREQ,
    };
    
    esp_err_t ret = i2c_param_config(s_qma7981.i2c_num, &i2c_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C 配置失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = i2c_driver_install(s_qma7981.i2c_num, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C 驱动安装失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2C 总线配置成功 (SDA=%d, SCL=%d, %dkHz)",
             config->sda_gpio, config->scl_gpio, QMA7981_I2C_FREQ / 1000);
    
    // 扫描 I2C 总线，确认设备存在
    i2c_scan(s_qma7981.i2c_num);
    
    // 检查设备是否存在
    ret = i2c_check_device(s_qma7981.i2c_num, s_qma7981.i2c_addr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C 设备 0x%02X 未响应：%s", s_qma7981.i2c_addr, esp_err_to_name(ret));
        ESP_LOGE(TAG, "请检查：1. 接线是否正确 2. 上拉电阻是否连接 3. 设备地址是否正确");
        return ret;
    }
    ESP_LOGI(TAG, "I2C 设备 0x%02X 响应正常", s_qma7981.i2c_addr);
    
    // 软复位
    ret = qma7981_reset();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "复位失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    // 等待复位完成（增加延迟）
    vTaskDelay(pdMS_TO_TICKS(QMA7981_RESET_DELAY_MS));
    
    // 检查设备 ID - 支持多种传感器
    uint8_t device_id;
    ret = qma7981_read_device_id(&device_id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取设备 ID 失败：%s", esp_err_to_name(ret));
        // 继续执行，不立即失败
    }
    
    // 扫描所有可能的 ID 寄存器来识别传感器
    qma7981_scan_id_registers();
    
    // 检测传感器类型
    if (device_id == QMA7981_DEVID_VALUE) {
        s_qma7981.sensor_type = SENSOR_TYPE_QMA7981;
        ESP_LOGI(TAG, "检测到传感器：QMA7981 (ID: 0x%02X)", device_id);
    } else if (device_id == ICM42607_DEVID_VALUE) {
        s_qma7981.sensor_type = SENSOR_TYPE_ICM42607;
        ESP_LOGI(TAG, "检测到传感器：ICM-42607 (ID: 0x%02X)", device_id);
    } else if (device_id == ICM42688P_DEVID_VALUE) {
        s_qma7981.sensor_type = SENSOR_TYPE_ICM42688P;
        ESP_LOGI(TAG, "检测到传感器：ICM-42688-P (ID: 0x%02X)", device_id);
    } else if (device_id == 0x00) {
        // 0x00 可能是传感器未正确唤醒或地址问题，尝试继续
        ESP_LOGW(TAG, "设备 ID 为 0x00，尝试使用 ICM-42607 配置继续");
        s_qma7981.sensor_type = SENSOR_TYPE_ICM42607;  // 默认尝试 ICM-42607
    } else {
        ESP_LOGW(TAG, "未知设备 ID: 0x%02X，尝试继续初始化", device_id);
        s_qma7981.sensor_type = SENSOR_TYPE_UNKNOWN;
    }
    
    s_qma7981.device_id = device_id;
    
    // 配置加速度计
    ret = qma7981_set_accel_range(config->accel_range);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "配置加速度计量程失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = qma7981_set_accel_bandwidth(config->accel_bandwidth);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "配置加速度计带宽失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    // 配置陀螺仪
    ret = qma7981_set_gyro_range(config->gyro_range);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "配置陀螺仪量程失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    // 设置工作模式
    ret = qma7981_set_mode(config->mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置工作模式失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    // 初始化中断
    ret = qma7981_interrupt_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "中断初始化失败：%s (非致命错误)", esp_err_to_name(ret));
    }
    
    s_qma7981.initialized = true;
    
    ESP_LOGI(TAG, "QMA7981 初始化完成");
    return ESP_OK;
}

esp_err_t qma7981_init_default(void)
{
    qma7981_config_t config = {
        .i2c_num = QMA7981_I2C_NUM,
        .sda_gpio = QMA7981_I2C_SDA_GPIO,
        .scl_gpio = QMA7981_I2C_SCL_GPIO,
        .int_gpio = QMA7981_INT_GPIO,
        .accel_range = QMA7981_RANGE_4G,
        .accel_bandwidth = QMA7981_BW_125HZ,
        .gyro_range = 2,  // ±500 dps
        .mode = QMA7981_MODE_ACCEL_GYRO,
    };
    
    return qma7981_init(&config);
}

esp_err_t qma7981_read_device_id(uint8_t *device_id)
{
    if (!device_id) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return i2c_read_reg(QMA7981_DEVID_ADDR, device_id);
}

/**
 * @brief 读取任意寄存器（用于调试）
 */
esp_err_t qma7981_read_raw_reg(uint8_t reg, uint8_t *value)
{
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_read_reg(reg, value);
}

/**
 * @brief 扫描传感器 ID 寄存器（调试用）
 */
void qma7981_scan_id_registers(void)
{
    ESP_LOGI(TAG, "=== 扫描传感器 ID 寄存器 ===");
    
    // 尝试常见的 ID 寄存器地址
    uint8_t id_registers[] = {0x00, 0x0F, 0x75, 0x6F, 0xD0, 0x01, 0x02, 0x03};
    const char *reg_names[] = {
        "0x00 (QMA7981 DEVID)",
        "0x0F (常见 WHO_AM_I)",
        "0x75 (ICM-426xx WHO_AM_I)",
        "0x6F (ICM-426xx RSV_WHO_ID)",
        "0xD0 (BMI088 DEVID)",
        "0x01 (BMA42x CHIP_ID)",
        "0x02",
        "0x03"
    };
    
    for (int i = 0; i < 8; i++) {
        uint8_t value;
        esp_err_t ret = i2c_read_reg(id_registers[i], &value);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "寄存器 %s = 0x%02X", reg_names[i], value);
        } else {
            ESP_LOGW(TAG, "寄存器 %s 读取失败：%s", reg_names[i], esp_err_to_name(ret));
        }
    }
    
    // 扫描连续寄存器
    ESP_LOGI(TAG, "=== 扫描连续寄存器 0x00-0x1F ===");
    uint8_t buffer[32];
    esp_err_t ret = i2c_read_regs(0x00, buffer, 32);
    if (ret == ESP_OK) {
        for (int i = 0; i < 32; i++) {
            if (buffer[i] != 0x00 && buffer[i] != 0xFF) {
                ESP_LOGI(TAG, "  0x%02X = 0x%02X", i, buffer[i]);
            }
        }
    } else {
        ESP_LOGW(TAG, "连续寄存器读取失败：%s", esp_err_to_name(ret));
    }
    
    // 扫描 0x70-0x7F
    ESP_LOGI(TAG, "=== 扫描寄存器 0x70-0x7F ===");
    ret = i2c_read_regs(0x70, buffer, 16);
    if (ret == ESP_OK) {
        for (int i = 0; i < 16; i++) {
            if (buffer[i] != 0x00 && buffer[i] != 0xFF) {
                ESP_LOGI(TAG, "  0x%02X = 0x%02X", 0x70 + i, buffer[i]);
            }
        }
    } else {
        ESP_LOGW(TAG, "寄存器 0x70-0x7F 读取失败：%s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "=== 扫描完成 ===");
}

bool qma7981_is_ready(void)
{
    if (!s_qma7981.initialized) {
        return false;
    }
    
    uint8_t device_id;
    esp_err_t ret = qma7981_read_device_id(&device_id);
    return (ret == ESP_OK && device_id == QMA7981_DEVID_VALUE);
}

esp_err_t qma7981_reset(void)
{
    ESP_LOGD(TAG, "执行软复位");
    return i2c_write_reg(QMA7981_CTRL_ADDR, QMA7981_CTRL_RESET);
}

esp_err_t qma7981_set_accel_range(uint8_t range)
{
    if (range > QMA7981_RANGE_16G) {
        return ESP_ERR_INVALID_ARG;
    }
    
    s_qma7981.config.accel_range = range;
    
    // QMA7981 加速度计量程配置寄存器
    // 注意：实际寄存器地址和位配置需参考数据手册
    // 此处为示例代码
    ESP_LOGD(TAG, "设置加速度计量程：%dg", 1 << (range + 1));
    return ESP_OK;
}

esp_err_t qma7981_set_accel_bandwidth(uint8_t bandwidth)
{
    if (bandwidth > QMA7981_BW_1000HZ) {
        return ESP_ERR_INVALID_ARG;
    }
    
    s_qma7981.config.accel_bandwidth = bandwidth;
    
    ESP_LOGD(TAG, "设置加速度计带宽配置：%d", bandwidth);
    return ESP_OK;
}

esp_err_t qma7981_set_gyro_range(uint8_t range)
{
    if (range > 4) {
        return ESP_ERR_INVALID_ARG;
    }
    
    s_qma7981.config.gyro_range = range;
    
    int dps_values[] = {125, 250, 500, 1000, 2000};
    ESP_LOGD(TAG, "设置陀螺仪量程：±%d dps", dps_values[range]);
    return ESP_OK;
}

esp_err_t qma7981_set_mode(uint8_t mode)
{
    if (mode > QMA7981_MODE_ACCEL_GYRO) {
        return ESP_ERR_INVALID_ARG;
    }
    
    s_qma7981.config.mode = mode;
    
    const char *mode_names[] = {"STANDBY", "ACCEL", "GYRO", "ACCEL_GYRO"};
    ESP_LOGD(TAG, "设置工作模式：%s", mode_names[mode]);
    return ESP_OK;
}

esp_err_t qma7981_read_accel(qma7981_accel_data_t *data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_qma7981.initialized) {
        ESP_LOGE(TAG, "设备未初始化");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 读取加速度数据 (6 字节)
    uint8_t buffer[6];
    esp_err_t ret = i2c_read_regs(QMA7981_ACC_X_LSB_ADDR, buffer, 6);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取加速度数据失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    // 组合 16 位数据 (小端格式)
    data->x = (int16_t)((buffer[1] << 8) | buffer[0]);
    data->y = (int16_t)((buffer[3] << 8) | buffer[2]);
    data->z = (int16_t)((buffer[5] << 8) | buffer[4]);
    
    ESP_LOGD(TAG, "加速度：X=%d, Y=%d, Z=%d mg", data->x, data->y, data->z);
    return ESP_OK;
}

esp_err_t qma7981_read_gyro(qma7981_gyro_data_t *data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_qma7981.initialized) {
        ESP_LOGE(TAG, "设备未初始化");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 读取陀螺仪数据 (6 字节)
    uint8_t buffer[6];
    esp_err_t ret = i2c_read_regs(QMA7981_GYRO_X_LSB_ADDR, buffer, 6);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取陀螺仪数据失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    // 组合 16 位数据 (小端格式)
    data->x = (int16_t)((buffer[1] << 8) | buffer[0]);
    data->y = (int16_t)((buffer[3] << 8) | buffer[2]);
    data->z = (int16_t)((buffer[5] << 8) | buffer[4]);
    
    ESP_LOGD(TAG, "陀螺仪：X=%d, Y=%d, Z=%d mdps", data->x, data->y, data->z);
    return ESP_OK;
}

esp_err_t qma7981_read_all_data(qma7981_sensor_data_t *data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret;
    
    ret = qma7981_read_accel(&data->accel);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = qma7981_read_gyro(&data->gyro);
    if (ret != ESP_OK) {
        return ret;
    }
    
    data->timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000);
    
    return ESP_OK;
}

esp_err_t qma7981_config_motion_interrupt(uint8_t threshold, uint16_t duration)
{
    if (!s_qma7981.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "配置运动检测中断：阈值=%d, 持续时间=%dms", threshold, duration);
    
    // 注意：实际寄存器配置需参考 QMA7981 数据手册
    // 此处为示例代码框架
    
    return ESP_OK;
}

esp_err_t qma7981_interrupt_init(void)
{
    ESP_LOGI(TAG, "初始化 GPIO 中断 (GPIO%d)", QMA7981_INT_GPIO);
    
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << QMA7981_INT_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,  // 下降沿触发
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO 配置失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    // 安装 GPIO 中断服务
    ret = gpio_install_isr_service(0);
    if (ret == ESP_ERR_INVALID_STATE) {
        // ISR 服务已安装，忽略此错误
        ESP_LOGD(TAG, "GPIO ISR 服务已安装");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "安装 GPIO ISR 服务失败：%s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "GPIO 中断初始化完成");
    return ESP_OK;
}

esp_err_t qma7981_interrupt_remove(void)
{
    if (!s_qma7981.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    gpio_isr_handler_remove(s_qma7981.int_gpio);
    ESP_LOGD(TAG, "GPIO 中断已移除");
    return ESP_OK;
}

esp_err_t qma7981_enter_standby(void)
{
    if (!s_qma7981.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "进入待机模式");
    return qma7981_set_mode(QMA7981_MODE_STANDBY);
}

esp_err_t qma7981_enter_deep_sleep(void)
{
    if (!s_qma7981.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "进入深度休眠模式");
    // 深度休眠可能需要特定的寄存器配置
    // 此处为示例
    return qma7981_set_mode(QMA7981_MODE_STANDBY);
}

esp_err_t qma7981_wake_up(void)
{
    if (!s_qma7981.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "退出休眠模式");
    return qma7981_set_mode(s_qma7981.config.mode);
}

esp_err_t qma7981_deinit(void)
{
    if (!s_qma7981.initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "去初始化 QMA7981");
    
    // 移除中断
    qma7981_interrupt_remove();
    
    // 释放互斥锁
    if (s_qma7981.mutex) {
        vSemaphoreDelete(s_qma7981.mutex);
        s_qma7981.mutex = NULL;
    }
    
    // 释放 I2C 驱动
    i2c_driver_delete(s_qma7981.i2c_num);
    
    memset(&s_qma7981, 0, sizeof(s_qma7981));
    
    ESP_LOGI(TAG, "QMA7981 已去初始化");
    return ESP_OK;
}