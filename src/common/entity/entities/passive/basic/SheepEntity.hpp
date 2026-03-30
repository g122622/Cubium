#pragma once

#include "AnimalEntity.hpp"
#include "common/entity/interfaces/IShearable.hpp"
#include "../../../../core/Types.hpp"
#include <vector>

namespace mc {

// 前向声明
class ItemStack;
class Player;

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

    // ========== 羊毛颜色 ==========

    /**
     * @brief 获取羊毛颜色
     * @return 羊毛颜色ID（0=白色，其他见 DyeColor）
     */
    [[nodiscard]] u8 getWoolColor() const { return m_woolColor; }

    /**
     * @brief 设置羊毛颜色
     */
    void setWoolColor(u8 color) { m_woolColor = color; }

    /**
     * @brief 是否有羊毛
     */
    [[nodiscard]] bool hasWool() const { return m_hasWool; }

    /**
     * @brief 设置羊毛状态
     */
    void setWool(bool hasWool) { m_hasWool = hasWool; }

    // ========== IShearable 接口实现 ==========

    [[nodiscard]] bool isShearable() const override { return m_hasWool; }

    std::vector<ItemStack> shear(Player* player = nullptr) override;

    [[nodiscard]] i32 getShearCooldown() const override { return m_shearCooldown; }

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 吃草 ==========

    /**
     * @brief 吃草动画计时器
     */
    [[nodiscard]] i32 getEatAnimationTimer() const { return m_eatAnimationTimer; }

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.9f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 1.3f; }

private:
    u8 m_woolColor = 0;         // 羊毛颜色（默认白色）
    bool m_hasWool = true;       // 是否有羊毛
    i32 m_eatAnimationTimer = 0; // 吃草动画计时器
    i32 m_shearCooldown = 0;     // 剪毛冷却（ticks）

    static constexpr i32 WOOL_REGROW_TIME = 2400; // 羊毛重新生长时间（2分钟 = 2400 ticks）
};

} // namespace mc
