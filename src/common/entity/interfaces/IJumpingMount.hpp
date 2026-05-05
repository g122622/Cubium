#pragma once

#include "common/core/Types.hpp"

namespace mc {
namespace entity {

// Forward declarations
class Player;

/**
 * @brief 可跳跃骑乘接口 - 用于可以通过玩家输入控制跳跃的骑乘实体
 *
 * 实现此接口的实体在玩家骑乘时可以通过玩家的跳跃输入来跳跃。
 * 例如：马、驴、骡、羊驼等。
 *
 * 参考 MC 1.16.5 IJumpingMount
 *
 * 【重要】MC 1.16.5 中：
 * - jumpPower 是 int 类型，范围 0-100
 * - handleStartJump(int) 接收跳跃力度参数
 * - handleStopJump() 用于停止跳跃
 */
class IJumpingMount {
public:
    virtual ~IJumpingMount() = default;

    /**
     * @brief 当玩家请求跳跃时调用
     *
     * 此方法由客户端通过发送跳跃包来触发
     */
    virtual void onJump() = 0;

    /**
     * @brief 获取跳跃力度（0 - 100）
     * @return 当前跳跃力度
     *
     * MC 1.16.5: 马的跳跃力度由玩家按住跳跃键的时间决定
     * 注意：MC 使用 int 类型，范围 0-100
     */
    virtual i32 getJumpPower() const = 0;

    /**
     * @brief 设置跳跃力度
     * @param power 跳跃力度 (0 - 100)
     */
    virtual void setJumpPower(i32 power) = 0;

    /**
     * @brief 获取最大跳跃高度
     * @return 最大跳跃高度对应的跳跃高度（方块数）
     */
    virtual f32 getMaxJumpHeight() const = 0;

    /**
     * @brief 检查是否可以跳跃
     * @return 如果可以跳跃返回true
     */
    virtual bool canJump() const = 0;

    /**
     * @brief 开始蓄力跳跃
     * @param jumpPower 初始跳跃力度 (MC 1.16.5: handleStartJump)
     *
     * MC 1.16.5: 当玩家开始按住跳跃键时调用
     * 参数表示初始跳跃力度
     */
    virtual void startJumping(i32 jumpPower) = 0;

    /**
     * @brief 停止跳跃蓄力
     *
     * MC 1.16.5: handleStopJump()
     * 当玩家松开跳跃键时调用，执行实际跳跃
     */
    virtual void stopJumping() = 0;
};

} // namespace entity
} // namespace mc
