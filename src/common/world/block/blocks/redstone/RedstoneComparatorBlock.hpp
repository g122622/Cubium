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

#pragma once

#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/core/ActionResult.hpp"
#include "../../../../util/property/Properties.hpp"
#include "RedstoneDiodeBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace mc {

class BlockEntity;

namespace entity {
class ItemFrameEntity;
}

namespace blocks {

/**
 * @brief 红石比较器模式
 */
enum class ComparatorMode : u8 {
    Compare = 0, ///< 比较模式
    Subtract = 1 ///< 减法模式
};

} // namespace blocks

// 特化 EnumProperty::Traits for ComparatorMode
template <>
struct EnumProperty<blocks::ComparatorMode>::Traits {
    static std::string toString(const blocks::ComparatorMode& value);
    static std::optional<blocks::ComparatorMode> fromName(std::string_view name);
};

namespace blocks {

/**
 * @brief 红石比较器方块
 *
 * 红石比较器可以比较信号强度、检测容器内容物。
 *
 * ## 特性
 * - 比较模式：输出较强信号
 * - 减法模式：输出差值信号
 * - 容器检测：可以检测容器内容物
 * - 2 tick延迟
 * - 不受侧面信号锁定（与中继器不同）
 * - 前端信号保持（需要 BlockEntity 存储）
 *
 * ## 容易踩的坑
 * - 比较器需要 BlockEntity 存储输出信号强度
 * - 比较器不会被侧面信号锁定（这与中继器不同）
 * - 减法模式计算复杂
 * - 方向性处理
 * - 输出信号在 tick 时存储到 BlockEntity，而不是实时计算
 */
class RedstoneComparatorBlock : public RedstoneDiodeBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RedstoneComparatorBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    /**
     * @brief 是否有方块实体
     * @return true 比较器需要 BlockEntity 存储输出信号
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    /**
     * @brief 创建方块实体
     * @param pos 方块位置
     * @return 新创建的比较器方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 红石二极管接口实现 ==========

    [[nodiscard]] i32 getDelay(const BlockState& state) const override;

    [[nodiscard]] bool shouldBePowered(IWorld& world, const BlockPos& pos, const BlockState& state) const override;

    [[nodiscard]] i32 calculateOutputSignal(IWorld& world, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 检查是否被锁定
     *
     * 比较器不会被侧面信号锁定（与中继器不同）。
     * 红石二极管基类默认返回侧面信号检测，比较器需要重写为始终返回 false。
     *
     * @return false 比较器永远不被锁定
     */
    [[nodiscard]] bool isLocked(IWorld& world, const BlockPos& pos, const BlockState& state) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        return false;
    }

    // ========== 比较器特有方法 ==========

    /**
     * @brief 获取比较器模式
     *
     * @param state 方块状态
     * @return ComparatorMode 当前模式
     */
    [[nodiscard]] static ComparatorMode getMode(const BlockState& state);

    /**
     * @brief 设置比较器模式
     *
     * @param state 方块状态
     * @param mode 模式
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withMode(BlockState state, ComparatorMode mode);

    /**
     * @brief 检查是否为减法模式
     *
     * @param state 方块状态
     * @return true 如果是减法模式
     */
    [[nodiscard]] static bool isSubtractMode(const BlockState& state);

    /**
     * @brief 获取存储的输出信号强度
     *
     * 从 BlockEntity 读取输出信号。如果 BlockEntity 不存在，返回 0。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return i32 存储的输出信号强度 0-15
     */
    [[nodiscard]] i32 getStoredOutputSignal(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 存储输出信号强度
     *
     * 将输出信号存储到 BlockEntity 中。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param signal 信号强度 0-15
     */
    void storeOutputSignal(IWorld& world, const BlockPos& pos, i32 signal) const;

    /**
     * @brief 右键交互 - 切换比较/减法模式
     *
     * 右键点击比较器可以在比较模式和减法模式之间切换。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

protected:
    /**
     * @brief 状态更新时触发
     *
     * 重写以在状态变化时更新 BlockEntity 中的输出信号。
     */
    void onStateChanged(IWorld& world, const BlockPos& pos, const BlockState& oldState, const BlockState& newState);

private:
    /// 比较器延迟（固定2 tick）
    static constexpr i32 COMPARATOR_DELAY = 2;

    /**
     * @brief 计算输入信号强度（包括容器信号检测）
     *
     * 不仅检测红石信号，还检测容器信号和物品展示框信号。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return i32 输入信号强度 0-15
     */
    [[nodiscard]] i32 _calculateInputStrength(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 计算输出信号强度
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param mainInput 主输入信号（用于比较）
     * @param sideInput 侧面输入信号（用于减法模式）
     * @return i32 计算后的输出信号强度
     */
    [[nodiscard]] i32 _calculateOutput(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 查找物品展示框
     *
     * 在指定位置查找朝向特定方向的物品展示框。
     * 物品展示框必须附着在方块的表面上，且朝向必须与比较器朝向相同。
     *
     * @param world 世界引用
     * @param facing 比较器的朝向（也是物品展示框应有的朝向）
     * @param pos 要搜索的方块位置
     * @return ItemFrameEntity* 找到的物品展示框，如果没有或多个则返回 nullptr
     */
    [[nodiscard]] static entity::ItemFrameEntity* _findItemFrame(IWorld& world, Direction facing, const BlockPos& pos);
};

} // namespace blocks
} // namespace mc
