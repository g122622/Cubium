#pragma once

#include "AbstractHorseEntity.hpp"
#include "CoatColors.hpp"
#include "CoatTypes.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declaration
class DamageSource;

/**
 * @brief 马实体
 *
 * 对齐 1.16.5 `HorseEntity` 的基础外观与骑乘语义。
 * 花色与花纹由独立的 `CoatColors` / `CoatTypes` 支撑类型承载。
 */
class HorseEntity : public AbstractHorseEntity {
public:
    /**
     * @brief 构造马实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    HorseEntity(LegacyEntityType type, EntityId id);
    ~HorseEntity() override = default;

    HorseEntity(const HorseEntity&) = delete;
    HorseEntity& operator=(const HorseEntity&) = delete;
    HorseEntity(HorseEntity&&) = default;
    HorseEntity& operator=(HorseEntity&&) = default;

    /**
     * @brief 创建马实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 获取马的毛色
     */
    [[nodiscard]] CoatColors getColor() const { return m_color; }

    /**
     * @brief 设置马的毛色
     */
    void setColor(CoatColors color) { m_color = color; }

    /**
     * @brief 获取马的花纹
     */
    [[nodiscard]] CoatTypes getMarking() const { return m_marking; }

    /**
     * @brief 设置马的花纹
     */
    void setMarking(CoatTypes marking) { m_marking = marking; }

    /**
     * @brief 获取外观变种编码
     *
     * 低 8 位为 `CoatColors`，高 8 位为 `CoatTypes`。
     */
    [[nodiscard]] i32 getVariant() const;

    /**
     * @brief 通过变种编码设置外观
     */
    void setVariant(i32 variant);

    /**
     * @brief 随机设置外观
     */
    void randomizeAppearance();

    /**
     * @brief 马不依赖专门驯服食物
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const override;

    /**
     * @brief 马使用金苹果或金胡萝卜繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 马只有鞍槽和马铠槽
     */
    [[nodiscard]] i32 getInventorySize() const override { return 2; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.53f; }

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取愤怒音效
     *
     * MC 1.16.5: HorseEntity.getAngrySound()
     * 返回马的愤怒音效，扬蹄时播放
     */
    [[nodiscard]] std::optional<ResourceLocation> getAngrySound() const override;

    /**
     * @brief 播放进食音效
     */
    void playEatSound();

    /**
     * @brief 播放跳跃音效
     */
    void playJumpSound();

    /**
     * @brief 播放愤怒音效（被骑乘时）
     */
    void playAngrySound();

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    CoatColors m_color = CoatColors::White;
    CoatTypes m_marking = CoatTypes::None;
    i32 m_rearingCounter = 0;
    bool m_isRearing = false;
};

} // namespace mc
