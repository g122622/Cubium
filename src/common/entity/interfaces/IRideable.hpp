#pragma once

#include "common/core/Types.hpp"

namespace mc {
namespace entity {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 可骑乘接口 - 用于可以被玩家骑乘的实体
 *
 * 实现此接口的实体可以被玩家骑乘并控制方向。
 * 例如：猪（需要鞍和胡萝卜钓竿）、炽足兽（需要鞍和诡异菌钓竿）、马等。
 *
 * 参考 MC 1.16.5 IRideable
 */
class IRideable {
public:
    virtual ~IRideable() = default;

    /**
     * @brief 检查是否装备了鞍
     * @return 如果装备了鞍则返回true
     */
    virtual bool hasSaddle() const = 0;

    /**
     * @brief 设置鞍的状态
     * @param saddle 是否装备鞍
     */
    virtual void setSaddle(bool saddle) = 0;

    /**
     * @brief 当玩家开始骑乘时调用
     * @param player 骑乘的玩家
     */
    virtual void onPlayerStartRiding(Player* player) = 0;

    /**
     * @brief 当玩家停止骑乘时调用
     * @param player 下马的玩家
     */
    virtual void onPlayerStopRiding(Player* player) = 0;

    /**
     * @brief 获取骑乘时的移动速度
     * @return 移动速度
     */
    virtual f32 getSteeringSpeed() const = 0;

    /**
     * @brief 使用钓竿加速（如胡萝卜钓竿、诡异菌钓竿）
     * @return 如果成功加速返回true
     */
    virtual bool boost() = 0;

    /**
     * @brief 获取当前速度提升时间
     * @return 剩余加速时间（ticks），0表示未加速
     */
    virtual i32 getBoostTime() const { return 0; }

    /**
     * @brief 设置速度提升时间
     * @param time 加速时间（ticks）
     */
    virtual void setBoostTime(i32 time) { MC_UNUSED(time); }
};

} // namespace entity
} // namespace mc
