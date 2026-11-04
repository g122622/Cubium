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

#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 掉落条件基类
 *
 * 定义掉落表条件接口。条件用于控制掉落条目是否生效。
 *
 * 示例用法:
 * @code
 * auto chance = std::make_unique<RandomChanceCondition>(0.5f);
 * entry->addCondition(std::move(chance));
 * @endcode
 */
class LootCondition {
public:
    virtual ~LootCondition() = default;

    /**
     * @brief 测试条件是否满足
     *
     * @param context 掉落上下文
     * @return 如果条件满足返回true
     */
    [[nodiscard]] virtual bool test(LootContext& context) const = 0;

    /**
     * @brief 创建条件副本
     */
    [[nodiscard]] virtual std::unique_ptr<LootCondition> clone() const noexcept = 0;

    /**
     * @brief 获取条件类型标识
     */
    [[nodiscard]] virtual std::string getType() const = 0;

    /**
     * @brief 复活引用了物品标签的成员
     *
     * 战利品表在物品标签表建立之前解析，解析期无法展开 `items` 里的 `#tag`；而启动
     * 流程还会清空并重建物品标签，早期已展开成具体物品 ID 的成员随之失效。标签表
     * 就绪后调用本方法，让持有物品谓词的条件重新展开一次。
     *
     * 默认什么也不做：只有直接或间接持有物品谓词的条件需要覆写。
     */
    virtual void expandTagItems() {}
};

} // namespace loot
} // namespace mc
