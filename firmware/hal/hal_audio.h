/**
 * @file hal_audio.h
 * @brief Hardware Abstraction Layer - 麦克风抽象接口
 *
 * 为上层算法与业务层提供统一的音频采集接口。
 * 底层驱动目标为 INMP441 MEMS 数字麦克风（I2S 接口）。
 *
 * @author BanSafe Team
 * @version 0.1.0
 * @date 2026-05-13
 */

#ifndef HAL_AUDIO_H
#define HAL_AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 宏定义
 *============================================================================*/

/** 采样率选项 */
#define HAL_AUDIO_SR_8K      (8000)   /*!< 8 kHz */
#define HAL_AUDIO_SR_16K     (16000)  /*!< 16 kHz (默认) */
#define HAL_AUDIO_SR_44K1    (44100)  /*!< 44.1 kHz */

/** 采样位深 */
#define HAL_AUDIO_BITS_16    (16)     /*!< 16-bit */
#define HAL_AUDIO_BITS_24    (24)     /*!< 24-bit */
#define HAL_AUDIO_BITS_32    (32)     /*!< 32-bit */

/*============================================================================
 * 数据结构
 *============================================================================*/

/**
 * @brief 麦克风配置
 */
typedef struct {
    uint32_t sample_rate;       /*!< 采样率 (Hz) */
    uint8_t  bits_per_sample;   /*!< 位深 (HAL_AUDIO_BITS_*) */
    uint8_t  channels;          /*!< 声道数 (1=单声道) */
    uint16_t buffer_size_ms;    /*!< 每帧缓冲时长 (ms) */
} hal_audio_config_t;

/**
 * @brief 音频帧数据
 */
typedef struct {
    int16_t *buffer;            /*!< PCM 音频数据缓冲 */
    size_t   length;            /*!< 采样点数 */
    uint32_t sample_rate;       /*!< 实际采样率 */
    uint32_t timestamp_ms;      /*!< 时间戳 (ms) */
} hal_audio_frame_t;

/*============================================================================
 * 公共接口
 *============================================================================*/

/**
 * @brief 初始化麦克风模块
 *
 * @param config 麦克风配置指针，NULL 则使用默认配置 (16kHz/16bit/mono)
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_audio_init(const hal_audio_config_t *config);

/**
 * @brief 反初始化麦克风，释放资源
 *
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_audio_deinit(void);

/**
 * @brief 采集一帧音频数据
 *
 * 阻塞式调用，返回的 buffer 由调用方负责管理。
 * 当前规划阶段，此函数返回 ESP_ERR_NOT_SUPPORTED。
 *
 * @param[out] frame 音频帧输出指针
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t hal_audio_capture(hal_audio_frame_t *frame);

/**
 * @brief 检查麦克风是否就绪
 *
 * @return true 就绪
 * @return false 未就绪
 */
bool hal_audio_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_AUDIO_H */