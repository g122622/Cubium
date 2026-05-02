#pragma once

#include "ProjectileEntity.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 可投掷实体基类
 *
 * 用于雪球、鸡蛋、末影珍珠等可投掷物品。
 * 提供重力、碰撞检测和基本的投掷物理。
 *
 * 参考 MC 1.16.5 ThrowableEntity
 */
class ThrowableEntity : public ProjectileEntity {
public:
    virtual ~ThrowableEntity() = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    // ========== ThrowableEntity 方法 ==========

    /**
     * @brief 获取重力加速度
     *
     * 可投掷物品有固定的重力加速度 0.03
     */
    [[nodiscard]] f32 getGravity() const override { return 0.03f; }

protected:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    ThrowableEntity(LegacyEntityType type, EntityId id);
};

} // namespace entity
} // namespace mc
