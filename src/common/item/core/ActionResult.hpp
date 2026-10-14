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

namespace mc {

// Forward declarations
class ItemStack;

/**
 * @brief 动作结果类型
 *
 * 定义动作执行后的结果类型。取值语义来自 vanilla `InteractionResult`：
 *
 * | Cubium                | vanilla 等价                              | consumesAction | wasItemInteraction |
 * 服务端挥臂 |
 * |-----------------------|-------------------------------------------|----------------|--------------------|-----------|
 * | `Success`             | `SUCCESS`（swingSource=CLIENT）            | 是             | 是                 |
 * 否（客户端挥）| | `SuccessServer`       | `SUCCESS_SERVER`（swingSource=SERVER）     | 是             | 是 | 是 | |
 * `SuccessServerWithoutItem` | `SUCCESS_SERVER.withoutItem()`        | 是             | 否                 | 是 | |
 * `Consume`             | `CONSUME`（swingSource=NONE）             | 是             | 是                 | 否        |
 * | `TryEmptyHandInteraction` | `TRY_WITH_EMPTY_HAND`                  | 否             | —                  | 否 | |
 * `Fail`                | `FAIL`                                    | 否             | —                  | 否        |
 * | `Pass`                | `PASS`                                    | 否             | —                  | 否 |
 *
 * 关键区分：
 * - `Success` 与 `SuccessServer` 都表示"已消费动作且涉及物品"，但前者由客户端负责播放挥臂
 *   动画（客户端本地预测命中），后者要求服务端主动广播挥臂（客户端未预测到，如放置失败后
 *   回退、或纯服务端判定）。错误使用会导致挥臂动画缺失或重复。
 * - `SuccessServerWithoutItem` 表示"已消费动作但与手持物品无关"（如空手吃蛋糕、空手开箱），
 *   使 `ITEM_USED` 统计不触发。
 * - `TryEmptyHandInteraction` 是"携带物品时无法处理，请以空手语义重试"的信号，服务端据此
 *   转调 `Block::useWithoutItem`。它本身不消费动作。
 */
enum class ActionResultType : u8 {
    Success = 0,                 ///< 成功执行，物品已参与交互，客户端挥臂
    Consume = 1,                 ///< 执行成功但不需要挥臂（swingSource=NONE）
    Fail = 2,                    ///< 执行失败，不消耗物品
    Pass = 3,                    ///< 传递给下一个处理器
    TryEmptyHandInteraction = 4, ///< 请求以空手语义重试（服务端转调 useWithoutItem）
    SuccessServer = 5,           ///< 成功执行，物品已参与交互，服务端负责挥臂
    SuccessServerWithoutItem = 6 ///< 成功执行，与手持物品无关，服务端负责挥臂
};

/**
 * @brief 动作结果
 *
 * 包含动作结果类型和结果物品堆。
 *
 * @tparam T 结果值类型
 */
template <typename T>
class ActionResult {
public:
    /**
     * @brief 构造动作结果
     * @param type 结果类型
     * @param result 结果值
     */
    ActionResult(ActionResultType type, T result)
        : m_type(type)
        , m_result(std::move(result))
    {}

    /**
     * @brief 创建成功结果
     * @param result 结果值
     */
    [[nodiscard]] static ActionResult success(T result)
    {
        return ActionResult(ActionResultType::Success, std::move(result));
    }

    /**
     * @brief 创建消耗结果
     * @param result 结果值
     */
    [[nodiscard]] static ActionResult consume(T result)
    {
        return ActionResult(ActionResultType::Consume, std::move(result));
    }

    /**
     * @brief 创建失败结果
     * @param result 结果值
     */
    [[nodiscard]] static ActionResult fail(T result) { return ActionResult(ActionResultType::Fail, std::move(result)); }

    /**
     * @brief 创建传递结果
     * @param result 结果值
     */
    [[nodiscard]] static ActionResult pass(T result) { return ActionResult(ActionResultType::Pass, std::move(result)); }

    /**
     * @brief 获取结果类型
     */
    [[nodiscard]] ActionResultType getType() const noexcept { return m_type; }

    /**
     * @brief 获取结果值
     */
    [[nodiscard]] const T& getResult() const noexcept { return m_result; }

    /**
     * @brief 获取结果值（可修改）
     */
    [[nodiscard]] T& getResult() noexcept { return m_result; }

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
     * @brief 是否消费了动作（对应 vanilla `InteractionResult.consumesAction()`）
     *
     * `Success` / `Consume` / `SuccessServer` / `SuccessServerWithoutItem` 均消费动作；
     * `TryEmptyHandInteraction` / `Fail` / `Pass` 不消费。
     */
    [[nodiscard]] bool consumesAction() const noexcept
    {
        return m_type == ActionResultType::Success || m_type == ActionResultType::Consume ||
            m_type == ActionResultType::SuccessServer || m_type == ActionResultType::SuccessServerWithoutItem;
    }

    /**
     * @brief 该结果是否源自物品交互（对应 vanilla `InteractionResult.Success.wasItemInteraction()`）
     *
     * `SuccessServerWithoutItem`（vanilla `SUCCESS_SERVER.withoutItem()`）为 false，
     * 用于抑制 `ITEM_USED` 统计。
     */
    [[nodiscard]] bool wasItemInteraction() const noexcept
    {
        return m_type != ActionResultType::SuccessServerWithoutItem;
    }

    /**
     * @brief 是否需要服务端主动广播挥臂动画（对应 vanilla `swingSource() == SERVER`）
     */
    [[nodiscard]] bool needsServerSwing() const noexcept
    {
        return m_type == ActionResultType::SuccessServer || m_type == ActionResultType::SuccessServerWithoutItem;
    }

    /**
     * @brief 是否请求以空手语义重试（对应 vanilla `TRY_WITH_EMPTY_HAND`）
     */
    [[nodiscard]] bool isTryEmptyHandInteraction() const noexcept
    {
        return m_type == ActionResultType::TryEmptyHandInteraction;
    }

private:
    ActionResultType m_type;
    T m_result;
};

/**
 * @brief 物品动作结果
 *
 * 特化版本，结果值为ItemStack。
 */
using ItemActionResult = ActionResult<ItemStack>;

} // namespace mc
