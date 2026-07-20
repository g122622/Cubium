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

#include "DetectorRailBlock.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/redstone/RedstoneHelper.hpp"

namespace mc {
namespace blocks {

DetectorRailBlock::DetectorRailBlock(const BlockProperties& properties)
    : AbstractRailBlock(properties, true, true) // isStraight=true: 探测铁轨不支持弯轨, isPowered=true: 可提供红石信号
{
    // 创建状态容器（含 SHAPE、POWERED 和 WATERLOGGED 属性）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(SHAPE())
            .add(POWERED())
            .add(BlockStateProperties::WATERLOGGED())
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
            .with(POWERED(), false)
            .with(SHAPE(), RailShape::NorthSouth)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

void DetectorRailBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态容器在构造函数中创建，此方法留空
    MC_UNUSED(container);
}

void DetectorRailBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 检测矿车并更新状态
    bool shouldBePowered = false;

    // 创建检测区域（铁轨上方一格高度）
    AxisAlignedBB searchBox = AxisAlignedBB::fromBlock(pos.x, pos.y, pos.z);

    // 获取区域内的实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(searchBox, nullptr);

    // 检查是否有矿车（包括所有矿车类型）
    for (Entity* entity : entities) {
        if (!entity || entity->isRemoved()) {
            continue;
        }

        // 检查是否为矿车类型（所有矿车都继承自 AbstractMinecartEntity）
        auto* minecart = dynamic_cast<entity::AbstractMinecartEntity*>(entity);
        if (minecart != nullptr) {
            shouldBePowered = true;
            break;
        }
    }

    bool isCurrentlyPowered = isPowered(state);
    if (shouldBePowered != isCurrentlyPowered) {
        // 更新状态 - 修改传入的state引用
        state = state.with(POWERED(), shouldBePowered);
        world.setBlockState(pos.x, pos.y, pos.z, &state, 3);

        // 通知相邻方块更新
        world.updateNeighbors(pos, *this);
    }
}

i32 DetectorRailBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    if (isPowered(state)) {
        return 15;
    }
    return 0;
}

i32 DetectorRailBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 探测铁轨只向上输出强信号
    if (isPowered(state) && side == Direction::Up) {
        return 15;
    }
    return 0;
}

bool DetectorRailBlock::hasComparatorInputOverride(const BlockState& state) const noexcept
{
    MC_UNUSED(state);
    // 探测铁轨有比较器信号覆盖（基于矿车内容物）
    return true;
}

i32 DetectorRailBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    if (!isPowered(state)) {
        return 0;
    }

    // 在检测区域查找矿车实体
    // 搜索范围比 tick() 中稍小，与MC原版一致
    AxisAlignedBB searchBox(static_cast<f64>(pos.x) + 0.2,
        static_cast<f64>(pos.y),
        static_cast<f64>(pos.z) + 0.2,
        static_cast<f64>(pos.x) + 0.8,
        static_cast<f64>(pos.y) + 0.8,
        static_cast<f64>(pos.z) + 0.8);

    std::vector<Entity*> entities = world.getEntitiesInAABB(searchBox, nullptr);

    // 优先级1：命令方块矿车（返回命令成功次数）
    for (Entity* entity : entities) {
        if (!entity || entity->isRemoved()) {
            continue;
        }
        auto* commandMinecart = dynamic_cast<entity::CommandBlockMinecartEntity*>(entity);
        if (commandMinecart != nullptr) {
            return commandMinecart->getComparatorOutput();
        }
    }

    // 优先级2：容器矿车（箱子矿车、漏斗矿车，基于容器填充率）
    // 使用 getEntitySignal() 获取搜索区域内的最大比较器信号
    i32 entitySignal = world::redstone::RedstoneHelper::getEntitySignal(world, searchBox);
    if (entitySignal > 0) {
        return entitySignal;
    }

    // 非容器矿车（普通矿车、TNT矿车、熔炉矿车等）不产生比较器信号
    return 0;
}

RailShape DetectorRailBlock::getRailShape(const BlockState& state) const
{
    return state.get(SHAPE());
}

BlockState DetectorRailBlock::withRailShape(const BlockState& state, RailShape shape) const
{
    return state.with(SHAPE(), shape);
}

bool DetectorRailBlock::isPowered(const BlockState& state)
{
    return state.get(POWERED());
}

} // namespace blocks
} // namespace mc
