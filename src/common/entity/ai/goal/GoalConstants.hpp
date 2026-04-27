#pragma once

#include "../../../core/Types.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace constants {

// ============================================================================
// 距离常量（平方值，用于距离比较）
// ============================================================================

/// 默认跟随距离（格）
constexpr f32 DEFAULT_FOLLOW_DISTANCE = 10.0f;
constexpr f32 DEFAULT_FOLLOW_DISTANCE_SQ = DEFAULT_FOLLOW_DISTANCE * DEFAULT_FOLLOW_DISTANCE;

/// 繁殖相关距离
/// MC 1.16.5: 配偶搜索范围 8.0D，繁殖距离 3.0D
constexpr f32 BREED_DETECTION_RANGE = 8.0f;          // 检测伴侣的距离
constexpr f32 BREED_DETECTION_RANGE_SQ = BREED_DETECTION_RANGE * BREED_DETECTION_RANGE;
constexpr f32 BREED_DISTANCE = 3.0f;                 // 繁殖所需距离
constexpr f32 BREED_DISTANCE_SQ = BREED_DISTANCE * BREED_DISTANCE;  // 9.0D

/// 诱惑相关距离
/// MC 1.16.5: 诱惑范围 10.0D，惊吓距离 6.0D，近距离停止 2.5D
constexpr f32 TEMPT_RANGE = 10.0f;                   // 诱惑检测范围
constexpr f32 TEMPT_RANGE_SQ = TEMPT_RANGE * TEMPT_RANGE;
constexpr f32 TEMPT_SCARE_DISTANCE = 6.0f;           // MC 1.16.5: 被吓跑的距离
constexpr f32 TEMPT_SCARE_DISTANCE_SQ = 36.0f;       // 6*6 = 36
constexpr f32 TEMPT_CLOSE_DISTANCE = 2.5f;           // MC 1.16.5: 停止移动的近距离
constexpr f32 TEMPT_CLOSE_DISTANCE_SQ = 6.25f;       // 2.5*2.5 = 6.25

/// 避开实体相关距离
/// MC 1.16.5: 近距离逃跑阈值 7.0D
constexpr f32 AVOID_DETECTION_RANGE = 16.0f;         // 检测威胁的距离
constexpr f32 AVOID_ESCAPE_RANGE = 16.0f;            // 逃跑距离
constexpr f32 AVOID_NEAR_DISTANCE = 7.0f;            // MC 1.16.5: 近距离逃跑阈值
constexpr f32 AVOID_NEAR_DISTANCE_SQ = 49.0f;        // 7*7 = 49

/// 跟随父母相关距离
/// MC 1.16.5: 最小距离 3.0D（9.0），最大距离 16.0D（256.0）
constexpr f32 FOLLOW_PARENT_SEARCH_RANGE = 8.0f;     // 搜索父母的距离
constexpr f32 FOLLOW_PARENT_SEARCH_RANGE_SQ = FOLLOW_PARENT_SEARCH_RANGE * FOLLOW_PARENT_SEARCH_RANGE;
constexpr f32 FOLLOW_PARENT_MIN_DISTANCE = 3.0f;     // 最小跟随距离
constexpr f32 FOLLOW_PARENT_MIN_DISTANCE_SQ = FOLLOW_PARENT_MIN_DISTANCE * FOLLOW_PARENT_MIN_DISTANCE;  // 9.0D
constexpr f32 FOLLOW_PARENT_MAX_DISTANCE = 16.0f;    // MC 1.16.5: 最大跟随距离
constexpr f32 FOLLOW_PARENT_MAX_DISTANCE_SQ = FOLLOW_PARENT_MAX_DISTANCE * FOLLOW_PARENT_MAX_DISTANCE;  // 256.0D

/// 近战攻击相关距离
/// MC 1.16.5: 攻击范围公式 (width * 2)^2 + targetWidth
constexpr f32 MELEE_ATTACK_STOP_DISTANCE = 32.0f;    // 停止追踪的距离
constexpr f32 MELEE_ATTACK_STOP_DISTANCE_SQ = MELEE_ATTACK_STOP_DISTANCE * MELEE_ATTACK_STOP_DISTANCE;
constexpr f32 MELEE_ATTACK_REACH = 2.0f;             // 攻击范围
constexpr f32 MELEE_ATTACK_REACH_SQ = MELEE_ATTACK_REACH * MELEE_ATTACK_REACH;

/// 恐慌逃跑相关距离
/// MC 1.16.5: findRandomTarget(creature, 5, 4)
constexpr f32 PANIC_ESCAPE_MIN_DISTANCE = 5.0f;      // 最小逃跑距离
constexpr f32 PANIC_ESCAPE_MAX_DISTANCE = 10.0f;     // 最大逃跑距离
constexpr f32 PANIC_STOP_DISTANCE = 2.0f;            // 停止距离
constexpr f32 PANIC_STOP_DISTANCE_SQ = PANIC_STOP_DISTANCE * PANIC_STOP_DISTANCE;
constexpr f32 PANIC_WATER_SEARCH_RANGE = 5.0f;       // 搜索水源的水平范围
constexpr f32 PANIC_WATER_SEARCH_VERTICAL = 4.0f;    // 搜索水源的垂直范围

