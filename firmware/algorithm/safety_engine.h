/**
 * @file safety_engine.h
 * @brief 三重判定引擎 — 危险行为检测与报警决策
 *
 * 三层级联判定流程：
 *   1. 姿态异常检测（加速度计 → 评分）
 *   2. 音频异常检测（麦克风 → 特征提取 → 评分）【规划中】
 *   3. 视觉异常检测（摄像头 → TFLite 推理 → 评分）【规划中】
 *
 * 仅当三层评分均超阈值时才触发最终报警（三重"与"逻辑）。
 * 本模块只调用 HAL 层接口，不直接访问驱动。
 *
 * @author BanSafe Team
 * @version 0.1.0
 * @date 2026-05-13
 */

#ifndef SAFETY_ENGINE_H
#define SAFETY_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "../hal/hal_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 宏定义 — 阈值配置
 *============================================================================*/

/** 姿态异常评分阈值 (0-100)，超过此值触发阶段1 */
#define SAFETY_POSTURE_THRESHOLD       (40)

/** 音频异常评分阈值 (0-100)，超过此值触发阶段2 【规划中】 */
#define SAFETY_AUDIO_THRESHOLD         (60)

/** 视觉异常评分阈值 (0-100)，超过此值触发阶段3 【规划中】 */
#define SAFETY_VISUAL_THRESHOLD        (80)

/*============================================================================
 * 数据结构
 *============================================================================*/

/**
 * @brief 判定阶段枚举
 */
typedef enum {
    SAFETY_STAGE_IDLE      = 0,  /*!< 空闲，等待触发 */
    SAFETY_STAGE_POSTURE   = 1,  /*!< 阶段1：姿态检测 */
    SAFETY_STAGE_AUDIO     = 2,  /*!< 阶段2：音频分析 */
    SAFETY_STAGE_VISUAL    = 3,  /*!< 阶段3：视觉分析 */
    SAFETY_STAGE_ALERT     = 4   /*!< 报警已触发 */
} safety_stage_t;

/**
 * @brief 单次判定结果
 */
typedef struct {
    safety_stage_t stage;          /*!< 当前阶段 */
    int            posture_score;  /*!< 姿态评分 0-100 */
    int            audio_score;    /*!< 音频评分 0-100 【规划中】 */
    int            visual_score;   /*!< 视觉评分 0-100 【规划中】 */
    bool           alert_triggered; /*!< 是否触发报警 */
    uint32_t       timestamp_ms;   /*!< 时间戳 */
} safety_result_t;

/*============================================================================
 * 公共接口
 *============================================================================*/

/**
 * @brief 初始化安全引擎
 *
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t safety_engine_init(void);

/**
 * @brief 执行一次姿态安全评估
 *
 * 从 HAL 层读取加速度数据并计算异常评分。
 *
 * @return int 姿态评分 (0-100)，负值表示错误
 */
int safety_engine_evaluate_posture(void);

/**
 * @brief 执行一次音频安全评估
 *
 * 【规划中】当前返回 ESP_ERR_NOT_SUPPORTED。
 *
 * @return int 音频评分 (0-100)，负值表示错误
 */
int safety_engine_evaluate_audio(void);

/**
 * @brief 执行一次视觉安全评估
 *
 * 【规划中】当前返回 ESP_ERR_NOT_SUPPORTED。
 *
 * @return int 视觉评分 (0-100)，负值表示错误
 */
int safety_engine_evaluate_visual(void);

/**
 * @brief 执行完整的三重判定流程
 *
 * 按阶段级联：姿态 → 音频 → 视觉，全部超阈值则触发报警。
 *
 * @param[out] result 判定结果输出指针（可为 NULL）
 * @return esp_err_t ESP_OK 成功，其他为错误码
 */
esp_err_t safety_engine_run(safety_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_ENGINE_H */