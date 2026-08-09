/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "DamagingProjectileEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"

#include <memory>

// 前向声明粒子类型（与 common/particle/ParticleTypes.hpp 中的定义一致）
namespace mc {
namespace particle {
enum class ParticleTypeId : u16;
}
} // namespace mc

namespace mc {
namespace entity {

/**
 * @brief 抽象火球实体基类
 *
 * 火球族共用的最小公共语义基类。
 */
class AbstractFireballEntity : public DamagingProjectileEntity {
public:
    ~AbstractFireballEntity() override = default;

    [[nodiscard]] f32 width() const override { return 1.0f; }
    [[nodiscard]] f32 height() const override { return 1.0f; }

protected:
    explicit AbstractFireballEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    /// 本类继承链标识（parent = DamagingProjectileEntity::classInfo()）。见 Entity::classInfo()。
    /// 本类无同步字段，classInfo 仅作父链遍历节点：子类（Fireball/WitherSkull）
    /// ClassRegisterGuard 沿父链查找最高 id 时穿过本节点续接。
    static const EntityClassInfo& classInfo();
};

class FireballEntity : public AbstractFireballEntity {
public:
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    explicit FireballEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    [[nodiscard]] f32 width() const override { return 1.0f; }
    [[nodiscard]] f32 height() const override { return 1.0f; }

    [[nodiscard]] i32 explosionPower() const;
    void setExplosionPower(i32 power);

    // ========== 网络同步数据参数 ==========
    // 对齐 vanilla 1.21.11 Fireball.defineSynchedData(): DATA_ITEM_STACK(ItemStack, id8)。
    // vanilla 火球同步其物品本体（发射者蓄力时手持火球物品）；项目 FireballEntity 当前
    // 无 item 字段（m_explosionPower 是运行时计算），此处注册占位空 ItemStackView 保持
    // wire 字段位置对齐，避免客户端按 id8 反序列化时类型/数量校验失败。
    // TODO: 补齐 FireballEntity 的物品字段后，镜像真实物品到 DATA_ITEM_STACK。
    static entity::DataParameter<network::ir::play::ItemStackView> DATA_ITEM_STACK_PARAM;

    [[nodiscard]] static u16 getItemStackParamId() { return DATA_ITEM_STACK_PARAM.id(); }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;

    /**
     * @brief 注册实体同步数据参数
     *
     * 重写基类 registerData()，注册 Fireball 专属同步参数 DATA_ITEM_STACK（id8）。
     * C++ 虚函数在构造函数中不会派生到子类，FireballEntity 构造函数必须显式调用此方法。
     */
    void registerData() override;

    /// 本类继承链标识（parent = AbstractFireballEntity::classInfo()）。见 Entity::classInfo()。
    static const EntityClassInfo& classInfo();

    // 批次6 子目标2 Step4：m_explosionPower 迁入 ecs::FireballStateComponent（与 WitherSkull 共用）。
};

class SmallFireballEntity : public AbstractFireballEntity {
public:
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    explicit SmallFireballEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    [[nodiscard]] f32 width() const override { return 0.3125f; }
    [[nodiscard]] f32 height() const override { return 0.3125f; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;
};

class DragonFireballEntity : public AbstractFireballEntity {
public:
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    explicit DragonFireballEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    [[nodiscard]] f32 width() const override { return 1.0f; }
    [[nodiscard]] f32 height() const override { return 1.0f; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;

    // 龙息火球使用 DRAGON_BREATH 粒子
    [[nodiscard]] particle::ParticleTypeId getParticleType() const override;

    // 龙息火球不燃烧
    [[nodiscard]] bool isFiery() const override { return false; }

private:
    /**
     * @brief 创建龙息区域效果云
     */
    void _createDragonBreathCloud();
};

class WitherSkullEntity : public AbstractFireballEntity {
public:
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    explicit WitherSkullEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    [[nodiscard]] f32 width() const override { return 0.3125f; }
    [[nodiscard]] f32 height() const override { return 0.3125f; }

    [[nodiscard]] bool isBlue() const;
    void setBlue(bool blue);

    // ========== 网络同步数据参数 ==========
    // 对齐 vanilla 1.21.11 WitherSkull.defineSynchedData(): DATA_DANGEROUS(bool, id8)。
    // 真相源为 FireballStateComponent.m_blue，DataParameter 作镜像（批次4 模式）。
    static entity::DataParameter<bool> DATA_DANGEROUS_PARAM;

    [[nodiscard]] static u16 getDangerousParamId() { return DATA_DANGEROUS_PARAM.id(); }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;

    // 蓝色凋灵之首运动因子为 0.73，普通为 0.95
    [[nodiscard]] f32 getMotionFactor() const override;
    // 凋灵之首不燃烧
    [[nodiscard]] bool isFiery() const override;

    /**
     * @brief 注册实体同步数据参数
     *
     * 重写基类 registerData()，注册 WitherSkull 专属同步参数 DATA_DANGEROUS（id8）。
     * C++ 虚函数在构造函数中不会派生到子类，WitherSkullEntity 构造函数必须显式调用此方法。
     */
    void registerData() override;

    /// 本类继承链标识（parent = AbstractFireballEntity::classInfo()）。见 Entity::classInfo()。
    static const EntityClassInfo& classInfo();

    // 批次6 子目标2 Step4：m_blue 迁入 ecs::FireballStateComponent（与 Fireball 共用）。
};

} // namespace entity
} // namespace mc
