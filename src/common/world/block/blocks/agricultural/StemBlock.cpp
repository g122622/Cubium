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

#include "common/world/block/blocks/agricultural/StemBlock.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mc::i32 getBonemealAgeIncrease(const mc::IWorld& world, const mc::BlockPos& pos)
{
    const mc::u64 seed = world.seed() ^ static_cast<mc::u64>(std::hash<mc::BlockPos>{}(pos));
    mc::math::Random random(seed);
    return 2 + random.nextInt(4);
}

} // namespace

namespace mc {
namespace blocks {

// ========== StemBlock 实现 ==========

StemBlock::StemBlock(const StemGrownBlock* crop, const BlockProperties& properties)
    : BushBlock(properties)
    , m_crop(crop)
{
    // 【构造顺序约束】shape 容器必须在 createBlockState 之前填充。
    // 原因：createBlockState 会为每个 BlockState 调用 _cacheProperties，其中
    // getOpacity→propagatesSkylightDown→getOcclusionShape→getShape 会在 BlockState 构造期
    // 回调 getShape。std::array::operator[] 虽不抛异常，但构造期取到的是默认构造的空
    // CollisionShape，使光照判定依赖"空 shape 恰好非 full block"的脆弱巧合。先填 shape
    // 再 createBlockState，保证构造期 getShape 返回真实形状。对齐 PointedDripstoneBlock /
    // AmethystClusterBlock 的正确构造顺序。

    // 预计算各生长阶段的形状
    // 茎是居中的小柱子，随年龄增长高度增加
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 heights[] = {2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f};

    for (int i = 0; i < 8; ++i) {
        // 茎是居中的 2x2 像素柱子
        m_shapesByAge[i] = CollisionShape::box(7.0f * P, 0.0f, 7.0f * P, 9.0f * P, heights[i] * P, 9.0f * P);
    }

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_7())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_7(), 0));
}

i32 StemBlock::getAge(const BlockState& state) const
{
    return state.get(BlockStateProperties::AGE_0_7());
}

const BlockState& StemBlock::withAge(i32 age) const
{
    return defaultState().with(BlockStateProperties::AGE_0_7(), std::min(age, getMaxAge()));
}

bool StemBlock::isMaxAge(const BlockState& state) const
{
    return getAge(state) >= getMaxAge();
}

BlockState StemBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(BlockStateProperties::AGE_0_7(), 0);
}

bool StemBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方是否为耕地
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    return canSustain(*belowState, world, belowPos);
}

void StemBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 光照检查
    if (world.getLightSubtracted(pos, 0) < game::CROP_GROWTH_LIGHT_THRESHOLD) {
        return;
    }

    // 使用与 CropBlock 相同的生长概率公式
    const f32 growthChance = CropBlock::getGrowthChance(*this, static_cast<IBlockReader&>(world), pos);
    const i32 randomBound = static_cast<i32>(25.0f / growthChance) + 1;
    if (random.nextInt(randomBound) != 0) {
        return;
    }

    const i32 age = getAge(state);
    if (age < 7) {
        // 未成熟：增加年龄
        world.setBlockState(pos, &withAge(age + 1), 2);
    } else {
        // 已成熟：尝试生成果实
        tryGrowFruit(state, world, pos, random);
    }
}

// ========== IGrowable 接口实现 ==========

bool StemBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{

    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isClientSide);

    // 只有未成熟时才能生长（原版：AGE != 7 时才能使用骨粉）
    return !isMaxAge(state);
}

bool StemBlock::canUseBonemeal(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{

    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);

    // 骨粉总是有效
    return true;
}

void StemBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{

    MC_UNUSED(random);

    const auto newAge = std::min(getAge(state) + getBonemealAgeIncrease(world, pos), getMaxAge());
    const BlockState& newState = withAge(newAge);
    world.setBlockState(pos, &newState, 2);

    // 骨粉使茎达到最大年龄后，调用 randomTick 尝试生成果实
    // 这与原版逻辑一致：performBonemeal 中 AGE 达到 7 后调用 blockstate.randomTick()
    // randomTick 内部会再次检查光照和生长概率，果实生成并非 100%
    if (newAge == getMaxAge()) {
        randomTick(world, pos, const_cast<BlockState&>(newState), random);
    }
}

void StemBlock::grow(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    math::Random random(world.seed());
    grow(world, random, pos, state);
}

const CollisionShape& StemBlock::getShape(const BlockState& state) const
{
    i32 age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 7);
    return m_shapesByAge[age];
}

bool StemBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{
    // 委托给下方方块的 canSustainPlant 方法
    // StemBlock::getPlantType() 返回 PlantType::Crop，
    // Block::canSustainPlant 的 Crop 分支只接受 Farmland
    const Block& groundBlock = groundState.getBlock();
    return groundBlock.canSustainPlant(groundState, static_cast<IBlockReader&>(world), groundPos, Direction::Up, *this);
}

PlantType StemBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Crop;
}

