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

#include "RedstoneComparatorBlock.hpp"
#include "../../../../entity/entities/hanging/HangingEntity.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../blockentity/redstone/ComparatorEntity.hpp"
#include "../../../redstone/RedstoneHelper.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/redstone/RedstoneDiodeBlock.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc {

// EnumProperty Traits 实现 - 必须在 mc 命名空间
std::string EnumProperty<blocks::ComparatorMode>::Traits::toString(const blocks::ComparatorMode& value)
{
    switch (value) {
        case blocks::ComparatorMode::Compare:
            return "compare";
        case blocks::ComparatorMode::Subtract:
            return "subtract";
        default:
            return "compare";
    }
}

std::optional<blocks::ComparatorMode> EnumProperty<blocks::ComparatorMode>::Traits::fromName(std::string_view name)
{
    if (name == "compare") return blocks::ComparatorMode::Compare;
    if (name == "subtract") return blocks::ComparatorMode::Subtract;
    return std::nullopt;
}

namespace blocks {

// 比较器模式属性
namespace {
const EnumProperty<ComparatorMode>& MODE_PROP()
{
    static auto prop =
        EnumProperty<ComparatorMode>::create("mode", {ComparatorMode::Compare, ComparatorMode::Subtract});
    return *prop;
}
} // namespace

RedstoneComparatorBlock::RedstoneComparatorBlock(const BlockProperties& properties)
    : RedstoneDiodeBlock("redstone_comparator", properties)
{

    // 创建状态容器 - 比较器只有三个属性：HORIZONTAL_FACING, MODE, POWERED
    // 注意：比较器没有 LOCKED 属性（与中继器不同，比较器不会被侧面信号锁定）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::POWERED())
            .add(MODE_PROP())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::POWERED(), false)
            .with(MODE_PROP(), ComparatorMode::Compare));
}

std::unique_ptr<BlockEntity> RedstoneComparatorBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::ComparatorEntity>(pos);
}

i32 RedstoneComparatorBlock::getDelay(const BlockState& state) const
{
    MC_UNUSED(state);
    return COMPARATOR_DELAY;
}

ComparatorMode RedstoneComparatorBlock::getMode(const BlockState& state)
{
    return state.get(MODE_PROP());
}

BlockState RedstoneComparatorBlock::withMode(BlockState state, ComparatorMode mode)
{
    return state.with(MODE_PROP(), mode);
}

bool RedstoneComparatorBlock::isSubtractMode(const BlockState& state)
{
    return getMode(state) == ComparatorMode::Subtract;
}

i32 RedstoneComparatorBlock::getStoredOutputSignal(IWorld& world, const BlockPos& pos) const
{
    BlockEntity* be = world.getBlockEntity(pos);
    if (auto* comparator = dynamic_cast<blockentity::ComparatorEntity*>(be)) {
        return comparator->getOutputSignal();
    }
    return 0;
}

void RedstoneComparatorBlock::storeOutputSignal(IWorld& world, const BlockPos& pos, i32 signal) const
{
    BlockEntity* be = world.getBlockEntity(pos);
    if (auto* comparator = dynamic_cast<blockentity::ComparatorEntity*>(be)) {
        comparator->setOutputSignal(signal);
    }
}

bool RedstoneComparatorBlock::shouldBePowered(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    // 正确逻辑：
    // 1. 获取主输入信号（背面）- 使用 _calculateInputStrength 检测容器信号
    // 2. 获取侧面输入信号
    // 3. 如果输入为0，不充能
    // 4. 如果输入 > 侧面信号，充能
    // 5. 如果输入 == 侧面信号且是比较模式，充能
    // 6. 否则不充能

    // 关键：使用 _calculateInputStrength 而非 getInputSignal
    // _calculateInputStrength 会检测容器信号覆盖
    i32 mainInput = _calculateInputStrength(world, pos, state);

    // 输入为0时不激活
    if (mainInput == 0) {
        return false;
    }

    i32 sidePower = getPowerOnSides(world, pos, state);

    // 主输入 > 侧面信号时激活
    if (mainInput > sidePower) {
        return true;
    }

    // 主输入 == 侧面信号时，只有比较模式才激活
    if (mainInput == sidePower) {
        return getMode(state) == ComparatorMode::Compare;
    }

    // 其他情况不激活
    return false;
}

i32 RedstoneComparatorBlock::calculateOutputSignal(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    // 从 BlockEntity 读取输出信号，实现"前端信号保持"特性
    if (!isPowered(state)) {
        return 0;
    }

    // 尝试从 BlockEntity 读取存储的信号
    i32 storedSignal = getStoredOutputSignal(world, pos);
    if (storedSignal > 0) {
        return storedSignal;
    }

    // 如果 BlockEntity 中没有存储信号，重新计算
    return _calculateOutput(world, pos, state);
}

void RedstoneComparatorBlock::onStateChanged(
    IWorld& world, const BlockPos& pos, const BlockState& oldState, const BlockState& newState)
{
    // 当状态变化时，更新 BlockEntity 中的输出信号
    if (isPowered(newState)) {
        i32 outputSignal = _calculateOutput(world, pos, newState);
        storeOutputSignal(world, pos, outputSignal);
    } else {
        storeOutputSignal(world, pos, 0);
    }
}

