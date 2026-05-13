# BanSafe API 设计文档

> 版本: 0.1.0 | 日期: 2026-05-13

---

## 1. 概述

本文档描述 BanSafe 系统中各层之间的内部 API 接口约定。

---

## 2. HAL 接口

### 2.1 传感器 HAL (`hal_sensor.h`)

```c
/* 数据结构 */
typedef struct {
    int16_t x;  /* X 轴加速度 (mg) */
    int16_t y;  /* Y 轴加速度 (mg) */
    int16_t z;  /* Z 轴加速度 (mg) */
} hal_accel_data_t;

typedef struct {
    int16_t x;  /* X 轴角速度 (mdps) */
    int16_t y;  /* Y 轴角速度 (mdps) */
    int16_t z;  /* Z 轴角速度 (mdps) */
} hal_gyro_data_t;

/* 接口函数 */
esp_err_t hal_sensor_init(void);
esp_err_t hal_sensor_read_accel(hal_accel_data_t *data);
esp_err_t hal_sensor_read_gyro(hal_gyro_data_t *data);
```

### 2.2 摄像头 HAL (`hal_camera.h`) — 规划中

```c
typedef struct {
    int width;           /* 图像宽度 */
    int height;          /* 图像高度 */
    int format;          /* 像素格式 */
    void *buffer;        /* 帧缓冲指针 */
    size_t buffer_size;  /* 缓冲大小 */
} hal_camera_config_t;

typedef struct {
    uint8_t *data;       /* 像素数据 */
    uint32_t size;       /* 数据大小 */
    uint32_t timestamp;  /* 时间戳 (ms) */
} hal_camera_frame_t;

esp_err_t hal_camera_init(const hal_camera_config_t *config);
esp_err_t hal_camera_capture(hal_camera_frame_t *frame);
```

### 2.3 音频 HAL (`hal_audio.h`) — 规划中

```c
typedef struct {
    int sample_rate;     /* 采样率 (Hz) */
    int bits_per_sample; /* 位深度 */
    int channels;        /* 声道数 */
} hal_audio_config_t;

typedef struct {
    int16_t *samples;    /* PCM 样本 */
    uint32_t count;      /* 样本数量 */
    uint32_t timestamp;  /* 时间戳 (ms) */
} hal_audio_frame_t;

esp_err_t hal_audio_init(const hal_audio_config_t *config);
esp_err_t hal_audio_capture(hal_audio_frame_t *frame);
```

---

## 3. 算法引擎接口

### 3.1 安全引擎 (`safety_engine.h`)

```c
/* 数据结构 */
typedef enum {
    SAFETY_STAGE_IDLE    = 0,
    SAFETY_STAGE_POSTURE = 1,
    SAFETY_STAGE_AUDIO   = 2,
    SAFETY_STAGE_VISUAL  = 3,
    SAFETY_STAGE_ALERT   = 4,
} safety_stage_t;

typedef struct {
    safety_stage_t stage;
    int            posture_score;
    int            audio_score;
    int            visual_score;
    bool           alert_triggered;
    uint32_t       timestamp_ms;
} safety_result_t;

/* 接口函数 */
esp_err_t safety_engine_init(void);
int       safety_engine_evaluate_posture(void);
int       safety_engine_evaluate_audio(void);    /* 规划中 */
int       safety_engine_evaluate_visual(void);   /* 规划中 */
esp_err_t safety_engine_run(safety_result_t *result);
```

---

## 4. 后端 API（规划中）

> 后端 API 尚未实现。以下为规划中的 RESTful API 端点。

| 方法 | 端点 | 说明 |
|------|------|------|
| POST | `/api/v1/alerts` | 接收设备报警 |
| GET | `/api/v1/status/:device_id` | 查询设备状态 |
| GET | `/api/v1/logs/:device_id` | 查询历史日志 |
| POST | `/api/v1/ota` | 触发 OTA 升级 |

---

## 5. 错误码约定

| 返回值 | 含义 |
|--------|------|
| `ESP_OK` (0) | 成功 |
| `ESP_ERR_NOT_SUPPORTED` | 功能未实现/占位 |
| `ESP_ERR_INVALID_ARG` | 参数无效 |
| `ESP_ERR_TIMEOUT` | 操作超时 |
| `ESP_FAIL` | 一般性失败 |
| 负值 (非 ESP 标准) | 评分函数专用，表示评估失败 |