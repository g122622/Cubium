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

#include "ParticleData.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/item/core/ItemStack.hpp"
#include <memory>
#include <string>

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 物品粒子数据
 *
 * 用于需要物品信息的粒子类型，如物品破坏、物品使用等。
 *
 * 用法示例：
 * @code
 * ItemStack stack(ItemId::Diamond, 1);
 * auto itemData = std::make_unique<ItemParticleData>(ParticleTypeId::Item, stack);
 * @endcode
 */
class ItemParticleData : public ParticleData {
public:
    /**
     * @brief 构造物品粒子数据
     *
     * @param type 粒子类型 ID（必须是 Item 类型）
     * @param itemStack 物品堆
     */
    ItemParticleData(ParticleTypeId type, const ItemStack& itemStack);

    ~ItemParticleData() override = default;

    // 允许拷贝
    ItemParticleData(const ItemParticleData&) = default;
    ItemParticleData& operator=(const ItemParticleData&) = default;

    // 允许移动
    ItemParticleData(ItemParticleData&&) noexcept = default;
    ItemParticleData& operator=(ItemParticleData&&) noexcept = default;

    // ========================================================================
    // ParticleData 接口实现
    // ========================================================================

    [[nodiscard]] ParticleTypeId getType() const override { return m_type; }
    [[nodiscard]] std::string getTypeName() const override;
    [[nodiscard]] std::string getParameters() const override;
    [[nodiscard]] std::unique_ptr<ParticleData> clone() const override;

    // ========================================================================
    // 物品特有方法
    // ========================================================================

    /**
     * @brief 获取物品堆
     *
     * @return 物品堆
     */
    [[nodiscard]] const ItemStack& getItemStack() const { return m_itemStack; }

private:
    ParticleTypeId m_type;
    ItemStack m_itemStack;
};

} // namespace mc::client::renderer::trident::particle::data
