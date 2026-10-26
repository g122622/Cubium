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

#include <memory>
#include <utility>

#include "common/item/core/ItemStack.hpp"

namespace mc {
namespace loot {

/**
 * @brief 接收一个物品堆的回调
 *
 * 对应原版 `Consumer<ItemStack>`，也就是 `LootItemFunction#decorate` 拼出来的那串。
 * 用抽象接口而不是 `std::function`：
 *
 * - 装饰链是「consumer 包 consumer」，而 `std::function` 收另一个 `std::function`
 *   在 MSVC STL + clang-cl 下既无法构造也无法转发（实参被当成 `_Types...` 的值去
 *   匹配 `_Do_call`，候选全部不匹配）；改成抽象接口后只是普通的虚函数调用；
 * - 「投递到实体队列」只需要复制一个 `shared_ptr`，闭包存活期与语义都更清楚。
 */
class Consumer {
public:
    virtual ~Consumer() = default;

    /// 接收一个物品堆（可能是空堆：原版各层都不做过滤）。
    virtual void accept(const ItemStack& stack) const = 0;
};

/**
 * @brief 由可调用对象构造 Consumer
 *
 * @param callable 形如 `void(const ItemStack&)` 的可调用对象
 */
template <class Callable>
[[nodiscard]] std::shared_ptr<const Consumer> makeConsumer(Callable&& callable)
{
    struct Wrapped final : Consumer {
        explicit Wrapped(Callable&& target)
            : m_target(std::forward<Callable>(target))
        {}

        void accept(const ItemStack& stack) const override
        {
            m_target(stack);
        }

        std::decay_t<Callable> m_target;
    };
    return std::make_shared<Wrapped>(std::forward<Callable>(callable));
}

/** @brief 共享的 Consumer 引用（装饰链按值持有） */
using ConsumerPtr = std::shared_ptr<const Consumer>;

} // namespace loot
} // namespace mc
