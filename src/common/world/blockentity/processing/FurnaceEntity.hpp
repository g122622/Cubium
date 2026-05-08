#pragma once

#include "world/blockentity/processing/AbstractFurnaceEntity.hpp"

namespace mc {
namespace blockentity {

/**
 * @brief 普通熔炉方块实体
 *
 * 实现200tick熔炼时间的熔炉。
 * 可以熔炼所有类型的物品（矿石、食物、材料等）。
 *
 * 参考: net.minecraft.tileentity.FurnaceTileEntity
 */
class FurnaceEntity : public AbstractFurnaceEntity {
public:
    /// 默认熔炼时间（tick）
    static constexpr i32 DEFAULT_COOK_TIME = 200;

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit FurnaceEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~FurnaceEntity() override = default;

    /**
     * @brief 创建方块实体副本
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    /**
     * @brief 获取默认显示名称
     */
    [[nodiscard]] std::string getDefaultName() const override { return "container.furnace"; }

    /**
     * @brief 获取默认熔炼时间
     */
    [[nodiscard]] i32 getDefaultCookTime() const override { return DEFAULT_COOK_TIME; }

    /**
     * @brief 获取熔炼配方类型
     */
    [[nodiscard]] crafting::RecipeType getRecipeType() const override {
        return crafting::RecipeType::Smelting;
    }

    /**
     * @brief 获取火苗噼啪声
     */
    [[nodiscard]] const ResourceLocation& getFireCrackleSound() const override;
};

} // namespace blockentity
} // namespace mc