bool StemBlock::tryGrowFruit(const BlockState& state, IWorld& world, const BlockPos& pos, math::IRandom& random)
{
    MC_UNUSED(state);

    // 随机打乱四个水平方向，依次尝试每个方向直到成功
    // 参考 MC Java StemBlock.randomTick: Direction.Plane.HORIZONTAL.shuffle(random)
    Direction directions[] = {Direction::North, Direction::South, Direction::East, Direction::West};

    // Fisher-Yates 洗牌
    for (int i = 3; i > 0; --i) {
        int j = random.nextInt(i + 1);
        Direction temp = directions[i];
        directions[i] = directions[j];
        directions[j] = temp;
    }

    for (int i = 0; i < 4; ++i) {
        Direction dir = directions[i];
        BlockPos fruitPos(pos.x + Directions::xOffset(dir), pos.y, pos.z + Directions::zOffset(dir));

        // 检查果实位置是否为空气
        const BlockState* fruitState = world.getBlockState(fruitPos);
        if (fruitState == nullptr || !fruitState->isAir()) {
            continue;
        }

        // 检查果实下方是否为耕地或 DIRT 标签方块
        BlockPos belowFruitPos(fruitPos.x, fruitPos.y - 1, fruitPos.z);
        const BlockState* belowFruitState = world.getBlockState(belowFruitPos);

        if (belowFruitState == nullptr) {
            continue;
        }

        const bool canSupportFruit =
            (VanillaBlocks::FARMLAND != nullptr && belowFruitState->is(VanillaBlocks::FARMLAND)) ||
            BlockTags::DIRT().contains(*belowFruitState);
        if (!canSupportFruit) {
            continue;
        }

        // 放置果实
        if (m_crop != nullptr) {
            const BlockState& cropDefaultState = m_crop->defaultState();
            world.setBlockState(fruitPos, &cropDefaultState, 3);

            // 将茎变为连接茎，朝向果实方向
            const Block* attachedStem = m_crop->getAttachedStem();
            if (attachedStem != nullptr) {
                const BlockState& stemState =
                    attachedStem->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), dir);
                world.setBlockState(pos, &stemState, 3);
            }

            return true;
        }
    }

    return false;
}

// ========== AttachedStemBlock 实现 ==========

AttachedStemBlock::AttachedStemBlock(const StemGrownBlock* crop, const BlockProperties& properties)
    : BushBlock(properties)
    , m_crop(crop)
{
    // 【构造顺序约束】shape 容器必须在 createBlockState 之前填充。
    // 原因：createBlockState 会为每个 BlockState 调用 _cacheProperties，其中
    // getOpacity→propagatesSkylightDown→getOcclusionShape→getShape 会在 BlockState 构造期
    // （此时本构造函数体尚未执行到此处）回调 getShape。若 m_shapesByDirection 此时为空，
    // getShape 的 fallback（m_shapesByDirection.at(North)）会命中空 unordered_map 抛
    // std::out_of_range("invalid unordered_map<K, T> key")。对齐 PointedDripstoneBlock /
    // AmethystClusterBlock 的正确构造顺序（先填 shape 再 createBlockState）。

    // 预计算各方向的形状
    // 形状从中心 (6, 0, 6) 延伸到对应方向的边缘，高度 10 像素
    constexpr f32 P = 1.0f / 16.0f;

    // North: (6, 0, 0) -> (10, 10, 10)
    m_shapesByDirection[Direction::North] = CollisionShape::box(6.0f * P, 0.0f, 0.0f, 10.0f * P, 10.0f * P, 10.0f * P);

    // South: (6, 0, 6) -> (10, 10, 16)
    m_shapesByDirection[Direction::South] =
        CollisionShape::box(6.0f * P, 0.0f, 6.0f * P, 10.0f * P, 10.0f * P, 16.0f * P);

    // West: (0, 0, 6) -> (10, 10, 10)
    m_shapesByDirection[Direction::West] =
        CollisionShape::box(0.0f * P, 0.0f, 6.0f * P, 10.0f * P, 10.0f * P, 10.0f * P);

    // East: (6, 0, 6) -> (16, 10, 10)
    m_shapesByDirection[Direction::East] =
        CollisionShape::box(6.0f * P, 0.0f, 6.0f * P, 16.0f * P, 10.0f * P, 10.0f * P);

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));
}

BlockState AttachedStemBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), context.horizontalDirection());
}

BlockState AttachedStemBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // 获取茎指向的方向
    Direction stemFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 如果更新方向是茎指向的方向，检查该方向是否还是果实
    if (facing == stemFacing) {
        // 检查邻居是否是对应的果实方块
        if (m_crop != nullptr && !facingState.is(m_crop)) {
            // 果实不存在了，变回普通茎（AGE=7）
            const Block* stem = m_crop->getStem();
            if (stem != nullptr) {
                return stem->defaultState().with(BlockStateProperties::AGE_0_7(), 7);
            }
        }
    }

    // 其他情况调用父类处理（下方支撑检查等）
    return BushBlock::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

const CollisionShape& AttachedStemBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    auto it = m_shapesByDirection.find(facing);
    if (it != m_shapesByDirection.end()) {
        return it->second;
    }
    // 默认返回北方向的形状
    return m_shapesByDirection.at(Direction::North);
}

const BlockState& AttachedStemBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& AttachedStemBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

PlantType AttachedStemBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Crop;
}

} // namespace blocks
} // namespace mc
