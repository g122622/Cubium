#pragma once

#include "AnimalEntity.hpp"
#include "common/entity/interfaces/IShearable.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include <vector>
#include <optional>

namespace mc {

// 前向声明
class ItemStack;
class Player;
class DamageSource;
class Block;

/**
 * @brief 羊毛颜色枚举
 *
 * 对应 MC 1.16.5 DyeColor
 */
enum class DyeColor : u8 {
    White = 0,
    Orange = 1,
    Magenta = 2,
    LightBlue = 3,
    Yellow = 4,
    Lime = 5,
    Pink = 6,
    Gray = 7,
    LightGray = 8,
    Cyan = 9,
    Purple = 10,
    Blue = 11,
    Brown = 12,
    Green = 13,
    Red = 14,
    Black = 15,
    Count = 16
};

/**
 * @brief 羊实体
 *
 * 可剪羊毛的被动动物，用小麦繁殖。
 * 实现 IShearable 接口以支持剪羊毛功能。
 *
 * 参考 MC 1.16.5 SheepEntity
 */
class SheepEntity : public AnimalEntity, public entity::IShearable {
public:
    SheepEntity(LegacyEntityType type, EntityId id);
    ~SheepEntity() override = default;

    /**
     * @brief 实体工厂方法
     *
     * 用于 EntityRegistry 注册
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     * 参考 MC 1.16.5 SheepEntity.getAmbientSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     * 参考 MC 1.16.5 SheepEntity.getHurtSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     * 参考 MC 1.16.5 SheepEntity.getDeathSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    // ========== 羊毛颜色 ==========

    /**
     * @brief 获取羊毛颜色
     * @return 羊毛颜色
     */
    [[nodiscard]] DyeColor getFleeceColor() const { return m_fleeceColor; }

    /**
     * @brief 设置羊毛颜色
     * 参考 MC 1.16.5 SheepEntity.setFleeceColor()
     */
    void setFleeceColor(DyeColor color) { m_fleeceColor = color; }

    /**
     * @brief 是否被剪过（没有羊毛）
     * 参考 MC 1.16.5 SheepEntity.getSheared()
     */
    [[nodiscard]] bool isSheared() const { return m_sheared; }

    /**
     * @brief 设置剪毛状态
     * 参考 MC 1.16.5 SheepEntity.setSheared()
     */
    void setSheared(bool sheared) { m_sheared = sheared; }

    // ========== IShearable 接口实现 ==========

    /**
     * @brief 是否可以被剪毛
     * 参考 MC 1.16.5 SheepEntity.isShearable()
     */
    [[nodiscard]] bool isShearable() const override;

    /**
     * @brief 剪毛
     * 参考 MC 1.16.5 SheepEntity.shear()
     */
    std::vector<ItemStack> shear(Player* player = nullptr) override;

    [[nodiscard]] i32 getShearCooldown() const override { return m_shearCooldown; }

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 吃草 ==========

    /**
     * @brief 吃草奖励
     * 参考 MC 1.16.5 SheepEntity.eatGrassBonus()
     *
     * 当羊吃草时调用：
     * - 如果被剪过，重新长出羊毛
     * - 如果是幼羊，加速成长60 ticks
     */
    void eatGrassBonus();

    // ========== 颜色混合 ==========

    /**
     * @brief 从父母颜色获取混合后的幼羊颜色
     * 参考 MC 1.16.5 SheepEntity.getDyeColorMixFromParents()
     *
     * @param parent1Color 父母1的颜色
     * @param parent2Color 父母2的颜色
     * @param random 随机数生成器
     * @return 混合后的颜色（如果没有配方则随机选择父母颜色）
     */
    [[nodiscard]] static DyeColor getDyeColorMixFromParents(
        DyeColor parent1Color, DyeColor parent2Color, math::Random& random);

    /**
     * @brief 吃草动画计时器
     */
    [[nodiscard]] i32 getEatAnimationTimer() const { return m_eatAnimationTimer; }

    /**
     * @brief 设置吃草动画计时器
     */
    void setEatAnimationTimer(i32 timer) { m_eatAnimationTimer = timer; }

    // ========== 静态工具方法 ==========

    /**
     * @brief 获取随机羊毛颜色
     * 参考 MC 1.16.5 SheepEntity.getRandomSheepColor()
     *
     * 概率分布：
     * - 5% 黑色
     * - 5% 灰色
     * - 5% 浅灰色
     * - 3% 棕色
     * - 0.2% 粉色
     * - 81.8% 白色
     */
    [[nodiscard]] static DyeColor getRandomSheepColor(math::Random& random);

    /**
     * @brief 根据染料颜色获取对应的羊毛方块
     * @param color 染料颜色
     * @return 对应的羊毛方块指针，无效颜色返回 nullptr
     */
    [[nodiscard]] static const Block* getWoolBlockByColor(DyeColor color);

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.9f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 1.3f; }

    /**
     * @brief 获取站立时眼睛高度
     * 参考 MC 1.16.5 SheepEntity.getStandingEyeHeight()
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.95f * height(); }

private:
    DyeColor m_fleeceColor = DyeColor::White;  // 羊毛颜色
    bool m_sheared = false;                     // 是否被剪过
    i32 m_eatAnimationTimer = 0;                // 吃草动画计时器
    i32 m_shearCooldown = 0;                    // 剪毛冷却（ticks）

    static constexpr i32 EAT_GRASS_TIMER_MAX = 40;  // 吃草动画持续时间
};

} // namespace mc
