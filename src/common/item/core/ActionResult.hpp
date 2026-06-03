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
 * 定义动作执行后的结果类型。
 */
enum class ActionResultType : u8 {
    Success = 0, ///< 成功执行，消耗物品
    Consume = 1, ///< 消耗物品但不执行动作
    Fail = 2,    ///< 执行失败，不消耗物品
    Pass = 3     ///< 传递给下一个处理器
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
