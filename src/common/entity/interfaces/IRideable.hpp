#pragma once

#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc {

// Forward declarations
class Player;
class BoostHelper;
class MobEntity;  // MobEntity 定义在 mc 命名空间

namespace entity {

/**
 * @brief 可骑乘接口 - 用于可以被玩家骑乘并控制方向的实体
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
    [[nodiscard]] virtual bool hasSaddle() const = 0;

    /**
     * @brief 设置鞍的状态
     * @param saddle 是否装备鞍
     */
    virtual void setSaddle(bool saddle) = 0;

    /**
     * @brief 当玩家开始骑乘时调用
     * @param player 骑乘的玩家
     */
    virtual void onPlayerStartRiding(Player* player) {
        MC_UNUSED(player);
    }

    /**
     * @brief 当玩家停止骑乘时调用
     * @param player 下马的玩家
     */
    virtual void onPlayerStopRiding(Player* player) {
        MC_UNUSED(player);
    }

    /**
     * @brief 获取骑乘时的移动速度
     * @return 移动速度
     */
    [[nodiscard]] virtual f32 getSteeringSpeed() const = 0;

    /**
     * @brief 使用钓竿加速（如胡萝卜钓竿、诡异菌钓竿）
     * @return 如果成功加速返回true
     */
    virtual bool boost() = 0;

    /**
     * @brief 获取当前速度提升时间
     * @return 剩余加速时间（ticks），0表示未加速
     */
    [[nodiscard]] virtual i32 getBoostTime() const { return 0; }

    /**
     * @brief 设置速度提升时间
     * @param time 加速时间（ticks）
     */
    virtual void setBoostTime(i32 time) { MC_UNUSED(time); }

    /**
     * @brief 检查是否可以被控制方向
     * @return 如果可以被控制返回true
     *
     * MC 1.16.5: canBeSteered()
     * 猪需要玩家手持胡萝卜钓竿，炽足兽需要玩家手持诡异菌钓竿
     */
    [[nodiscard]] virtual bool canBeSteered() const { return hasSaddle(); }

    /**
     * @brief 控制骑乘实体向指定方向移动
     * @param travelVec 移动向量（通常来自玩家的输入）
     *
     * MC 1.16.5: travelTowards(Vector3d)
     * 用于接收玩家的移动输入
     */
    virtual void travelTowards(const Vector3& travelVec) {
        MC_UNUSED(travelVec);
    }

    /**
     * @brief 执行骑乘移动逻辑
     *
     * MC 1.16.5: ride(MobEntity, BoostHelper, Vector3d)
     * 处理骑乘实体的移动逻辑，包括：
     * - 同步骑乘者朝向
     * - 计算加速速度
     * - 调用travelTowards移动实体
     *
     * @param mount 骑乘的实体
     * @param helper 加速辅助器
     * @param travelVec 移动向量
     * @return 是否成功执行骑乘移动
     */
    virtual bool ride(MobEntity& mount, BoostHelper& helper, const Vector3& travelVec);
};

} // namespace entity
} // namespace mc
