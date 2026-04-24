#pragma once

#include "ParticleData.hpp"
#include "common/item/core/ItemStack.hpp"

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 物品粒子数据
 *
 * 用于需要物品信息的粒子类型，如物品破坏、物品使用等。
 * 参考 MC 1.16.5 ItemParticleData
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
    [[nodiscard]] String getTypeName() const override;
    [[nodiscard]] String getParameters() const override;
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
