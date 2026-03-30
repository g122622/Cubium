#include "IDispenseItemBehavior.hpp"
#include "../../IWorld.hpp"
#include "../Block.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../../entity/entities/item/ItemEntity.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// DefaultDispenseItemBehavior
// ============================================================================

ItemStack DefaultDispenseItemBehavior::dispense(IBlockSource& source, ItemStack& stack) {
    // 获取发射方向
    const BlockState& state = source.getBlockState();
    Direction direction = state.get(BlockStateProperties::FACING());

    // 执行投掷
    ItemStack result = doDispense(source, stack, direction, 0.2f, 6.0f);

    // 播放音效和粒子
    playSound(source);
    spawnParticles(source);

    return result;
}

ItemStack DefaultDispenseItemBehavior::doDispense(IBlockSource& source, ItemStack& stack,
                                                   Direction direction, f32 velocity, f32 inaccuracy) {
    MC_UNUSED(inaccuracy);

    if (stack.isEmpty()) {
        return stack;
    }

    // 减少物品数量
    stack.shrink(1);

    // 计算发射位置
    DispensePosition dispensePos = getDispensePosition(source, direction);

    // TODO: 创建物品实体并设置速度
    // 当前简化实现，需要实体系统支持
    // IWorld& world = source.getWorld();
    // auto itemEntity = std::make_unique<ItemEntity>(world, dispensePos.getX(), dispensePos.getY(), dispensePos.getZ(), resultStack);
    //
    // // 设置速度
    // f32 vx = static_cast<f32>(direction.getXOffset()) * velocity;
    // f32 vy = static_cast<f32>(direction.getYOffset()) * velocity + 0.1f;
    // f32 vz = static_cast<f32>(direction.getZOffset()) * velocity;
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

void DefaultDispenseItemBehavior::playSound(IBlockSource& source) {
    MC_UNUSED(source);
    // TODO: 播放发射音效
    // world.playSound(source.getBlockPos(), SoundEvents::BLOCK_DISPENSER_DISPENSE, 1.0f, 1.0f);
}

void DefaultDispenseItemBehavior::spawnParticles(IBlockSource& source) {
    MC_UNUSED(source);
    // TODO: 生成烟雾粒子
    // world.addParticle(ParticleTypes::SMOKE, source.getX(), source.getY(), source.getZ(), 0.0, 0.0, 0.0);
}

DispensePosition DefaultDispenseItemBehavior::getDispensePosition(IBlockSource& source, Direction direction) {
    // 计算发射位置：从方块面中心稍微向外偏移
    double offsetX = 0.5 + 0.7 * static_cast<double>(Directions::xOffset(direction));
    double offsetY = 0.5 + 0.7 * static_cast<double>(Directions::yOffset(direction));
    double offsetZ = 0.5 + 0.7 * static_cast<double>(Directions::zOffset(direction));

    return DispensePosition(source.getWorld(), source.getBlockPos(), source.getBlockState(),
                           offsetX, offsetY, offsetZ);
}

// ============================================================================
// ProjectileDispenseBehavior
// ============================================================================

ProjectileDispenseBehavior::ProjectileDispenseBehavior(i32 projectileType, f32 velocity, f32 inaccuracy)
    : m_projectileType(projectileType)
    , m_velocity(velocity)
    , m_inaccuracy(inaccuracy) {
}

ItemStack ProjectileDispenseBehavior::dispense(IBlockSource& source, ItemStack& stack) {
    // 获取发射方向
    const BlockState& state = source.getBlockState();
    Direction direction = state.get(BlockStateProperties::FACING());

    // 计算发射位置
    DispensePosition dispensePos = getDispensePosition(source, direction);

    // TODO: 创建投掷物实体
    // 当前简化实现，需要实体系统支持
    // IWorld& world = source.getWorld();
    // auto projectile = createProjectile(world, dispensePos, stack);
    //
    // // 设置发射方向和速度
    // projectile->shoot(Directions::xOffset(direction), Directions::yOffset(direction) + 0.1f, Directions::zOffset(direction),
    //                   m_velocity, m_inaccuracy);
    //
    // // 添加到世界
    // world.addEntity(std::move(projectile));

    // 减少物品数量
    stack.shrink(1);

    // 播放音效
    playSound(source);

    return stack.isEmpty() ? ItemStack() : stack;
}

} // namespace blocks
} // namespace mc