i32 RedstoneComparatorBlock::_calculateOutput(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    // 获取主输入信号（背面），包含容器信号检测
    i32 mainInput = _calculateInputStrength(world, pos, state);

    // 获取侧面输入信号
    i32 sidePower = getPowerOnSides(world, pos, state);

    if (isSubtractMode(state)) {
        // 减法模式：输出 = 主输入 - 侧面输入
        return std::max(0, mainInput - sidePower);
    } else {
        // 比较模式：如果主输入 >= 侧面输入，输出主输入；否则输出0
        if (mainInput >= sidePower) {
            return mainInput;
        }
        return 0;
    }
}

i32 RedstoneComparatorBlock::_calculateInputStrength(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    // 先获取基础红石信号
    i32 input = getInputSignal(world, pos, state);

    Direction facing = getFacing(state);
    BlockPos inputPos = pos.offset(Directions::opposite(facing));
    const BlockState* inputState = world.getBlockState(inputPos);

    if (!inputState || inputState->isAir()) {
        return input;
    }

    const Block& inputBlock = inputState->getBlock();

    // 检查容器信号覆盖（hasComparatorInputOverride）
    if (inputBlock.hasComparatorInputOverride(*inputState)) {
        i32 containerSignal = inputBlock.getComparatorInputOverride(*inputState, world, inputPos);
        return containerSignal;
    }

    // 如果输入信号 < 15 且输入端是实体方块
    // 检查实体方块后面是否有容器或物品展示框
    if (input < 15 && world::redstone::RedstoneHelper::isNormalCube(*inputState)) {
        BlockPos behindPos = inputPos.offset(Directions::opposite(facing));
        const BlockState* behindState = world.getBlockState(behindPos);

        if (behindState && !behindState->isAir()) {
            const Block& behindBlock = behindState->getBlock();

            // 检查后面的容器信号
            i32 maxSignal = input;
            if (behindBlock.hasComparatorInputOverride(*behindState)) {
                i32 behindSignal = behindBlock.getComparatorInputOverride(*behindState, world, behindPos);
                maxSignal = std::max(maxSignal, behindSignal);
            }

            // 检查物品展示框的模拟信号
            // 物品展示框附着在 behindPos 方块的表面上
            // 物品展示框的朝向必须与比较器的朝向相同
            entity::ItemFrameEntity* itemFrame = _findItemFrame(world, facing, behindPos);
            if (itemFrame != nullptr) {
                i32 frameSignal = itemFrame->getAnalogOutput();
                maxSignal = std::max(maxSignal, frameSignal);
            }

            return maxSignal;
        }

        // 即使 behindPos 没有方块，也要检查物品展示框
        // 物品展示框可能附着在 inputPos 方块的背面
        entity::ItemFrameEntity* itemFrame = _findItemFrame(world, facing, behindPos);
        if (itemFrame != nullptr) {
            i32 frameSignal = itemFrame->getAnalogOutput();
            input = std::max(input, frameSignal);
        }
    }

    return input;
}

BlockActionResult RedstoneComparatorBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 冒险/旁观模式下无建造权限时，禁止切换比较器模式
    if (!player.mayBuild()) {
        return ActionResultType::Pass;
    }

    // 右键点击比较器可以在比较模式和减法模式之间切换
    ComparatorMode currentMode = getMode(state);
    ComparatorMode newMode =
        (currentMode == ComparatorMode::Compare) ? ComparatorMode::Subtract : ComparatorMode::Compare;

    // 设置新模式
    BlockState newState = withMode(state, newMode);
    world.setBlockState(pos, &newState, 3);

    // 播放点击音效
    f32 pitch = (newMode == ComparatorMode::Subtract) ? 0.55f : 0.5f;
    world.playSound(
        ResourceLocation("minecraft:block.comparator.click"), sound::SoundCategory::Blocks, pos.center(), 0.3f, pitch);

    // 比较器模式改变后需要更新输出
    // 立即触发状态检查
    updateState(world, pos, newState);

    return ActionResultType::Success;
}

entity::ItemFrameEntity* RedstoneComparatorBlock::_findItemFrame(IWorld& world, Direction facing, const BlockPos& pos)
{
    // 在指定方块位置创建一个 1x1x1 的 AABB 包围盒
    AxisAlignedBB searchBox(static_cast<f64>(pos.x),
        static_cast<f64>(pos.y),
        static_cast<f64>(pos.z),
        static_cast<f64>(pos.x + 1),
        static_cast<f64>(pos.y + 1),
        static_cast<f64>(pos.z + 1));

    // 获取该区域内的所有实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(searchBox, nullptr);

    entity::ItemFrameEntity* foundFrame = nullptr;
    i32 foundCount = 0;

    for (Entity* entity : entities) {
        // 检查是否是物品展示框
        entity::ItemFrameEntity* frame = dynamic_cast<entity::ItemFrameEntity*>(entity);
        if (frame == nullptr) {
            continue;
        }

        // 物品展示框的朝向必须与比较器的朝向相同
        // 物品展示框贴在方块表面，朝向是它面向的方向
        // 比较器朝向是它输出信号的方向
        if (frame->getHorizontalFacing() == facing) {
            foundFrame = frame;
            foundCount++;
            // 只有唯一一个物品展示框时才返回
            if (foundCount > 1) {
                return nullptr; // 多个物品展示框，返回 nullptr
            }
        }
    }

    return foundFrame;
}

} // namespace blocks
} // namespace mc
