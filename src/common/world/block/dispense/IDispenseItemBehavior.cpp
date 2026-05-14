#include "IDispenseItemBehavior.hpp"
#include "../../../core/Types.hpp"
#include "../../../entity/entities/item/ItemEntity.hpp"
#include "../../../entity/entities/projectile/AbstractArrowEntity.hpp"
#include "../../../entity/entities/projectile/OtherProjectiles.hpp"
#include "../../../entity/entities/projectile/ProjectileEntity.hpp"
#include "../../../entity/entities/projectile/ProjectileItemEntity.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../sound/SoundCategory.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../../world/WorldEvents.hpp"
#include "../../IWorld.hpp"
#include "../Block.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// DefaultDispenseItemBehavior
// ============================================================================

ItemStack DefaultDispenseItemBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack)
{
    // 获取发射方向
    Direction direction = state.get(BlockStateProperties::FACING());

    // 执行投掷
    ItemStack result = doDispense(world, pos, state, stack, direction, 6.0f, 6.0f);

    // 播放音效和粒子
    playSound(world, pos);
    spawnParticles(world, pos, direction);

    return result;
}

ItemStack DefaultDispenseItemBehavior::doDispense(IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    ItemStack& stack,
    Direction direction,
    f32 speed,
    f32 inaccuracy)
{
    MC_UNUSED(state);
    MC_UNUSED(inaccuracy);

    if (stack.isEmpty()) {
        return stack;
    }

    // 分离出1个物品
    ItemStack dispensedStack = stack.split(1);
    if (dispensedStack.isEmpty()) {
        return stack;
    }

    // 计算发射位置（MC 1.16.5 算法）
    // 发射口位置 = 方块中心 + 方向偏移 * 0.7
    Vector3 dispensePos = getDispensePosition(pos, direction);

    // Y轴偏移调整（MC 1.16.5 DefaultDispenseItemBehavior.doDispense）
    // 使物品看起来从发射口出来
    f32 adjustedY = dispensePos.y;
    if (Directions::getAxis(direction) == Axis::Y) {
        adjustedY -= 0.125f; // 向上/向下时
    } else {
        adjustedY -= 0.15625f; // 水平方向时
    }

    // 获取随机数生成器
    math::Random& rng = world.getRandom();

    // 计算速度（MC 1.16.5 算法）
    // 基础速度 d3 = random(0.1) + 0.2，范围 [0.2, 0.3]
    f32 baseVelocity = static_cast<f32>(rng.nextDouble() * 0.1 + 0.2);

    // 速度计算：
    // vx = random.gaussian() * 0.0075 * speed + direction.xOffset * baseVelocity
    // vy = random.gaussian() * 0.0075 * speed + 0.2
    // vz = random.gaussian() * 0.0075 * speed + direction.zOffset * baseVelocity
    f32 gaussianFactor = 0.0075f * speed;
    f32 vx = static_cast<f32>(rng.nextGaussian()) * gaussianFactor +
        static_cast<f32>(Directions::xOffset(direction)) * baseVelocity;
    f32 vy = static_cast<f32>(rng.nextGaussian()) * gaussianFactor + 0.2f;
    f32 vz = static_cast<f32>(rng.nextGaussian()) * gaussianFactor +
        static_cast<f32>(Directions::zOffset(direction)) * baseVelocity;

    // 创建物品实体（ItemEntity在mc命名空间，不在mc::entity中）
    auto itemEntity = std::make_unique<ItemEntity>(EntityId(0), // ID由世界分配
        dispensedStack,
        dispensePos.x,
        adjustedY,
        dispensePos.z,
        vx,
        vy,
        vz);

    // 设置拾取延迟（发射器发射的物品不能立即被拾取）
    itemEntity->setPickupDelay(10);

    // 添加到世界
    world.spawnEntity(std::move(itemEntity));

    // 返回剩余物品
    return stack;
}

void DefaultDispenseItemBehavior::playSound(IWorld& world, const BlockPos& pos)
{
    // MC 1.16.5: 播放发射音效 (事件ID 1000)
    // 参考: DefaultDispenseItemBehavior.playSound()
    if (!world.isClientSide()) {
        world.playEvent(world::WorldEvents::DISPENSER_DISPENSE_SOUND, pos, 0);
    }
}

