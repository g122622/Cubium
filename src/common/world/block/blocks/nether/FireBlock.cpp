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

#include "FireBlock.hpp"
#include "../../../../entity/combat/DifficultyHelper.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../entity/damage/DamageSource.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorld.hpp"
#include "../../../dimension/DimensionManager.hpp"
#include "../../../dimension/teleport/PortalSize.hpp"
#include "../../BlockRegistry.hpp"
#include "../../BlockTags.hpp"
#include "../../FireInfoRegistry.hpp"
#include "SoulFireBlock.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== FireBlock ==========

FireBlock::FireBlock(const BlockProperties& properties, i32 fireDamage)
    : Block(properties)
    , m_fireDamage(fireDamage)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_15())
            .add(BlockStateProperties::NORTH())
            .add(BlockStateProperties::SOUTH())
            .add(BlockStateProperties::EAST())
            .add(BlockStateProperties::WEST())
            .add(BlockStateProperties::UP())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::AGE_0_15(), 0)
            .with(BlockStateProperties::NORTH(), false)
            .with(BlockStateProperties::SOUTH(), false)
            .with(BlockStateProperties::EAST(), false)
            .with(BlockStateProperties::WEST(), false)
            .with(BlockStateProperties::UP(), false));

    // 火焰形状（无碰撞）
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

i32 FireBlock::getAge(const BlockState& state) const
{
    return state.get(BlockStateProperties::AGE_0_15());
}

BlockState FireBlock::withAge(i32 age) const
{
    return defaultState().with(BlockStateProperties::AGE_0_15(), std::min(age, 15));
}

const BlockState& FireBlock::getFireState(IWorld& world, const BlockPos& pos)
{
    // 检查目标位置下方是否为灵魂火基座方块（灵魂沙/灵魂土）
    const BlockState* belowState = world.getBlockState(pos.down());
    if (belowState != nullptr && SoulFireBlock::isSoulFireBase(&belowState->getBlock())) {
        return VanillaBlocks::SOUL_FIRE->defaultState();
    }
    return VanillaBlocks::FIRE->defaultState();
}

BlockState FireBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据环境选择正确的火焰类型
    return FireBlock::getFireState(context.getWorld(), context.placementPos());
}

bool FireBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查周围是否有可支撑的方块
    for (Direction dir :
        {Direction::North, Direction::South, Direction::East, Direction::West, Direction::Up, Direction::Down}) {
        BlockPos adjPos = pos.offset(dir);
        const BlockState* adjState = world.getBlockState(adjPos);

        if (adjState != nullptr && adjState->isSolid()) {
            return true;
        }
    }

    return canBurn(world, pos);
}

BlockState FireBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingPos);

    // 检查是否仍然有支撑
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!isValidPosition(state, blockReader, currentPos)) {
        if (auto* airState = BlockRegistry::instance().airState()) {
            return *airState;
        }
    }

    // 更新连接状态
    bool connected = false;
    switch (facing) {
        case Direction::North:
            connected = facingState.isSolid();
            return state.with(BlockStateProperties::NORTH(), connected);
        case Direction::South:
            connected = facingState.isSolid();
            return state.with(BlockStateProperties::SOUTH(), connected);
        case Direction::East:
            connected = facingState.isSolid();
            return state.with(BlockStateProperties::EAST(), connected);
        case Direction::West:
            connected = facingState.isSolid();
            return state.with(BlockStateProperties::WEST(), connected);
        case Direction::Up:
            connected = facingState.isSolid();
            return state.with(BlockStateProperties::UP(), connected);
        default:
            return state;
    }
}

void FireBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 1. 检查位置有效性
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!isValidPosition(state, blockReader, pos)) {
        world.setBlockState(pos, nullptr, 3);
        return;
    }

    // 2. 检查游戏规则
    if (!world.doFireTick()) {
        return;
    }

    // 3. 检查是否为无限火源（如下界岩上的火）
    bool isFireSource = false;
    const BlockState* belowState = world.getBlockState(pos.down());
    if (belowState != nullptr) {
        isFireSource = belowState->isFireSource(world, pos.down(), Direction::Up);
    }

    // 4. 下雨熄灭检查
    i32 age = getAge(state);
    if (!isFireSource && world.isRaining() && canDie(world, pos)) {
        // 熄灭概率: 0.2 + age * 0.03
        f32 extinguishChance = 0.2f + static_cast<f32>(age) * 0.03f;
        if (random.nextFloat() < extinguishChance) {
            world.setBlockState(pos, nullptr, 3);
            return;
        }
    }

    // 5. 火焰年龄增长
    if (age < 15 && random.nextInt(3) == 0) {
        BlockState newState = withAge(age + 1);
        world.setBlockState(pos, &newState, 2);
        state = newState; // 更新本地状态引用
        age = age + 1;
    }

    // 6. 无可燃邻居时的处理
    if (!isFireSource) {
        if (!areNeighborsFlammable(blockReader, pos)) {
            // 没有可燃邻居，检查下方是否有支撑
            const BlockState* supportState = world.getBlockState(pos.down());
            bool hasSolidBelow = supportState != nullptr && supportState->isSolidSide(world, pos.down(), Direction::Up);

            if (!hasSolidBelow || age > 3) {
                world.setBlockState(pos, nullptr, 3);
                return;
            }
        } else {
            // 有可燃邻居，但年龄达到最大且有概率熄灭
            if (age == 15 && random.nextInt(4) == 0) {
                // 检查下方是否可燃
                if (!canCatchFire(world, pos.down(), Direction::Up)) {
                    world.setBlockState(pos, nullptr, 3);
                    return;
                }
            }
        }
    }

    // 7. 尝试蔓延
    trySpread(world, pos, age, random);
}

void FireBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 火焰方块被放置时，立即检测并点燃下界传送门

    // 维度检查：只允许在主世界和下界点燃下界传送门
    DimensionId dimensionId = world.dimension();

    if (dimensionId != DimensionManager::OVERWORLD && dimensionId != DimensionManager::NETHER) {
        // 不在主世界或下界，不检测传送门
        return;
    }

    // 尝试点燃下界传送门（先尝试 X 轴，再尝试 Z 轴）
    if (tryLightNetherPortal(world, pos)) {
        return;
    }
}

bool FireBlock::tryLightNetherPortal(IWorld& world, const BlockPos& pos)
{
    // 检查火焰周围是否形成有效的下界传送门框架

    if (VanillaBlocks::OBSIDIAN == nullptr || VanillaBlocks::NETHER_PORTAL == nullptr) {
        return false;
    }

    // 先尝试 X 轴，再尝试 Z 轴
    // PortalSize::findNetherPortal 会先尝试 preferXAxis 指定的轴向
    auto portalResult = PortalSize::findNetherPortal(world, pos, true);

    if (portalResult.has_value() && portalResult->valid) {
        // 找到有效的传送门框架，点燃传送门
        PortalSize::lightNetherPortal(world, portalResult.value());
        return true;
    }

    return false;
}

void FireBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    i32 age = getAge(state);

    // 火焰可能熄灭
    if (age < 15 && random.nextInt(3) == 0) {
        BlockState newState = withAge(age + 1);
        world.setBlockState(pos, &newState, 2);
    }

    // 尝试蔓延
    trySpread(world, pos, age, random);
}

void FireBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    MC_UNUSED(pos);

    // MC Java: BaseFireBlock.entityInside() -> InsideBlockEffectType.FIRE_IGNITE -> BaseFireBlock.fireIgnite()
    // 1. 检查实体是否免疫火焰
    if (entity.isImmuneToFire()) {
        return;
    }

    // 2. 处理火焰免疫期倒计时
    // MC Java: 如果 remainingFireTicks < 0（免疫期），每 tick 增加 1 直到归零
    i32 fireTicks = entity.getRemainingFireTicks();
    if (fireTicks < 0) {
        entity.setRemainingFireTicks(fireTicks + 1);
    } else {
        // 3. 非免疫期，增加火焰计时器（每个碰撞 tick 增加 1）
        entity.forceFireTicks(fireTicks + 1);
    }

    // 4. 如果火焰计时器 >= 0（不在免疫期），点燃 8 秒（160 ticks）
    if (entity.getRemainingFireTicks() >= 0) {
        entity.igniteForSeconds(8.0f);
    }

    // 5. 造成火焰伤害
    auto damageSource = DamageSources::inFire();
    entity.hurt(damageSource, static_cast<f32>(m_fireDamage));
}

