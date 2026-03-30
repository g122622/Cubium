#pragma once

#include "common/core/Types.hpp"

namespace mc {
namespace entity {

/**
 * @brief 飞行动物接口 - 用于可以飞行的动物
 *
 * 实现此接口的动物可以在空中飞行。
 * 例如：蜜蜂、鹦鹉等。
 *
 * 参考 MC 1.16.5 IFlyingAnimal
 */
class IFlyingAnimal {
public:
    virtual ~IFlyingAnimal() = default;

    /**
     * @brief 检查是否正在飞行
     * @return 如果正在飞行返回true
     */
    virtual bool isFlying() const = 0;

    /**
     * @brief 设置飞行状态
     * @param flying 是否飞行
     */
    virtual void setFlying(bool flying) = 0;

    /**
     * @brief 获取飞行高度限制
     * @return 最大飞行高度（相对地面）
     */
    virtual f32 getMaxFlightHeight() const { return 32.0f; }

    /**
     * @brief 检查是否可以降落
     * @return 如果可以降落返回true
     */
    virtual bool canLand() const { return true; }

    /**
     * @brief 检查是否可以在当前位置悬停
     * @return 如果可以悬停返回true
     */
    virtual bool canHover() const { return false; }
};

} // namespace entity
} // namespace mc
