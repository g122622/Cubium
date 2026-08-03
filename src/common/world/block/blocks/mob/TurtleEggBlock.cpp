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

#include "TurtleEggBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/lighting/InternalLightUtils.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

TurtleEggBlock::TurtleEggBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::EGGS_1_4())
            .add(BlockStateProperties::HATCH_0_2())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(
        defaultState().with(BlockStateProperties::EGGS_1_4(), 1).with(BlockStateProperties::HATCH_0_2(), 0));

    // 创建各蛋数量的形状
    // 1个蛋: 3/16=0.1875, 12/16=0.75, 7/16=0.4375
    m_shapesByEggCount[0] = CollisionShape::box(0.1875f, 0.0f, 0.1875f, 0.75f, 0.4375f, 0.75f);     // 1 egg
    m_shapesByEggCount[1] = CollisionShape::box(0.0625f, 0.0f, 0.1875f, 0.9375f, 0.4375f, 0.75f);   // 2 eggs
    m_shapesByEggCount[2] = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.4375f, 0.9375f); // 3 eggs
    m_shapesByEggCount[3] = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.4375f, 0.9375f); // 4 eggs
}

i32 TurtleEggBlock::getEggs(const BlockState& state) const
{
    return state.get(BlockStateProperties::EGGS_1_4());
}

BlockState TurtleEggBlock::withEggs(i32 count) const
{
    return defaultState().with(BlockStateProperties::EGGS_1_4(), std::clamp(count, 1, 4));
}

i32 TurtleEggBlock::getHatch(const BlockState& state) const
{
    return state.get(BlockStateProperties::HATCH_0_2());
}

BlockState TurtleEggBlock::withHatch(i32 hatch) const
{
    return defaultState().with(BlockStateProperties::HATCH_0_2(), std::clamp(hatch, 0, 2));
}

BlockState TurtleEggBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 如果放置在已有的海龟蛋上，增加蛋数量
    const BlockState* existingState = world.getBlockState(pos);
    if (existingState != nullptr && existingState->is(this)) {
        i32 currentEggs = existingState->get(BlockStateProperties::EGGS_1_4());
        if (currentEggs < 4) {
            return existingState->with(BlockStateProperties::EGGS_1_4(), currentEggs + 1);
        }
        return *existingState;
    }

    return defaultState();
}

bool TurtleEggBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 海龟蛋只能放在沙子类方块上
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 检查是否为沙子类方块 (沙子、红沙、灵魂沙)
    return BlockTags::SAND().contains(*belowState);
}

bool TurtleEggBlock::_canGrow(IWorld& world, math::IRandom& random) const
{
    // 天体角度计算（MC 1.16.5 World#getCelestialAngle / DimensionType#getSkyAngle）：
    // - 0.0  = 正午 (dayTime=6000)
    // - 0.25 = 日落 (dayTime=12000)（公式实际约 0.2155）
    // - 0.5  = 午夜 (dayTime=18000)
    // - 0.78 = 日出 (dayTime=0)（公式实际约 0.7845）
    // MC 1.16.5 TurtleEggBlock#shouldUpdateHatchLevel：天体角度落在 [0.65, 0.74]
    // 区间（dayTime≈21500-22800，黎明时分）时 100% 孵化，否则 1/500 随机概率。
    f32 celestialAngle = InternalLightUtils::getCelestialAngleMC(world.dayTimeOfDay());

    if (celestialAngle >= 0.65F && celestialAngle <= 0.74F) {
        // 黎明时分，100% 孵化
        return true;
    }
    // 其他时间，1/500 随机概率
    return random.nextInt(500) == 0;
}

void TurtleEggBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 检查是否在沙子上
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!_hasProperHabitat(blockReader, pos)) {
        return;
    }

    // 检查孵化条件
    if (!_canGrow(world, random)) {
        return;
    }

    i32 hatch = getHatch(state);
    if (hatch < 2) {
        // 孵化进度增加
        if (!world.isClientSide()) {
            world.playSound(SoundEvents::ENTITY_TURTLE_EGG_CRACK,
                sound::SoundCategory::Blocks,
                pos.center(),
                0.7f,
                0.9f + random.nextFloat() * 0.2f);
        }
        const BlockState& newState = state.with(BlockStateProperties::HATCH_0_2(), hatch + 1);
        world.setBlockState(pos, &newState, 2);
    } else {
        // 孵化完成，生成海龟
        if (!world.isClientSide()) {
            world.playSound(SoundEvents::ENTITY_TURTLE_EGG_HATCH,
                sound::SoundCategory::Blocks,
                pos.center(),
                0.7f,
                0.9f + random.nextFloat() * 0.2f);
        }
        i32 eggs = getEggs(state);

        // 移除方块
        const BlockState* airState = BlockRegistry::instance().airState();
        if (airState != nullptr) {
            world.setBlockState(pos, airState, 2);
        }

        // 为每个蛋生成一只小海龟
        for (i32 i = 0; i < eggs; ++i) {
            auto turtle = std::make_unique<TurtleEntity>(EntityInstanceId(0));
            if (turtle) {
                // 设置为幼体（-24000 ticks = 20分钟）
                turtle->setChild(true);

                // 设置出生位置（小海龟会记住这个位置作为"家"）
                turtle->setHomePos(pos);

                // 设置位置：多个蛋时错开位置
                // x = pos.x + 0.3 + i * 0.2
                // z = pos.z + 0.3
                turtle->setPosition(static_cast<f32>(pos.x) + 0.3f + static_cast<f32>(i) * 0.2f,
                    static_cast<f32>(pos.y),
                    static_cast<f32>(pos.z) + 0.3f);
                turtle->setRotation(0.0f, 0.0f);

                // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化（使用位置感知的区域难度）
                entity::combat::DifficultyInstance difficultyInstance =
                    entity::combat::DifficultyInstance::at(world, pos);
                turtle->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Natural);

                // 生成到世界
                world.spawnEntity(std::move(turtle));
            }
        }
    }
}

