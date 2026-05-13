/**
 * @file hal_camera.h
 * @brief Hardware Abstraction Layer - 摄像头抽象接口
 *
 * 为上层算法与业务层提供统一的摄像头访问接口。
 * 底层驱动目标为 OV2640。
 *
 * @author BanSafe Team
 * @version 0.1.0
 * @date 2026-05-13
 */

#ifndef HAL_CAMERA_H
#define HAL_CAMERA_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 宏定义
 *============================================================================*/

/** 支持的图像格式 */
#define HAL_CAMERA_FMT_JPEG         (0)  /*!< JPEG 压缩 */
#define HAL_CAMERA_FMT_RGB565       (1)  /*!< RGB565 */
#define HAL_CAMERA_FMT_GRAYSCALE    (2)  /*!< 灰度图 */

/** 默认分辨率 */
#define HAL_CAMERA_RES_QQVGA        (0)  /*!< 160x120 */
#define HAL_CAMERA_RES_QVGA         (1)  /*!< 320x240 */
#define HAL_CAMERA_RES_VGA          (2)  /*!< 640x480 */
#define HAL_CAMERA_RES_SVGA         (3)  /*!< 800x600 */

/*============================================================================
 * 数据结构
 *============================================================================*/

/**
 * @brief 摄像头配置
 */
typedef struct {
    uint8_t  format;        /*!< 图像格式 (HAL_CAMERA_FMT_*) */
    uint8_t  resolution;    /*!< 分辨率 (HAL_CAMERA_RES_*) */
    uint8_t  jpeg_quality;  /*!< JPEG 质量 (0-63, 越低质量越好) */
    bool     flip_h;        /*!< 水平翻转 */
    bool     flip_v;        /*!< 垂直翻转 */
} hal_camera_config_t;

/**
 * @brief 图像帧数据
 */
typedef struct {
    uint8_t *buffer;        /*!< 图像数据缓冲 */
    size_t   length;        /*!< 数据长度 (bytes) */
    uint16_t width;         /*!< 图像宽度 */
    uint16_t height;        /*!< 图像高度 */
    uint8_t  format;        /*!< 图像格式 */
    uint32_t timestamp_ms;  /*!< 时间戳 (ms) */
} hal_camera_frame_t;

/*============================================================================
 * 公共接口
 *============================================================================*/

/**
 * @brief 初始化摄像头模块
 *
 * @param config 摄像头配置指针，NULL 则使用默认配置
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_camera_init(const hal_camera_config_t *config);

/**
 * @brief 反初始化摄像头，释放资源
 *
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_camera_deinit(void);

/**
 * @brief 捕获一帧图像
 *
 * 阻塞式调用，返回的 buffer 由调用方负责管理。
 * 当前规划阶段，此函数返回 ESP_ERR_NOT_SUPPORTED。
 *
 * @param[out] frame 帧数据输出指针
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_camera_capture(hal_camera_frame_t *frame);

/**
 * @brief 检查摄像头是否就绪
 *
 * @return true 就绪
 * @return false 未就绪
 */
bool hal_camera_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_CAMERA_H */