const CollisionShape& FireBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& FireBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

bool FireBlock::canBurn(IBlockReader& world, const BlockPos& pos) const
{
    // 检查指定位置是否可以燃烧（检查周围是否有可燃方块）

    // 遍历6个方向，检查是否有可燃方块
    for (Direction dir :
        {Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos adjPos = pos.offset(dir);
        const BlockState* adjState = world.getBlockState(adjPos);

        if (adjState != nullptr && adjState->getFireSpreadSpeed(&world, &adjPos, Directions::opposite(dir)) > 0) {
            return true;
        }
    }

    return false;
}

void FireBlock::trySpread(IWorld& world, const BlockPos& pos, i32 age, math::IRandom& random)
{
    // 检查游戏规则
    if (!world.doFireTick()) {
        return;
    }

    // 获取难度相关的火焰蔓延加成
    // 公式: (encouragement + 40 + difficulty * 7) / (age + 30)
    Difficulty difficulty = world.difficulty();
    i32 difficultyBonus = entity::combat::DifficultyHelper::getFireSpreadBonus(difficulty);

    // 检查是否在下雨区域（高湿度）
    bool isHighHumidity = world.isRaining() && canDie(world, pos);

    // ===== 1. 直接相邻方块的燃烧 =====

    // 湿度惩罚：高湿度时 -50
    i32 humidityPenalty = isHighHumidity ? -50 : 0;

    // 垂直方向（上和下）：chance = 250 + humidityPenalty
    // 水平方向（4个方向）：chance = 300 + humidityPenalty
    tryCatchFire(world, pos.up(), 250 + humidityPenalty, random, age, Direction::Down);
    tryCatchFire(world, pos.down(), 250 + humidityPenalty, random, age, Direction::Up);
    tryCatchFire(world, pos.north(), 300 + humidityPenalty, random, age, Direction::South);
    tryCatchFire(world, pos.south(), 300 + humidityPenalty, random, age, Direction::North);
    tryCatchFire(world, pos.east(), 300 + humidityPenalty, random, age, Direction::West);
    tryCatchFire(world, pos.west(), 300 + humidityPenalty, random, age, Direction::East);

    // ===== 2. 远距离蔓延 =====

    // 蔓延范围：x: -1~1, y: -1~4, z: -1~1
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            for (i32 dy = -1; dy <= 4; ++dy) {
                // 跳过火焰自身位置
                if (dx == 0 && dy == 0 && dz == 0) {
                    continue;
                }

                BlockPos targetPos(pos.x + dx, pos.y + dy, pos.z + dz);

                // 基础难度值
                i32 baseChance = 100;

                // 高度惩罚：每向上一层 +100
                if (dy > 1) {
                    baseChance += (dy - 1) * 100;
                }

                // 获取目标位置的邻居鼓励值
                i32 neighborEncouragement = getNeighborEncouragement(world, targetPos);

                if (neighborEncouragement > 0) {
                    // 计算蔓延概率
                    // 公式: (encouragement + 40 + difficultyBonus) / (age + 30)
                    i32 spreadChance = (neighborEncouragement + 40 + difficultyBonus) / (age + 30);

                    // 高湿度时减半
                    if (isHighHumidity) {
                        spreadChance /= 2;
                    }

                    // 执行蔓延检查
                    if (spreadChance > 0 && random.nextInt(baseChance) <= spreadChance) {
                        // 检查目标位置是否会被雨淋灭
                        if (!world.isRaining() || !canDieAt(world, targetPos)) {
                            // 设置火焰
                            i32 newAge = std::min(15, age + random.nextInt(5) / 4);
                            BlockState fireState = withAge(newAge);
                            world.setBlockState(targetPos, &fireState, 3);
                        }
                    }
                }
            }
        }
    }
}

