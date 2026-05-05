#pragma once

#include "world/blockentity/processing/AbstractFurnaceEntity.hpp"

namespace mc {
namespace blockentity {

/**
 * @brief 烟熏炉方块实体
 *
 * 实现100tick熔炼时间的熔炉。
 * 只能烹饪食物。
 *
 * 参考: net.minecraft.tileentity.SmokerTileEntity
 */
class SmokerEntity : public AbstractFurnaceEntity {
public:
    /// 默认熔炼时间（tick），是普通熔炉的一半
    static constexpr i32 DEFAULT_COOK_TIME = 100;

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit SmokerEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~SmokerEntity() override = default;

    /**
     * @brief 创建方块实体副本
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    /**
     * @brief 获取默认显示名称
     */
    [[nodiscard]] String getDefaultName() const override { return "container.smoker"; }

    /**
     * @brief 获取默认熔炼时间
     */
    [[nodiscard]] i32 getDefaultCookTime() const override { return DEFAULT_COOK_TIME; }

    /**
     * @brief 获取熔炼配方类型
     */
    [[nodiscard]] crafting::RecipeType getRecipeType() const override {
        return crafting::RecipeType::Smoking;
    }

    /**
     * @brief 检查是否可以熔炼
     *
     * 烟熏炉只能烹饪食物。
     */
    [[nodiscard]] bool canSmelt(IWorld& world) const override;

    /**
     * @brief 获取燃料燃烧时间（重写）
     *
     * MC 1.16.5: 烟熏炉燃烧燃料的速度是普通熔炉的2倍，
     * 即同样燃料只能燃烧一半的时间。
     *
     * @param stack 物品堆
     * @return 燃烧时间（tick），如果不是燃料返回0
     */
    [[nodiscard]] i32 getBurnTimeForFuel(const ItemStack& stack) const override {
        return getBurnTime(stack) / 2;
    }

    /**
     * @brief 获取火苗噼啪声（烟熏炉使用烟雾声）
     */
    [[nodiscard]] const ResourceLocation& getFireCrackleSound() const override;
};

} // namespace blockentity
} // namespace mc
