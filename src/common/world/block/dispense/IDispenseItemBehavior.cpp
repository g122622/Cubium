#include "IDispenseItemBehavior.hpp"
#include "../../IWorld.hpp"
#include "../Block.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../../entity/entities/item/ItemEntity.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../sound/SoundCategory.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/math/Vector3.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// DefaultDispenseItemBehavior
// ============================================================================

ItemStack DefaultDispenseItemBehavior::dispense(IWorld& world, const BlockPos& pos,
                                                 const BlockState& state, ItemStack& stack) {
    // 获取发射方向
    Direction direction = state.get(BlockStateProperties::FACING());

    // 执行投掷
    ItemStack result = doDispense(world, pos, state, stack, direction, 0.2f, 6.0f);

    // 播放音效和粒子
    playSound(world, pos);
    spawnParticles(world, pos);

    return result;
}

ItemStack DefaultDispenseItemBehavior::doDispense(IWorld& world, const BlockPos& pos, const BlockState& state,
                                                   ItemStack& stack, Direction direction,
                                                   f32 velocity, f32 inaccuracy) {
    MC_UNUSED(inaccuracy);
    MC_UNUSED(state);

    if (stack.isEmpty()) {
        return stack;
    }

    // 减少物品数量
    stack.shrink(1);

    // 计算发射位置
    Vector3 dispensePos = getDispensePosition(pos, direction);

    // TODO: 创建物品实体并设置速度
    // 当前简化实现，需要实体系统支持
    // auto itemEntity = std::make_unique<ItemEntity>(world, dispensePos.x, dispensePos.y, dispensePos.z, resultStack);
    //
    // // 设置速度
    // f32 vx = static_cast<f32>(Directions::xOffset(direction)) * velocity;
    // f32 vy = static_cast<f32>(Directions::yOffset(direction)) * velocity + 0.1f;
    // f32 vz = static_cast<f32>(Directions::zOffset(direction)) * velocity;
    // itemEntity->setVelocity(vx, vy, vz);
    //
    // // 添加到世界
    // world.addEntity(std::move(itemEntity));

    // 返回剩余物品
    if (stack.isEmpty()) {
        return ItemStack();
    }
    return stack;
}

void DefaultDispenseItemBehavior::playSound(IWorld& world, const BlockPos& pos) {
    // MC 1.16.5: 播放发射音效
    // 参考: DefaultDispenseItemBehavior.playSound()
    if (!world.isClientSide()) {
        world.playSound(
            SoundEvents::BLOCK_DISPENSER_DISPENSE,
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            1.0f
        );
    }
}

void DefaultDispenseItemBehavior::spawnParticles(IWorld& world, const BlockPos& pos) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 生成烟雾粒子
    // world.addParticle(ParticleTypes::SMOKE, pos.x + 0.5, pos.y + 0.5, pos.z + 0.5, 0.0, 0.0, 0.0);
}

Vector3 DefaultDispenseItemBehavior::getDispensePosition(const BlockPos& pos, Direction direction) {
    // 计算发射位置：从方块面中心稍微向外偏移
    f32 x = static_cast<f32>(pos.x) + 0.5f + static_cast<f32>(Directions::xOffset(direction)) * 0.7f;
    f32 y = static_cast<f32>(pos.y) + 0.5f + static_cast<f32>(Directions::yOffset(direction)) * 0.7f;
    f32 z = static_cast<f32>(pos.z) + 0.5f + static_cast<f32>(Directions::zOffset(direction)) * 0.7f;
    return Vector3(x, y, z);
}

// ============================================================================
// ProjectileDispenseBehavior
// ============================================================================

ProjectileDispenseBehavior::ProjectileDispenseBehavior(i32 projectileType, f32 velocity, f32 inaccuracy)
    : m_projectileType(projectileType)
    , m_velocity(velocity)
    , m_inaccuracy(inaccuracy) {
}

ItemStack ProjectileDispenseBehavior::dispense(IWorld& world, const BlockPos& pos,
                                                const BlockState& state, ItemStack& stack) {
    // 获取发射方向
    Direction direction = state.get(BlockStateProperties::FACING());

    // 计算发射位置
    Vector3 dispensePos = getDispensePosition(pos, direction);

    // TODO: 创建投掷物实体
    // 当前简化实现，需要实体系统支持
    // auto projectile = createProjectile(world, dispensePos, stack);
    //
    // // 设置发射方向和速度
    // projectile->shoot(Directions::xOffset(direction), Directions::yOffset(direction) + 0.1f, Directions::zOffset(direction),
    //                   m_velocity, m_inaccuracy);
    //
    // // 添加到世界
    // world.addEntity(std::move(projectile));

    MC_UNUSED(dispensePos);
    MC_UNUSED(world);

    // 减少物品数量
    stack.shrink(1);

    // 播放音效
    playSound(world, pos);

    return stack.isEmpty() ? ItemStack() : stack;
}

} // namespace blocks
} // namespace mc