/// 看向目标相关距离
constexpr f32 LOOK_AT_MAX_DISTANCE = 8.0f;           // 最大看向距离
constexpr f32 LOOK_AT_MAX_DISTANCE_SQ = LOOK_AT_MAX_DISTANCE * LOOK_AT_MAX_DISTANCE;

/// 随机漫步相关距离
/// MC 1.16.5: RandomPositionGenerator.findRandomTarget(creature, 10, 7)
constexpr f32 RANDOM_WALK_RANGE = 10.0f;             // 随机漫步水平范围
constexpr f32 RANDOM_WALK_VERTICAL_RANGE = 7.0f;     // 随机漫步垂直范围

// ============================================================================
// 时间常量（tick）
// ============================================================================

/// 攻击冷却时间（tick）
/// MC 1.16.5: 20 tick
constexpr i32 MELEE_ATTACK_COOLDOWN = 20;            // 1秒 = 20 tick

/// 路径重新计算间隔（tick）
/// MC 1.16.5: 4 + random.nextInt(7)，基础值 4
constexpr i32 PATH_RECALCULATE_BASE = 4;
constexpr i32 PATH_RECALCULATE_RANDOM = 7;

/// 跟随延迟间隔（tick）
/// MC 1.16.5: 10 tick
constexpr i32 FOLLOW_DELAY_INTERVAL = 10;

/// 诱惑冷却时间（tick）
/// MC 1.16.5: 100 tick
constexpr i32 TEMPT_COOLDOWN = 100;                  // 5秒

/// 看向时间范围（tick）
/// MC 1.16.5: 40 + random.nextInt(40)
constexpr i32 LOOK_AT_MIN_TIME = 40;                 // 2秒
constexpr i32 LOOK_AT_MAX_TIME = 80;                 // 4秒

/// 随机看向时间范围（tick）
/// MC 1.16.5: 20 + random.nextInt(20)
constexpr i32 RANDOM_LOOK_MIN_TIME = 20;             // 1秒
constexpr i32 RANDOM_LOOK_MAX_TIME = 40;             // 2秒

/// 繁殖延迟（tick）
/// MC 1.16.5: 60 tick
constexpr i32 SPAWN_BABY_DELAY = 60;

/// 最大漫步时间（tick）
constexpr i32 MAX_WALK_TIME = 600;                   // 30秒

// ============================================================================
// 概率常量
// ============================================================================

/// 默认看向概率
/// MC 1.16.5: 0.02F
constexpr f32 DEFAULT_LOOK_CHANCE = 0.02f;           // 2%

/// 随机看向概率
/// MC 1.16.5: 0.02F
constexpr f32 RANDOM_LOOK_CHANCE = 0.02f;            // 2%

/// 默认随机漫步概率倒数
/// MC 1.16.5: 120
constexpr i32 DEFAULT_WALK_CHANCE = 120;

/// 游泳跳跃概率
/// MC 1.16.5: 0.8F
constexpr f32 SWIM_JUMP_CHANCE = 0.8f;               // 80%

// ============================================================================
// 速度常量
// ============================================================================

/// 默认移动速度
constexpr f64 DEFAULT_MOVE_SPEED = 1.0;

/// 远距离逃跑速度
constexpr f64 AVOID_FAR_SPEED = 1.0;

/// 近距离逃跑速度
constexpr f64 AVOID_NEAR_SPEED = 1.5;

// ============================================================================
// 角度常量（度）
// ============================================================================

/// 默认头部旋转速度
constexpr f32 DEFAULT_HEAD_ROTATION_SPEED = 10.0f;

/// 最大身体旋转速度
constexpr f32 MAX_BODY_ROTATION_SPEED = 90.0f;

/// 视角变化阈值（用于惊吓检测）
/// MC 1.16.5: 5.0D
constexpr f64 VIEW_CHANGE_THRESHOLD = 5.0;

/// 移动检测阈值
/// MC 1.16.5: 0.010000000000000002D
constexpr f64 MOVEMENT_THRESHOLD = 0.010000000000000002;

// ============================================================================
// 其他常量
// ============================================================================

/// 攻击目标检查冷却（tick）
/// MC 1.16.5: 20L
constexpr i64 TARGET_CHECK_COOLDOWN = 20LL;

/// 路径失败惩罚增量
/// MC 1.16.5: 10
constexpr i32 PATH_FAILURE_PENALTY = 10;

/// 远距离路径重算增量
/// MC 1.16.5: 1024D (> 32格) 加 10, 256D (> 16格) 加 5
constexpr f64 DISTANCE_FAR_THRESHOLD = 1024.0;  // 32^2
constexpr f64 DISTANCE_MEDIUM_THRESHOLD = 256.0; // 16^2
constexpr i32 PATH_RECALC_FAR_BONUS = 10;
constexpr i32 PATH_RECALC_MEDIUM_BONUS = 5;

// ============================================================================
// 路径导航常量
// ============================================================================

/// 路径导航卡住检测阈值（tick）
/// MC 1.16.5: 100 ticks无移动视为卡住
constexpr i32 PATH_STUCK_THRESHOLD = 100;

/// 路径导航卡住检测距离阈值
/// MC 1.16.5: 移动距离小于此值视为无移动
constexpr f64 PATH_STUCK_DISTANCE_THRESHOLD = 0.0025;

} // namespace constants
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
