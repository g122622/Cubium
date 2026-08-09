#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 马类动画组件（A 类本地字段）
 *
 * 承载 AbstractHorseEntity 的动画插值状态。仅 AbstractHorseEntity 子树 attach。
 *
 * 字段分类：A 类纯本地无同步无持久化——11 字段全为客户端渲染插值用的运行时状态，
 * 服务端 tick 推进，渲染器经 getRearingAmount/getHeadLeanAmount/getMouthOpennessAmount
 * 读取 lerp。vanilla AbstractHorse 不存盘这些字段。
 *
 * 计数器（i32×5）：eatingCounter/openMouthCounter/jumpRearingCounter/tailCounter/
 * sprintCounter，服务端 aiStep()/tick() 推进，超时重置触发动画状态切换。
 *
 * 插值量（f32×6，prev/cur 对）：headLean（低头吃草）/rearingAmount（扬蹄）/
 * mouthOpenness（张嘴），updateRiding() 每 tick 推进，渲染时 lerp(partialTicks, prev, cur)。
 */
struct HorseAnimationComponent {
    // 计数器
    i32 m_eatingCounter{0};       ///< 吃草计数器（超 50 tick 停止吃草）
    i32 m_openMouthCounter{0};    ///< 张嘴计数器（超 30 tick 闭嘴）
    i32 m_jumpRearingCounter{0};  ///< 扬蹄计数器（倒计时，归零清除扬蹄）
    i32 m_tailCounter{0};         ///< 尾巴摆动计数器（超 8 tick 重置）
    i32 m_sprintCounter{0};       ///< 冲刺计数器（超 300 tick 重置）

    // 插值量（prev/cur 对）
    f32 m_headLean{0.0f};          ///< 低头吃草动画量
    f32 m_prevHeadLean{0.0f};      ///< 上一帧低头量
    f32 m_rearingAmount{0.0f};     ///< 扬蹄动画量
    f32 m_prevRearingAmount{0.0f}; ///< 上一帧扬蹄量
    f32 m_mouthOpenness{0.0f};     ///< 张嘴动画量
    f32 m_prevMouthOpenness{0.0f}; ///< 上一帧张嘴量
};

} // namespace mc::ecs