bool FireBlock::canDie(IWorld& world, const BlockPos& pos) const
{
    // 检查火焰位置或相邻位置是否在下雨

    if (world.canRainAt(pos)) {
        return true;
    }

    // 检查相邻4个水平方向
    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
        if (world.canRainAt(pos.offset(dir))) {
            return true;
        }
    }

    return false;
}

bool FireBlock::canDieAt(IWorld& world, const BlockPos& pos) const
{
    // 同 canDie，用于远距离蔓延检查
    return canDie(world, pos);
}

i32 FireBlock::getNeighborEncouragement(IWorld& world, const BlockPos& pos) const
{
    // 获取目标位置周围的最大火焰蔓延速度

    const BlockState* targetState = world.getBlockState(pos);
    if (targetState != nullptr && !targetState->isAir()) {
        return 0; // 目标位置不是空气
    }

    i32 maxEncouragement = 0;

    // 检查6个方向
    for (Direction dir :
        {Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos adjPos = pos.offset(dir);
        const BlockState* adjState = world.getBlockState(adjPos);

        if (adjState != nullptr) {
            i32 encouragement = adjState->getFireSpreadSpeed(&world, &adjPos, Directions::opposite(dir));
            maxEncouragement = std::max(maxEncouragement, encouragement);
        }
    }

    return maxEncouragement;
}

void FireBlock::tryCatchFire(
    IWorld& world, const BlockPos& pos, i32 chance, math::IRandom& random, i32 age, Direction face)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || state->isAir()) {
        return;
    }

    // 获取可燃性
    i32 flammability = state->getFlammability(&world, &pos, face);

    // 检查是否可燃
    if (flammability <= 0) {
        return;
    }

    // 燃烧概率检查
    if (random.nextInt(chance) >= flammability) {
        return;
    }

    // 检查含水状态
    if (state->hasProperty(BlockStateProperties::WATERLOGGED()) && state->get(BlockStateProperties::WATERLOGGED())) {
        return; // 含水方块不可燃
    }

    // ===== 点燃或烧毁 =====
    // 5% 基础概率点燃，否则直接烧毁

    bool shouldIgnite = random.nextInt(age + 10) < 5 && !world.canRainAt(pos);

    if (shouldIgnite) {
        // 点燃：设置火焰方块
        i32 newAge = std::min(15, age + random.nextInt(5) / 4);
        BlockState fireState = withAge(newAge);
        world.setBlockState(pos, &fireState, 3);
    } else {
        // 直接烧毁：移除方块
        // MC Java 中火焰烧毁方块时不调用 spawnAfterBreak，仅移除方块
        world.setBlockState(pos, nullptr, 3);
    }

    // 触发燃烧回调（如 TNT 爆炸）
    state->catchFire(world, pos, face, nullptr);
}

bool FireBlock::areNeighborsFlammable(IBlockReader& world, const BlockPos& pos) const
{
    // 检查周围是否有可燃方块

    for (Direction dir :
        {Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos adjPos = pos.offset(dir);
        const BlockState* adjState = world.getBlockState(adjPos);

        if (adjState != nullptr && adjState->getFireSpreadSpeed(&world, &adjPos, Directions::opposite(dir)) > 0) {
            return true;
        }
    }

    return false;
}

bool FireBlock::canCatchFire(IWorld& world, const BlockPos& pos, Direction face) const
{
    // 检查指定位置是否可以被点燃

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    // 检查含水状态
    if (state->hasProperty(BlockStateProperties::WATERLOGGED()) && state->get(BlockStateProperties::WATERLOGGED())) {
        return false; // 含水方块不可燃
    }

    // 检查可燃性
    return state->getFlammability(&world, &pos, face) > 0;
}

bool FireBlock::isFlammable(const BlockState& state) const
{
    return state.getMaterial().isFlammable();
}

} // namespace blocks
} // namespace mc