void TurtleEggBlock::onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 实体走过时尝试踩破蛋
    _tryTrample(world, pos, state, entity, 100);
}

void TurtleEggBlock::onFallenUpon(
    IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance)
{
    // 僵尸类生物不会踩破蛋（它们会直接走过去）
    if (!_isZombieType(entity)) {
        _tryTrample(world, pos, state, entity, 3);
    }

    // 调用父类方法处理实体着地（摔落伤害等）
    // 参考: MC 1.21 TurtleEggBlock.fallOn — 总是调用 super.fallOn()
    Block::onFallenUpon(world, pos, state, entity, fallDistance);
}

bool TurtleEggBlock::_hasProperHabitat(IBlockReader& world, const BlockPos& pos) const
{
    // 检查下方是否为沙子
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    return belowState != nullptr && BlockTags::SAND().contains(*belowState);
}

bool TurtleEggBlock::_canTrample(IWorld& world, Entity& entity) const
{
    // 只有玩家或满足 mobGriefing 的生物才能踩破蛋
    // 海龟和蝙蝠不能踩破蛋

    // 获取实体类型
    const entity::EntityType* type = entity.entityType();

    // 海龟和蝙蝠不能踩破蛋
    if (type == entity::VanillaEntityTypeKeys::TURTLE || type == entity::VanillaEntityTypeKeys::BAT) {
        return false;
    }

    // 非生物实体不能踩破（物品、箭矢等）
    // 检查是否为生物实体：玩家和怪物类实体可以踩破
    // 使用动态类型检查判断是否为 LivingEntity
    auto* living = dynamic_cast<LivingEntity*>(&entity);
    if (living == nullptr) {
        return false;
    }

    // 玩家总是可以踩破
    if (type == entity::VanillaEntityTypeKeys::PLAYER) {
        return true;
    }

    // 检查 mobGriefing 游戏规则
    return world.getGameRules().getBoolean(mc::world::gamerule::GameRuleKeys::MOB_GRIEFING);
}

void TurtleEggBlock::_tryTrample(
    IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, i32 chance) const
{
    if (!_canTrample(world, entity)) {
        return;
    }

    // 随机检查
    if (world.getRandom().nextInt(chance) != 0) {
        return;
    }

    // 踩破一个蛋
    _removeOneEgg(world, pos, state);
}

void TurtleEggBlock::_removeOneEgg(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    // 播放破碎音效
    if (!world.isClientSide()) {
        world.playSound(SoundEvents::ENTITY_TURTLE_EGG_BREAK,
            sound::SoundCategory::Blocks,
            pos.center(),
            0.7f,
            0.9f + world.getRandom().nextFloat() * 0.2f);
    }

    i32 eggs = getEggs(state);
    if (eggs > 1) {
        // 减少蛋数量，重置孵化进度
        const BlockState& newState =
            state.with(BlockStateProperties::EGGS_1_4(), eggs - 1).with(BlockStateProperties::HATCH_0_2(), 0);
        world.setBlockState(pos, &newState, 2);
    } else {
        // 移除方块
        const BlockState* airState = BlockRegistry::instance().airState();
        if (airState != nullptr) {
            world.setBlockState(pos, airState, 2);
        }
    }
}

bool TurtleEggBlock::_isZombieType(Entity& entity) const
{
    // 检查实体是否为僵尸类（僵尸、尸壳、溺尸等会踩破海龟蛋）
    // 骷髅、流浪者、凋灵骷髅虽然是亡灵，但不是僵尸类，不会踩破蛋
    const entity::EntityType* type = entity.entityType();

    if (type == entity::VanillaEntityTypeKeys::ZOMBIE || type == entity::VanillaEntityTypeKeys::HUSK ||
        type == entity::VanillaEntityTypeKeys::DROWNED) {
        return true;
    }
    return false;
}

const CollisionShape& TurtleEggBlock::getShape(const BlockState& state) const
{
    i32 eggs = getEggs(state);
    return m_shapesByEggCount[static_cast<std::size_t>(std::min(eggs - 1, 3))];
}

} // namespace blocks
} // namespace mc
