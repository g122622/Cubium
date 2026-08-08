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

#include "CopperGolemStatueBlockEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemTypes.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>

namespace mc {
namespace blockentity {

// ============================================================================
// removeStatue 实现
// ============================================================================

std::unique_ptr<Entity> CopperGolemStatueBlockEntity::removeStatue(const BlockState& state)
{
    // 对应 MC Java 1.21.11: CopperGolemStatueBlockEntity.removeStatue(BlockState)
    //   CopperGolem coppergolem = EntityType.COPPER_GOLEM.create(this.level, EntitySpawnReason.TRIGGERED);
    //   if (coppergolem != null) {
    //       coppergolem.setCustomName(this.components().get(DataComponents.CUSTOM_NAME));
    //       return this.initCopperGolem(p_435095_, coppergolem);
    //   }
    //   return null;
    //
    // initCopperGolem:
    //   BlockPos blockpos = this.getBlockPos();
    //   p_481299_.snapTo(blockpos.getCenter().x, blockpos.getY(), blockpos.getCenter().z,
    //                    p_434526_.getValue(CopperGolemStatueBlock.FACING).toYRot(), 0.0F);
    //   p_481299_.yHeadRot = p_481299_.getYRot();
    //   p_481299_.yBodyRot = p_481299_.getYRot();
    //   p_481299_.playSpawnSound();
    //   return p_481299_;

    IWorld* world = getWorld();
    if (world == nullptr) {
        return nullptr;
    }

    // 通过实体注册表获取铜傀儡实体类型并创建实例
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* copperGolemType = registry.getType(entity::EntityTypeKeys::COPPER_GOLEM);
    if (copperGolemType == nullptr) {
        return nullptr;
    }

    // 通过世界获取 ECS 实体注册表（ServerWorld 持有 m_entityRegistry）
    auto* ecsRegistry = world->entityRegistry();
    if (ecsRegistry == nullptr) {
        return nullptr;
    }

    std::unique_ptr<Entity> entity = copperGolemType->create(world, *ecsRegistry);
    if (entity == nullptr) {
        return nullptr;
    }

    // 设置实体的世界引用（使 Entity::playSound 等方法可用）
    entity->setWorld(world);

    // 转移 CUSTOM_NAME（对应 MC: coppergolem.setCustomName(this.components().get(DataComponents.CUSTOM_NAME))）
    if (!m_customName.empty()) {
        entity->setCustomName(m_customName);
    }

    // 计算位置：blockpos.getCenter().x, blockpos.getY(), blockpos.getCenter().z
    // MC 的 snapTo(x, y, z, yRot, xRot) 等价于本项目的 setPosition(x,y,z) + setRotation(yaw, pitch)
    const Vector3 center = m_pos.center();
    entity->setPosition(center.x, static_cast<f32>(m_pos.y), center.z);

    // 从 FACING 计算 yaw（MC: Direction.toYRot()）
    // MC 约定：South=0, West=90, North=180, East=270
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    f32 yaw = 0.0f;
    switch (facing) {
        case Direction::South:
            yaw = 0.0f;
            break;
        case Direction::West:
            yaw = 90.0f;
            break;
        case Direction::North:
            yaw = 180.0f;
            break;
        case Direction::East:
            yaw = 270.0f;
            break;
        default:
            yaw = 0.0f;
            break;
    }
    entity->setRotation(yaw, 0.0f);

    // 同步身体与头部朝向到 FACING 方向
    // 对应 MC 1.21.11 CopperGolemStatueBlockEntity#initCopperGolem:
    //   p_481299_.yHeadRot = p_481299_.getYRot();
    //   p_481299_.yBodyRot = p_481299_.getYRot();
    // Entity 基类的 setYBodyRot/setYHeadRot 默认空实现，LivingEntity（含
    // CopperGolemEntity）重写后写入对应字段，因此对任意实体类型调用都安全。
    entity->setYBodyRot(yaw);
    entity->setYHeadRot(yaw);

    // 设置铜傀儡的初始氧化等级为 Unaffected 并播放生成音效
    // 对应 MC: p_481299_.playSpawnSound()（CopperGolem.spawn(WeatherState.UNAFFECTED) 内部也会播放音效）
    auto* copperGolem = dynamic_cast<CopperGolemEntity*>(entity.get());
    if (copperGolem != nullptr) {
        copperGolem->spawnFromStatue(entity::CopperGolemWeatherState::Unaffected);
    } else {
        // 兜底：直接通过世界播放生成音效
        world->playSound(SoundEvents::ENTITY_COPPER_GOLEM_SPAWN,
            sound::SoundCategory::Neutral,
            Vector3(center.x, static_cast<f32>(m_pos.y) + 0.5f, center.z),
            1.0f,
            1.0f);
    }

    return entity;
}

} // namespace blockentity
} // namespace mc
