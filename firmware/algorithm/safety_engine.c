/**
 * @file safety_engine.c
 * @brief 三重判定引擎 — 实现
 *
 * 姿态评分逻辑从旧的 main.c 中提取并重构。
 * 音频和视觉评估当前为空实现（占位），待后续 AI 模型就绪后补充。
 *
 * @author BanSafe Team
 * @version 0.1.0
 * @date 2026-05-13
 */

#include "safety_engine.h"
#include <math.h>
#include "esp_log.h"

static const char *TAG = "SAFETY_ENGINE";

esp_err_t safety_engine_init(void)
{
    ESP_LOGI(TAG, "Safety engine initialized");
    return ESP_OK;
}

int safety_engine_evaluate_posture(void)
{
    hal_accel_data_t accel;

    if (hal_sensor_read_accel(&accel) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read accel data");
        return -1;
    }

    /* 计算加速度矢量大小（单位: mg） */
    float magnitude = sqrtf(
        (float)accel.x * accel.x +
        (float)accel.y * accel.y +
        (float)accel.z * accel.z
    );

    /*
     * 异常判定：正常状态加速度矢量接近 1g (1000 mg)。
     * 偏差越大评分越高（越危险）。
     */
    float deviation = fabsf(magnitude - 1000.0f);

    /* 映射到 0-100 分，假设最大偏差 2000 mg 时评分为 100 */
    int score = (int)(deviation / 20.0f);
    if (score > 100) score = 100;
    if (score < 0)   score = 0;

    ESP_LOGD(TAG, "Posture eval: score=%d, mag=%.1f mg", score, (double)magnitude);

    return score;
}

int safety_engine_evaluate_audio(void)
{
    /*
     * TODO: 规划中功能
     * 1. 通过 hal_audio_capture() 获取 PCM 数据
     * 2. 提取 MFCC 或类似音频特征
     * 3. 跑 TFLite Micro 音频模型推理
     * 4. 返回 0-100 评分
     */
    return -1; /* ESP_ERR_NOT_SUPPORTED 的等价语义 */
}

int safety_engine_evaluate_visual(void)
{
    /*
     * TODO: 规划中功能
     * 1. 通过 hal_camera_capture() 获取 JPEG/RGB 帧
     * 2. 预处理图像（缩放/归一化）
     * 3. 跑 TFLite Micro 视觉模型推理
     * 4. 返回 0-100 评分
     */
    return -1; /* ESP_ERR_NOT_SUPPORTED 的等价语义 */
}

esp_err_t safety_engine_run(safety_result_t *result)
{
    safety_result_t local_result = {0};

    /* ---- 阶段 1: 姿态检测 ---- */
    local_result.posture_score = safety_engine_evaluate_posture();
    if (local_result.posture_score < 0) {
        ESP_LOGW(TAG, "Posture evaluation failed");
        local_result.stage = SAFETY_STAGE_IDLE;
        goto done;
    }

    if (local_result.posture_score >= SAFETY_POSTURE_THRESHOLD) {
        ESP_LOGI(TAG, "Stage 1 (Posture) triggered: score=%d >= %d",
                 local_result.posture_score, SAFETY_POSTURE_THRESHOLD);
        local_result.stage = SAFETY_STAGE_POSTURE;
    } else {
        local_result.stage = SAFETY_STAGE_IDLE;
        goto done;
    }

    /* ---- 阶段 2: 音频分析 ---- */
    local_result.audio_score = safety_engine_evaluate_audio();
    if (local_result.audio_score < 0) {
        /* 音频模块未就绪，直接跳过阶段2进入阶段3 */
        ESP_LOGW(TAG, "Audio module not available, skipping stage 2");
        local_result.stage = SAFETY_STAGE_AUDIO;
        local_result.audio_score = 0; /* 归一化 */
    }

    if (local_result.audio_score >= SAFETY_AUDIO_THRESHOLD) {
        ESP_LOGI(TAG, "Stage 2 (Audio) triggered: score=%d >= %d",
                 local_result.audio_score, SAFETY_AUDIO_THRESHOLD);
        local_result.stage = SAFETY_STAGE_AUDIO;
    } else {
        goto done; /* 阶段2未触发，不进入下一阶段 */
    }

    /* ---- 阶段 3: 视觉分析 ---- */
    local_result.visual_score = safety_engine_evaluate_visual();
    if (local_result.visual_score < 0) {
        ESP_LOGW(TAG, "Visual module not available, skipping stage 3");
        local_result.visual_score = 0;
        local_result.stage = SAFETY_STAGE_VISUAL;
    }

    if (local_result.visual_score >= SAFETY_VISUAL_THRESHOLD) {
        ESP_LOGI(TAG, "Stage 3 (Visual) triggered: score=%d >= %d",
                 local_result.visual_score, SAFETY_VISUAL_THRESHOLD);
        local_result.stage = SAFETY_STAGE_ALERT;
        local_result.alert_triggered = true;
    }

done:
    if (result) {
        *result = local_result;
    }

    return ESP_OK;
}