void DefaultDispenseItemBehavior::spawnParticles(IWorld& world, const BlockPos& pos, Direction direction)
{
    // MC 1.16.5: 生成发射烟雾粒子 (事件ID 2000，数据为方向索引)
    // 参考: DefaultDispenseItemBehavior.spawnDispenseParticles()
    if (!world.isClientSide()) {
        // 服务端通过世界事件广播粒子
        world.playEvent(world::WorldEvents::DISPENSER_SMOKE, pos, static_cast<i32>(direction));
    }
}

Vector3 DefaultDispenseItemBehavior::getDispensePosition(const BlockPos& pos, Direction direction)
{
    // 计算发射位置：从方块面中心稍微向外偏移
    // MC 1.16.5: DispenserBlock.getDispensePosition()
    f32 x = static_cast<f32>(pos.x) + 0.5f + static_cast<f32>(Directions::xOffset(direction)) * 0.7f;
    f32 y = static_cast<f32>(pos.y) + 0.5f + static_cast<f32>(Directions::yOffset(direction)) * 0.7f;
    f32 z = static_cast<f32>(pos.z) + 0.5f + static_cast<f32>(Directions::zOffset(direction)) * 0.7f;
    return Vector3(x, y, z);
}

// ============================================================================
// OptionalDispenseItemBehavior
// ============================================================================

void OptionalDispenseItemBehavior::playSound(IWorld& world, const BlockPos& pos)
{
    // MC 1.16.5: 根据成功/失败播放不同音效
    // 成功: 1000 (DISPENSER_DISPENSE_SOUND)
    // 失败: 1001 (DISPENSER_FAIL_SOUND)
    if (!world.isClientSide()) {
        i32 eventId =
            m_success ? world::WorldEvents::DISPENSER_DISPENSE_SOUND : world::WorldEvents::DISPENSER_FAIL_SOUND;
        world.playEvent(eventId, pos, 0);
    }
}

void OptionalDispenseItemBehavior::spawnParticles(IWorld& world, const BlockPos& pos, Direction direction)
{
    // 只有成功时才生成粒子
    if (m_success) {
        DefaultDispenseItemBehavior::spawnParticles(world, pos, direction);
    }
}

// ============================================================================
// ProjectileDispenseBehavior
// ============================================================================

ProjectileDispenseBehavior::ProjectileDispenseBehavior(
    std::function<std::unique_ptr<mc::Entity>(IWorld&, const Vector3&, const ItemStack&)> createProjectile,
    f32 velocity,
    f32 inaccuracy)
    : m_createProjectile(std::move(createProjectile))
    , m_velocity(velocity)
    , m_inaccuracy(inaccuracy)
{}

ItemStack ProjectileDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack)
{
    // 获取发射方向
    Direction direction = state.get(BlockStateProperties::FACING());

    // 计算发射位置
    Vector3 dispensePos = getDispensePosition(pos, direction);

    // 创建投掷物实体
    std::unique_ptr<mc::Entity> projectile = m_createProjectile(world, dispensePos, stack);
    if (!projectile) {
        // 创建失败，使用默认行为
        return DefaultDispenseItemBehavior::dispense(world, pos, state, stack);
    }

    // 设置投掷物的位置
    projectile->setPosition(dispensePos.x, dispensePos.y, dispensePos.z);

    // 转换为 ProjectileEntity 指针
    entity::ProjectileEntity* projectileEntity = dynamic_cast<entity::ProjectileEntity*>(projectile.get());
    if (!projectileEntity) {
        // 不是投掷物，使用默认行为
        return DefaultDispenseItemBehavior::dispense(world, pos, state, stack);
    }

    // 设置发射方向和速度（MC 1.16.5 算法）
    // shoot(x, y, z, velocity, inaccuracy)
    // Y方向额外+0.1使投掷物稍向上
    projectileEntity->shoot(static_cast<f32>(Directions::xOffset(direction)),
        static_cast<f32>(Directions::yOffset(direction)) + 0.1f,
        static_cast<f32>(Directions::zOffset(direction)),
        m_velocity,
        m_inaccuracy);

    // 添加到世界
    world.spawnEntity(std::move(projectile));

    // 减少物品数量
    stack.shrink(1);

    // 播放投掷物发射音效（事件ID 1002）
    if (!world.isClientSide()) {
        world.playEvent(world::WorldEvents::DISPENSER_LAUNCH_SOUND, pos, 0);
    }

    // 生成烟雾粒子
    spawnParticles(world, pos, direction);

    return stack.isEmpty() ? ItemStack() : stack;
}

} // namespace blocks
} // namespace mc
