#pragma once

#include "../../../core/Types.hpp"
#include <string>

namespace mc {
namespace world {
namespace village {
namespace raid {

/**
 * @brief 掠夺者类型枚举
 *
 * 参考 MC 1.16.5
 */
enum class RaiderType : u8 {
    /// 掠夺者（持弩）
    Pillager = 0,

    /// 灾厄村民（持铁斧）
    Vindicator,

    /// 唤魔者（召唤恼鬼和尖牙）
    Evoker,

    /// 劫掠兽（巨型野兽）
    Ravager,

    /// 女巫（在袭击中会参与）
    Witch
};

/**
 * @brief 掠夺者类型工具类
 */
class RaiderTypeHelper {
public:
    /**
     * @brief 获取类型名称
     */
    [[nodiscard]] static const char* getName(RaiderType type);

    /**
     * @brief 获取基础生命值
     */
    [[nodiscard]] static f32 getBaseHealth(RaiderType type);

    /**
     * @brief 获取生成权重（用于随机选择）
     */
    [[nodiscard]] static i32 getSpawnWeight(RaiderType type, i32 wave);

    /**
     * @brief 是否可以骑劫掠兽
     */
    [[nodiscard]] static bool canRideRavager(RaiderType type);
};

/**
 * @brief 袭击状态
 */
enum class RaidStatus : u8 {
    /// 进行中
    Ongoing,

    /// 胜利（玩家方胜利）
    Victory,

    /// 失败（掠夺者胜利）
    Loss,

    /// 已停止
    Stopped
};

/**
 * @brief 袭击ID类型
 */
using RaidId = u64;

/**
 * @brief 袭击配置常量
 */
struct RaidConfig {
    /// 最大波次（简单难度）
    static constexpr i32 MAX_WAVES_EASY = 3;

    /// 最大波次（普通难度）
    static constexpr i32 MAX_WAVES_NORMAL = 5;

    /// 最大波次（困难难度）
    static constexpr i32 MAX_WAVES_HARD = 7;

    /// 每波生成间隔（tick）
    static constexpr i64 WAVE_INTERVAL = 1200; // 60秒

    /// 袭击超时（tick）
    static constexpr i64 RAID_TIMEOUT = 48000; // 40分钟

    /// 生成距离最小值
    static constexpr f32 SPAWN_DISTANCE_MIN = 45.0f;

    /// 生成距离最大值
    static constexpr f32 SPAWN_DISTANCE_MAX = 52.0f;

    /// 不祥之兆等级对波次的影响
    static constexpr i32 BAD_OMEN_WAVE_BONUS = 1;

    /// 每波掠夺者最小数量
    static constexpr i32 MIN_RAIDERS_PER_WAVE = 3;

    /// 每波掠夺者最大数量
    static constexpr i32 MAX_RAIDERS_PER_WAVE = 20;
};

} // namespace raid
} // namespace village
} // namespace world
} // namespace mc
