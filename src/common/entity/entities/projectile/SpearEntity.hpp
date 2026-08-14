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

#include "AbstractArrowEntity.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 长矛投掷实体
 *
 * 长矛是可回收的投掷武器，命中后可被拾取。
 *
 * 与三叉戟的区别:
 * - 长矛不支持忠诚附魔，命中后不会自动返回（需手动拾取）
 * - 长矛不支持激流/引雷附魔
 * - 长矛水中阻力极小（0.99），可水中投掷
 * - 投掷伤害固定 8.0，与三叉戟一致
 */
class SpearEntity : public AbstractArrowEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /**
     * @brief 构造函数
     */
    explicit SpearEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.5f; }
    [[nodiscard]] f32 height() const override { return 0.5f; }

    // ========== 长矛属性 ==========

    /**
     * @brief 获取长矛物品堆
     */
    [[nodiscard]] ItemStack getItemStack() const;

    /**
     * @brief 获取箭矢对应的物品堆（AbstractArrowEntity 接口实现）
     * @return 长矛物品堆副本
     */
    [[nodiscard]] ItemStack getArrowStack() const override;

    /**
     * @brief 设置长矛物品堆
     */
    void setItemStack(const ItemStack& stack);

    /**
     * @brief 获取水中阻力
     *
     * 长矛水中阻力与三叉戟一致（0.99），水中可投掷且不易减速。
     */
    [[nodiscard]] f32 getWaterDrag() const override;

    /**
     * @brief 设置生物射出长矛的基础伤害
     *
     * 长矛不使用弓类附魔（力量/冲击/火焰），公式与三叉戟相同：
     * damage = power * 2.0 + triangle(difficulty * 0.11, 0.57425)
     */
    void setBaseDamageFromMob(f32 power) override;

    /**
     * @brief 玩家拾取长矛
     */
    bool onPlayerPickup(Player& player) override;

    // ========== NBT 序列化 ==========

    /**
     * @brief 序列化长矛实体特有数据到 NBT
     *
     * 持久化长矛物品堆、拾取状态、伤害值、已造成伤害标志等。
     * 参考 MC 1.21.11 AbstractArrow.addAdditionalSaveData() 与 ThrownTrident.addAdditionalSaveData()。
     */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /**
     * @brief 从 NBT 反序列化长矛实体特有数据
     *
     * 恢复长矛物品堆和各项状态。
     * 参考 MC 1.21.11 AbstractArrow.readAdditionalSaveData() 与 ThrownTrident.readAdditionalSaveData()。
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

protected:
    /**
     * @brief 长矛命中实体时的处理
     *
     * 应用固定 8.0 伤害，使用 DamageType::Spear 伤害源。
     */
    void onEntityHit(const RayTraceResult& result) override;

    /**
     * @brief 长矛命中方块时的处理
     *
     * 设置插地方块状态，等待拾取。
     */
    void onBlockHit(const RayTraceResult& result) override;

    // 批次6 子目标2 Step4：m_spearStack 迁入 ecs::ProjectileItemComponent。
};

} // namespace entity
} // namespace mc
