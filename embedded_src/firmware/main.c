/**
 * @file main.c
 * @brief ESP32-S3-EYE 实时时钟显示
 * 
 * 基于 ST7789 LCD 显示屏的时钟应用
 * 显示类似智能手表的圆形时钟界面
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

// 标签
static const char *TAG = "Clock";

// ========== LCD 引脚配置 ==========
#define LCD_HOST      HSPI_HOST
#define PIN_NUM_MOSI  7
#define PIN_NUM_SCLK  8
#define PIN_NUM_CS    5
#define PIN_NUM_DC    6
#define PIN_NUM_RST   15
#define PIN_NUM_BCKL  38

// ========== LCD 参数 ==========
#define LCD_WIDTH     240
#define LCD_HEIGHT    240
#define LCD_MAX_ROTATION 4

// ST7789 命令定义
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID   0x04
#define ST7789_RDDST   0x09

#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_PON     0x3A
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E
#define ST7789_PTLAR   0x30
#define ST7789_COLMOD  0x3A

// 颜色定义
#define ST7789_COLOR_RGB 0x00

// 显示旋转方向
#define LCD_DISPLAY_ORIENTATION_0   0   // 0 度
#define LCD_DISPLAY_ORIENTATION_90  1   // 90 度
#define LCD_DISPLAY_ORIENTATION_180 2   // 180 度
#define LCD_DISPLAY_ORIENTATION_270 3   // 270 度

// 时钟颜色
#define COLOR_BLACK          0x0000
#define COLOR_WHITE          0xFFFF
#define COLOR_RED            0xF800
#define COLOR_GREEN          0x0400
#define COLOR_BLUE           0x001F
#define COLOR_CYAN           0x07FF
#define COLOR_MAGENTA        0xF81F
#define COLOR_YELLOW         0xFFE0
#define COLOR_GRAY           0x8410
#define COLOR_LIGHT_GRAY     0xCECE
#define COLOR_DARK_GRAY      0x3284
#define COLOR_BG             0x0000  // 背景色

// 时钟布局参数
#define CLOCK_CENTER_X       120
#define CLOCK_CENTER_Y       120
#define CLOCK_RADIUS         100
#define CLOCK_OUTER_RADIUS   105
#define CLOCK_INNER_RADIUS   85
#define CLOCK_HOUR_HAND_LEN  55
#define CLOCK_MIN_HAND_LEN   75
#define CLOCK_SEC_HAND_LEN   85
#define CLOCK_NUM_RADIUS     88
#define CLOCK_TIME_Y         185
#define CLOCK_DATE_Y         205

// ========== 缓冲区定义 ==========
static uint16_t lcd_frame_buffer[LCD_WIDTH * LCD_HEIGHT];

// ========== 函数声明 ==========
static esp_err_t lcd_spi_write(const uint8_t *data, size_t len);
static void lcd_cmd(uint8_t cmd);
static void lcd_data(const uint8_t *data, size_t len);
static void lcd_write_color(uint16_t color, int len);
static esp_err_t lcd_init(void);
static void lcd_set_rotation(uint8_t r);
static void lcd_fill(int x, int y, int w, int h, uint16_t color);
static void lcd_draw_pixel(int x, int y, uint16_t color);
static void lcd_draw_circle(int x, int y, int r, uint16_t color);
static void lcd_fill_circle(int x, int y, int r, uint16_t color);
static void lcd_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
static void lcd_draw_char(int x, int y, char c, uint16_t color, uint16_t bg);
static void lcd_draw_string(int x, int y, const char *str, uint16_t color, uint16_t bg);
static void clock_draw(void);
static void clock_draw_bg(void);
static void clock_draw_numbers(void);
static void clock_draw_hand(float angle, int len, uint16_t color, int width);
static esp_err_t spi_bus_init(void);
static esp_err_t nvs_time_init(void);
static esp_err_t get_system_time(struct tm *timeinfo);
static void clock_task(void *pvParameters);

// ========== SPI LCD 驱动 ==========

static esp_err_t lcd_spi_write(const uint8_t *data, size_t len)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    return spi_device_transmit(LCD_HOST, &t);
}

static void lcd_cmd(uint8_t cmd)
{
    gpio_set_level(PIN_NUM_DC, 0);
    uint8_t data[1] = {cmd};
    lcd_spi_write(data, 1);
}

static void lcd_data(const uint8_t *data, size_t len)
{
    gpio_set_level(PIN_NUM_DC, 1);
    lcd_spi_write(data, len);
}

static void lcd_write_color(uint16_t color, int len)
{
    gpio_set_level(PIN_NUM_DC, 1);
    
    uint8_t data[2];
    for (int i = 0; i < len; i++) {
        data[0] = (color >> 8) & 0xFF;
        data[1] = color & 0xFF;
        lcd_spi_write(data, 2);
    }
}

esp_err_t lcd_init(void)
{
    ESP_LOGI(TAG, "初始化 ST7789 LCD");
    
    // 复位 LCD
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 发送复位命令
    lcd_cmd(ST7789_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));
    
    // 关闭睡眠
    lcd_cmd(ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));
    
    // 内存数据访问控制
    uint8_t colmod[1] = {0x55}; // 16-bit/pixel
    lcd_cmd(ST7789_COLMOD);
    lcd_data(colmod, 1);
    
    // 显示开启
    lcd_cmd(ST7789_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 设置显示区域
    uint8_t col_start[4] = {0, 0, 0, 160};
    uint8_t row_start[4] = {0, 0, 0, 120};
    
    lcd_cmd(ST7789_CASET);
    lcd_data(col_start, 4);
    
    lcd_cmd(ST7789_RASET);
    lcd_data(row_start, 4);
    
    lcd_cmd(ST7789_RAMWR);
    
    ESP_LOGI(TAG, "LCD 初始化完成");
    return ESP_OK;
}

void lcd_set_rotation(uint8_t r)
{
    // ST7789 不需要旋转命令，直接调整坐标即可
}

void lcd_fill(int x, int y, int w, int h, uint16_t color)
{
    if (x < 0 || y < 0 || w <= 0 || h <= 0) return;
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    // 裁剪
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    
    // 设置窗口
    uint8_t col_start[4] = {0, 0, (uint8_t)x, (uint8_t)(x + w - 1)};
    uint8_t row_start[4] = {0, 0, (uint8_t)y, (uint8_t)(y + h - 1)};
    
    lcd_cmd(ST7789_CASET);
    lcd_data(col_start, 4);
    
    lcd_cmd(ST7789_RASET);
    lcd_data(row_start, 4);
    
    lcd_cmd(ST7789_RAMWR);
    
    gpio_set_level(PIN_NUM_DC, 1);
    
    uint8_t data[2];
    data[0] = (color >> 8) & 0xFF;
    data[1] = color & 0xFF;
    
    size_t total = w * h;
    size_t chunk = 4096; // 每次最多4096字节
    
    for (size_t i = 0; i < total; i += chunk / 2) {
        size_t send = (total - i) > (chunk / 2) ? (chunk / 2) : (total - i);
        lcd_spi_write(data, send);
    }
}

void lcd_draw_pixel(int x, int y, uint16_t color)
{
    if (x < 0 || y < 0 || x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    uint8_t col_start[4] = {0, 0, (uint8_t)x, (uint8_t)x};
    uint8_t row_start[4] = {0, 0, (uint8_t)y, (uint8_t)y};
    
    lcd_cmd(ST7789_CASET);
    lcd_data(col_start, 4);
    
    lcd_cmd(ST7789_RASET);
    lcd_data(row_start, 4);
    
    lcd_cmd(ST7789_RAMWR);
    
    lcd_write_color(color, 1);
}

void lcd_draw_circle(int x, int y, int r, uint16_t color)
{
    int fx = 1, fy = 0, err = 1 - r, e2;
    
    for (; fy <= r; fy++) {
        lcd_draw_pixel(x + r, y + fy, color);
        lcd_draw_pixel(x - r, y + fy, color);
        lcd_draw_pixel(x + fy, y + r, color);
        lcd_draw_pixel(x - fy, y + r, color);
        lcd_draw_pixel(x + r, y - fy, color);
        lcd_draw_pixel(x - r, y - fy, color);
        lcd_draw_pixel(x + fy, y - r, color);
        lcd_draw_pixel(x - fy, y - r, color);
        
        e2 = 2 * err;
        if (e2 + fx <= 2 * r && e2 != -2 * r) {
            err += fx++;
        }
        if (e2 - fy <= 2 * r && e2 != 2 * r) {
            err -= fy++;
        }
    }
}

void lcd_fill_circle(int x, int y, int r, uint16_t color)
{
    for (int i = -r; i <= r; i++) {
        for (int j = -r; j <= r; j++) {
            if (i * i + j * j <= r * r) {
                lcd_draw_pixel(x + i, y + j, color);
            }
        }
    }
}

void lcd_draw_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        lcd_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void lcd_draw_string(int x, int y, const char *str, uint16_t color, uint16_t bg)
{
    // 使用简单的5x7字体
    extern const uint8_t font5x7[][5];
    extern const char font5x7_width[128];
    
    while (*str) {
        uint8_t c = *str;
        if (c >= 32 && c < 128) {
            uint8_t font_data[5];
            memcpy(font_data, font5x7[c - 32], 5);
            
            for (int i = 0; i < 5; i++) {
                uint8_t bits = font_data[i];
                for (int j = 0; j < 5; j++) {
                    if (bits & (1 << (4 - j))) {
                        lcd_fill(x + i * 1, y + j, 1, 1, color);
                    } else {
                        lcd_fill(x + i * 1, y + j, 1, 1, bg);
                    }
                }
            }
            x += font5x7_width[c] + 1;
        }
        str++;
    }
}

// ========== 简易数字字体 (8x16) ==========
static const uint8_t digit_font[10][16] = {
    // 0
    {
        0x00, 0x7C, 0xC6, 0xCE, 0xFE, 0xC6, 0xC6, 0xC6,
        0xC6, 0xC6, 0xC6, 0xFE, 0xCE, 0xC6, 0x7C, 0x00
    },
    // 1
    {
        0x00, 0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00, 0x00
    },
    // 2
    {
        0x00, 0x7C, 0xC6, 0x06, 0x0C, 0x18, 0x30, 0x60,
        0x60, 0x60, 0xC0, 0xC0, 0xCE, 0xFE, 0x06, 0x00
    },
    // 3
    {
        0x00, 0x7C, 0xC6, 0x06, 0x0C, 0x18, 0x30, 0x30,
        0x30, 0x18, 0x0C, 0x06, 0x06, 0xC6, 0x7C, 0x00
    },
    // 4
    {
        0x00, 0x38, 0x6C, 0x6C, 0xCC, 0xCC, 0xCC, 0xCC,
        0xFC, 0xCC, 0xCC, 0xCC, 0x0C, 0x7E, 0x00, 0x00
    },
    // 5
    {
        0x00, 0xFE, 0xC0, 0xC0, 0xC0, 0xFC, 0xC6, 0xC6,
        0xC6, 0xC6, 0xC6, 0xC6, 0x06, 0x7C, 0x00, 0x00
    },
    // 6
    {
        0x00, 0x38, 0x60, 0xC0, 0xC0, 0xFC, 0xCE, 0xCE,
        0xCE, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00, 0x00
    },
    // 7
    {
        0x00, 0xFE, 0xC6, 0x66, 0x36, 0x1C, 0x0C, 0x0C,
        0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x00
    },
    // 8
    {
        0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x38,
        0x38, 0x38, 0x6C, 0xC6, 0xC6, 0xC6, 0x7C, 0x00
    },
    // 9
    {
        0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x6C, 0x3C, 0x3C,
        0x3C, 0x06, 0x0C, 0x6C, 0xC6, 0x7E, 0x30, 0x00
    }
};

void lcd_draw_digit(int x, int y, int digit, uint16_t color, uint16_t bg)
{
    if (digit < 0 || digit > 9) return;
    
    const uint8_t *font = digit_font[digit];
    
    for (int row = 0; row < 16; row++) {
        uint16_t pixel = font[row];
        for (int col = 0; col < 8; col++) {
            if (pixel & (1 << col)) {
                lcd_draw_pixel(x + col, y + row, color);
            } else {
                lcd_draw_pixel(x + col, y + row, bg);
            }
        }
    }
}

void lcd_draw_number(int x, int y, int num, uint16_t color, uint16_t bg)
{
    char str[12];
    snprintf(str, sizeof(str), "%d", num);
    
    int len = strlen(str);
    int start_x = x - (len * 9) / 2;
    
    for (int i = 0; i < len; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            lcd_draw_digit(start_x + i * 9, y, str[i] - '0', color, bg);
        }
    }
}

void lcd_draw_time(int x, int y, int hour, int minute, int second)
{
    char time_str[12];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hour, minute);
    
    int len = strlen(time_str);
    int start_x = x - (len * 10) / 2;
    
    for (int i = 0; i < len; i++) {
        if (time_str[i] == ':') {
            lcd_draw_digit(start_x + i * 10, y, 10, COLOR_WHITE, COLOR_BLACK);
        } else if (time_str[i] >= '0' && time_str[i] <= '9') {
            lcd_draw_digit(start_x + i * 10, y, time_str[i] - '0', COLOR_WHITE, COLOR_BLACK);
        }
    }
}

// ========== 时钟绘制函数 ==========

void clock_draw_bg(void)
{
    // 绘制背景
    lcd_fill(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
    
    // 绘制外圆阴影
    lcd_fill_circle(CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_OUTER_RADIUS + 3, COLOR_DARK_GRAY);
    
    // 绘制外圆
    lcd_fill_circle(CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_OUTER_RADIUS, COLOR_BLACK);
    
    // 绘制边框
    lcd_draw_circle(CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_OUTER_RADIUS, COLOR_GRAY);
    lcd_draw_circle(CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIUS, COLOR_GRAY);
}

void clock_draw_numbers(void)
{
    // 绘制时钟数字
    float angles[] = {270, 300, 330, 0, 30, 60, 90, 120, 150, 180, 210, 240};
    int numbers[] = {12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    
    for (int i = 0; i < 12; i++) {
        float angle_rad = angles[i] * M_PI / 180.0;
        int x = CLOCK_CENTER_X + (int)(cos(angle_rad) * CLOCK_NUM_RADIUS);
        int y = CLOCK_CENTER_Y + (int)(sin(angle_rad) * CLOCK_NUM_RADIUS);
        
        lcd_draw_number(x, y - 8, numbers[i], COLOR_WHITE, COLOR_BLACK);
    }
    
    // 绘制刻度线
    for (int i = 0; i < 60; i++) {
        float angle_rad = i * 6 * M_PI / 180.0;
        int is_hour = (i % 5 == 0);
        int inner_r = is_hour ? 92 : 95;
        int outer_r = 100;
        
        int x0 = CLOCK_CENTER_X + (int)(cos(angle_rad) * inner_r);
        int y0 = CLOCK_CENTER_Y + (int)(sin(angle_rad) * inner_r);
        int x1 = CLOCK_CENTER_X + (int)(cos(angle_rad) * outer_r);
        int y1 = CLOCK_CENTER_Y + (int)(sin(angle_rad) * outer_r);
        
        lcd_draw_line(x0, y0, x1, y1, is_hour ? COLOR_WHITE : COLOR_GRAY);
    }
}

void clock_draw_hand(float angle, int len, uint16_t color, int width)
{
    float angle_rad = angle * M_PI / 180.0;
    int x1 = CLOCK_CENTER_X + (int)(cos(angle_rad) * len);
    int y1 = CLOCK_CENTER_Y + (int)(sin(angle_rad) * len);
    
    // 绘制指针（带宽度）
    for (int w = -width/2; w <= width/2; w++) {
        lcd_draw_line(CLOCK_CENTER_X, CLOCK_CENTER_Y, x1 + w, y1 + w, color);
    }
}

void clock_draw(void)
{
    struct tm timeinfo;
    if (get_system_time(&timeinfo) != ESP_OK) {
        return;
    }
    
    // 计算角度
    float sec_angle = timeinfo.tm_sec * 6.0;
    float min_angle = timeinfo.tm_min * 6.0 + timeinfo.tm_sec * 0.1;
    float hour_angle = (timeinfo.tm_hour % 12) * 30.0 + timeinfo.tm_min * 0.5;
    
    // 清屏
    lcd_fill(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
    
    // 绘制时钟背景
    clock_draw_bg();
    
    // 绘制数字和刻度
    clock_draw_numbers();
    
    // 绘制指针（秒针 -> 分针 -> 时针）
    lcd_draw_hand(sec_angle, CLOCK_SEC_HAND_LEN, COLOR_RED, 2);
    lcd_draw_hand(min_angle, CLOCK_MIN_HAND_LEN, COLOR_WHITE, 3);
    lcd_draw_hand(hour_angle, CLOCK_HOUR_HAND_LEN, COLOR_WHITE, 4);
    
    // 中心点
    lcd_fill_circle(CLOCK_CENTER_X, CLOCK_CENTER_Y, 4, COLOR_WHITE);
    lcd_fill_circle(CLOCK_CENTER_X, CLOCK_CENTER_Y, 2, COLOR_RED);
    
    // 显示日期
    char date_str[20];
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", 
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    
    // 简单绘制日期字符串
    lcd_fill(CLOCK_CENTER_X - 50, CLOCK_DATE_Y - 8, 100, 16, COLOR_DARK_GRAY);
    lcd_draw_string(CLOCK_CENTER_X - 45, CLOCK_DATE_Y - 6, date_str, COLOR_WHITE, COLOR_DARK_GRAY);
    
    // 显示星期
    const char *weekdays[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
    lcd_draw_string(CLOCK_CENTER_X - 15, CLOCK_TIME_Y - 10, weekdays[timeinfo.tm_wday], COLOR_CYAN, COLOR_BLACK);
}

// ========== SPI 总线初始化 ==========

esp_err_t spi_bus_init(void)
{
    ESP_LOGI(TAG, "初始化 SPI 总线");
    
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
    
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 40000000, // 40MHz
        .mode = 0,
        .queue_size = 7,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    
    esp_err_t ret = spi_bus_add_device(LCD_HOST, &dev_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "添加 SPI 设备失败");
        return ret;
    }
    
    // 初始化控制引脚
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST) | (1ULL << PIN_NUM_BCKL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    // 开启背光
    gpio_set_level(PIN_NUM_BCKL, 1);
    
    ESP_LOGI(TAG, "SPI 总线初始化完成");
    return ESP_OK;
}

// ========== NVS 时间存储 ==========

esp_err_t nvs_time_init(void)
{
    ESP_LOGI(TAG, "初始化 NVS 时间存储");
    return ESP_OK;
}

esp_err_t get_system_time(struct tm *timeinfo)
{
    time_t now = time(NULL);
    
    // 尝试从 NTP 获取时间
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    time_t now_time = tv.tv_sec;
    localtime_r(&now_time, timeinfo);
    
    // 如果时间是从1970年开始的，使用默认时间
    if (timeinfo->tm_year < 120) {  // 1970 + 120 = 2090 是合理的
        // 设置默认时间：2024-01-01 12:00:00
        timeinfo->tm_year = 124;  // 2024 - 1900
        timeinfo->tm_mon = 0;     // January
        timeinfo->tm_mday = 1;
        timeinfo->tm_hour = 12;
        timeinfo->tm_min = 0;
        timeinfo->tm_sec = 0;
        timeinfo->tm_wday = 1;    // Monday
    }
    
    return ESP_OK;
}

// ========== 时钟任务 ==========

void clock_task(void *pvParameters)
{
    ESP_LOGI(TAG, "时钟任务已启动");
    
    lcd_init();
    
    // 初始绘制
    clock_draw();
    
    int last_sec = -1;
    
    while (1) {
        struct tm timeinfo;
        get_system_time(&timeinfo);
        
        // 只在秒数变化时重绘
        if (timeinfo.tm_sec != last_sec) {
            clock_draw();
            last_sec = timeinfo.tm_sec;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms 检查一次
    }
}

// ========== 主函数 ==========

void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "  ESP32-S3-EYE 实时时钟");
    ESP_LOGI(TAG, "  版本: 1.0.0");
    ESP_LOGI(TAG, "=================================");
    
    // 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 初始化 SPI 总线
    ESP_ERROR_CHECK(spi_bus_init());
    
    // 初始化 NVS 时间
    ESP_ERROR_CHECK(nvs_time_init());
    
    // 创建时钟任务
    xTaskCreate(clock_task, "clock", 8192, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "时钟系统已启动");
}