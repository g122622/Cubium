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
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include <memory>
#include <string>

namespace mc {
namespace blockentity {

/**
 * @brief 高炉方块实体
 *
 * 实现100tick熔炼时间的熔炉。
 * 只能熔炼矿石和金属物品。
 */
class BlastFurnaceEntity : public AbstractFurnaceEntity {
public:
    /// 默认熔炼时间（tick），是普通熔炉的一半
    static constexpr i32 DEFAULT_COOK_TIME = 100;

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit BlastFurnaceEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~BlastFurnaceEntity() override = default;

    /**
     * @brief 创建方块实体副本
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    /**
     * @brief 获取默认显示名称
     */
    [[nodiscard]] std::string getDefaultName() const override { return "container.blast_furnace"; }

    /**
     * @brief 获取默认熔炼时间
     */
    [[nodiscard]] i32 getDefaultCookTime() const override { return DEFAULT_COOK_TIME; }

    /**
     * @brief 获取熔炼配方类型
     */
    [[nodiscard]] crafting::RecipeType getRecipeType() const override { return crafting::RecipeType::Blasting; }

    /**
     * @brief 检查是否可以熔炼
     *
     * 高炉只能熔炼矿石和金属物品。
     */
    [[nodiscard]] bool canSmelt(IWorld& world) const override;

    /**
     * @brief 获取燃料燃烧时间（重写）
     *
     * 高炉燃烧燃料的速度是普通熔炉的2倍，即同样燃料只能燃烧一半的时间。
     *
     * @param stack 物品堆
     * @return 燃烧时间（tick），如果不是燃料返回0
     */
    [[nodiscard]] i32 getBurnTimeForFuel(const ItemStack& stack) const override { return getBurnTime(stack) / 2; }

    /**
     * @brief 获取火苗噼啪声
     */
    [[nodiscard]] const ResourceLocation& getFireCrackleSound() const override;
};

} // namespace blockentity
} // namespace mc
