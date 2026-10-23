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

#include "common/core/Result.hpp"
#include "common/mod/bedrock/addon/content/base/properties/AddonDynamicProperties.hpp"

namespace mc {
class ItemStack;
}

namespace mc::mod::bedrock::addon {

/** @brief 通过物品已有 custom_data 组件保存 addon 属性，保持拷贝、容器和存档的值语义。 */
class ItemAddonProperties {
public:
    /** @brief 校验物品属性快照；损坏数据返回失败，不能静默覆盖。 */
    [[nodiscard]] static Result<AddonDynamicProperties> read(const mc::ItemStack& item);
    /** @brief 将完整属性快照写入非堆叠物品，保留其他自定义标签。 */
    static Result<void> write(mc::ItemStack& item, const AddonDynamicProperties& properties);
};

} // namespace mc::mod::bedrock::addon
