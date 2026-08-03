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
#include "common/entity/core/EntityType.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <memory>

namespace mc {
namespace item {

/**
 * @brief 生成蛋物品
 *
 * 右键使用时生成对应实体。
 */
class SpawnEggItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param entityType 要生成的实体类型
     * @param primaryColor 主颜色 (RGBA格式的颜色值)
     * @param secondaryColor 副颜色 (RGBA格式的颜色值)
     * @param properties 物品属性
     */
    SpawnEggItem(entity::EntityType entityType, u32 primaryColor, u32 secondaryColor, const ItemProperties& properties);

    ~SpawnEggItem() override = default;

    /**
     * @brief 获取要生成的实体类型
     */
    [[nodiscard]] const entity::EntityType& getEntityType() const { return m_entityType; }

    /**
     * @brief 获取主颜色
     */
    [[nodiscard]] u32 getPrimaryColor() const { return m_primaryColor; }

    /**
     * @brief 获取副颜色
     */
    [[nodiscard]] u32 getSecondaryColor() const { return m_secondaryColor; }

    /**
     * @brief 方块交互 - 在方块上生成实体
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 右键使用 - 在玩家位置生成实体
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 生成实体
     * @param world 世界
     * @param pos 位置
     * @param spawnReason 生成原因
     * @return 是否成功生成
     */
    bool spawnEntity(IWorld& world, const BlockPos& pos, world::spawn::SpawnReason spawnReason) const;

private:
    entity::EntityType m_entityType;
    u32 m_primaryColor;
    u32 m_secondaryColor;
};

} // namespace item
} // namespace mc
