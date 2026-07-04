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

#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"

#include <optional>
#include <utility>

namespace mc {

/**
 * @brief 方块交互结果
 *
 * 在 `ActionResultType` 的基础上增加 `heldItemTransformedTo` 字段，
 * 用于方块交互后显式传递玩家手持物品的转换结果。
 *
 * 设计参考 MC 1.21.11 `InteractionResult.Success.heldItemTransformedTo(ItemStack)`：
 * - 当方块交互修改了玩家手持物品时，应通过 `heldItemTransformedTo` 携带转换后的物品堆
 * - 当方块交互未修改手持物品时，`heldItemTransformedTo` 为 `std::nullopt`
 * - 消费方（如 `BlockInteractionManager`）据此决定是否同步物品栏到客户端
 *
 * 为了向后兼容已有的 52 个 `Block::onBlockActivated` override（它们直接返回 `ActionResultType`），
 * 本类提供从 `ActionResultType` 的隐式转换构造函数。这样旧代码无需修改即可继续工作，
 * 只有需要传递 `heldItemTransformedTo` 的方块（如 `ShelfBlock`）才使用新的工厂方法。
 *
 * 参考: net.minecraft.world.InteractionResult (MC 1.21.11)
 */
class BlockActionResult {
public:
    /**
     * @brief 从 `ActionResultType` 隐式构造（向后兼容）
     *
     * 旧代码 `return ActionResultType::Success;` 会通过此构造函数自动包装为
     * `BlockActionResult`，`heldItemTransformedTo` 为 `std::nullopt`。
     */
    BlockActionResult(ActionResultType type) // NOLINT(google-explicit-constructor)
        : m_type(type)
    {}

    /**
     * @brief 构造带转换后物品的方块交互结果
     * @param type 结果类型
     * @param heldItemTransformed 转换后的手持物品堆
     */
    BlockActionResult(ActionResultType type, ItemStack heldItemTransformed)
        : m_type(type)
        , m_heldItemTransformed(std::move(heldItemTransformed))
    {}

    // ========== 工厂方法 ==========

    /**
     * @brief 创建成功结果（不携带物品转换）
     */
    [[nodiscard]] static BlockActionResult success() { return BlockActionResult(ActionResultType::Success); }

    /**
     * @brief 创建成功结果，携带转换后的手持物品
     * @param heldItemTransformed 转换后的手持物品堆
     */
    [[nodiscard]] static BlockActionResult success(ItemStack heldItemTransformed)
    {
        return BlockActionResult(ActionResultType::Success, std::move(heldItemTransformed));
    }

    /**
     * @brief 创建消耗结果（不携带物品转换）
     */
    [[nodiscard]] static BlockActionResult consume() { return BlockActionResult(ActionResultType::Consume); }

    /**
     * @brief 创建消耗结果，携带转换后的手持物品
     * @param heldItemTransformed 转换后的手持物品堆
     */
    [[nodiscard]] static BlockActionResult consume(ItemStack heldItemTransformed)
    {
        return BlockActionResult(ActionResultType::Consume, std::move(heldItemTransformed));
    }

    /**
     * @brief 创建失败结果
     */
    [[nodiscard]] static BlockActionResult fail() { return BlockActionResult(ActionResultType::Fail); }

    /**
     * @brief 创建传递结果
     */
    [[nodiscard]] static BlockActionResult pass() { return BlockActionResult(ActionResultType::Pass); }

    // ========== 链式方法（参考 MC 1.21.11 InteractionResult.Success.heldItemTransformedTo） ==========

    /**
     * @brief 返回一个新的结果，附带转换后的手持物品
     *
     * 参考 MC 1.21.11 `InteractionResult.Success.heldItemTransformedTo(ItemStack)`。
     * 用于链式调用：`BlockActionResult::success().heldItemTransformedTo(stack)`。
     *
     * @param transformed 转换后的手持物品堆
     * @return 新的 BlockActionResult，携带转换后的物品
     */
    [[nodiscard]] BlockActionResult heldItemTransformedTo(ItemStack transformed) const
    {
        return BlockActionResult(m_type, std::move(transformed));
    }

    // ========== 访问器 ==========

    /**
     * @brief 获取结果类型
     */
    [[nodiscard]] ActionResultType getType() const noexcept { return m_type; }

    /**
     * @brief 获取转换后的手持物品（如果存在）
     *
     * 返回 `std::optional` 以区分"未转换"（nullopt）与"转换为空物品"（empty stack）。
     * 参考 MC 1.21.11 `InteractionResult.Success.heldItemTransformedTo()` 返回 `@Nullable ItemStack`。
     *
     * @return 转换后的手持物品堆，如果方块未修改手持物品则返回 `std::nullopt`
     */
    [[nodiscard]] const std::optional<ItemStack>& heldItemTransformedTo() const noexcept
    {
        return m_heldItemTransformed;
    }

    /**
     * @brief 是否成功
     */
    [[nodiscard]] bool isSuccess() const noexcept { return m_type == ActionResultType::Success; }

    /**
     * @brief 是否消耗
     */
    [[nodiscard]] bool isConsume() const noexcept { return m_type == ActionResultType::Consume; }

    /**
     * @brief 是否失败
     */
    [[nodiscard]] bool isFail() const noexcept { return m_type == ActionResultType::Fail; }

    /**
     * @brief 是否传递
     */
    [[nodiscard]] bool isPass() const noexcept { return m_type == ActionResultType::Pass; }

    /**
     * @brief 是否成功或消耗（表示动作已处理）
     */
    [[nodiscard]] bool isSuccessOrConsume() const noexcept
    {
        return m_type == ActionResultType::Success || m_type == ActionResultType::Consume;
    }

    /**
     * @brief 是否携带了物品转换信息
     */
    [[nodiscard]] bool hasHeldItemTransformed() const noexcept { return m_heldItemTransformed.has_value(); }

    /**
     * @brief 与 `ActionResultType` 相等比较
     *
     * 仅比较 `m_type`，不比较 `heldItemTransformed`。
     * 允许测试代码 `EXPECT_EQ(result, ActionResultType::Success)` 等用法。
     */
    [[nodiscard]] friend bool operator==(BlockActionResult lhs, ActionResultType rhs) noexcept
    {
        return lhs.m_type == rhs;
    }

    /**
     * @brief 与 `ActionResultType` 相等比较（反向参数顺序）
     */
    [[nodiscard]] friend bool operator==(ActionResultType lhs, BlockActionResult rhs) noexcept
    {
        return lhs == rhs.m_type;
    }

    /**
     * @brief 与 `ActionResultType` 不等比较
     */
    [[nodiscard]] friend bool operator!=(BlockActionResult lhs, ActionResultType rhs) noexcept
    {
        return lhs.m_type != rhs;
    }

    /**
     * @brief 与 `ActionResultType` 不等比较（反向参数顺序）
     */
    [[nodiscard]] friend bool operator!=(ActionResultType lhs, BlockActionResult rhs) noexcept
    {
        return lhs != rhs.m_type;
    }

private:
    ActionResultType m_type;
    std::optional<ItemStack> m_heldItemTransformed;
};

} // namespace mc
