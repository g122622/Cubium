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

#include "WaterMobEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "entity/damage/DamageSource.hpp"
#include <cmath>

namespace mc {

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = CreatureEntity::classInfo()）。
// 本类无同步字段，classInfo 仅作父链遍历节点：子类 ClassRegisterGuard 沿父链查找最高 id
// 时穿过本类（lastAssignedId=-1）直达父链已分配 id 的基类，子类首字段续接其后。
const entity::EntityClassInfo& WaterMobEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"WaterMobEntity", &CreatureEntity::classInfo()};
    return s_classInfo;
}

WaterMobEntity::WaterMobEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : CreatureEntity(id, registry)
{
    // 注册属性
    registerAttributes();
}

bool WaterMobEntity::isInWater() const
{
    // 水生生物使用基类的 isInWater() 实现
    // 基类读 EnvironmentStateComponent.inWater，由 ecs::sys::environmentSensing 每帧刷新
    return Entity::isInWater();
}

bool WaterMobEntity::isInWaterOrBubble() const
{
    // 检查是否在水中或气泡柱中
    if (isInWater()) {
        return true;
    }

    // 检查是否在气泡柱中
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return false;
    }

    // 获取实体当前位置的方块
    const BlockPos blockPos(static_cast<i32>(std::floor(position().x)),
        static_cast<i32>(std::floor(position().y)),
        static_cast<i32>(std::floor(position().z)));
    const BlockState* state = worldPtr->getBlockState(blockPos);
    if (state == nullptr) {
        return false;
    }

    // 检查是否为气泡柱方块
    return &state->owner() == VanillaBlocks::BUBBLE_COLUMN;
}

f32 WaterMobEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    // 水生生物偏好水中位置：在水中返回10.0f，否则返回0.0f
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return 0.0f;
    }

    BlockPos pos(static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(z));
    const fluid::FluidState* fluid = worldPtr->getFluidState(pos);
    if (fluid != nullptr && !fluid->isEmpty() && fluid->getFluid().isIn(fluid::FluidTags::WATER())) {
        return 10.0f;
    }

    return 0.0f;
}

void WaterMobEntity::tick()
{
    CreatureEntity::tick();

    // 更新空气供应（水状态检测和回调在 updateAirSupply 中处理）
    updateAirSupply();
}

void WaterMobEntity::registerAttributes()
{
    // 调用父类方法
    CreatureEntity::registerAttributes();

    // 水生生物的基础属性
}

void WaterMobEntity::updateAirSupply()
{
    // MC Java: WaterAnimal.baseTick() -> handleAirSupply()
    // 水生生物的反逻辑：在水中恢复空气，在陆地上消耗空气
    if (!isAlive()) {
        return;
    }

    bool inWater = isInWater();

    // 检测水状态变化并触发回调
    if (inWater && !m_wasInWater) {
        onEnterWater();
    } else if (!inWater && m_wasInWater) {
        onLeaveWater();
    }
    m_wasInWater = inWater;

    if (inWater) {
        // 在水中，立即恢复空气到最大值
        // MC Java: this.setAirSupply(300) -- 即 maxAirSupply
        setAir(maxAir());
    } else {
        // 不在水中，消耗空气（水生生物在陆地窒息）
        // MC Java: this.setAirSupply(airSupply - 1)
        i32 newAir = air() - 1;
        setAir(newAir);

        // 空气耗尽时触发窒息伤害
        // MC Java: if (this.shouldTakeDrowningDamage()) { setAirSupply(0); hurtServer(drown, 2.0F) }
        if (shouldTakeDrowningDamage()) {
            setAir(0);

            // 广播溺水实体事件
            if (m_world != nullptr) {
                m_world->broadcastEntityStatus(id(), static_cast<u8>(67));
            }

            // 窒息伤害量 2.0F（与 MC Java WaterAnimal.handleAirSupply 一致）
            EnvironmentalDamage drownSource = DamageSources::drown();
            hurt(drownSource, physics::DROWN_DAMAGE_AMOUNT);
        }
    }
}

} // namespace mc
