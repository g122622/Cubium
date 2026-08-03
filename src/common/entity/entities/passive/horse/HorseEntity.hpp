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

#include "AbstractHorseEntity.hpp"
#include "CoatColors.hpp"
#include "CoatTypes.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class DonkeyEntity;
class MuleEntity;
class DamageSource;

/**
 * @brief 马实体
 *
 * 花色与花纹由独立的 `CoatColors` / `CoatTypes` 支撑类型承载。
 */
class HorseEntity : public AbstractHorseEntity {
public:
    /**
     * @brief 构造马实体
     * @param id 实体 ID
     */
    HorseEntity(EntityInstanceId id);
    ~HorseEntity() override = default;

    HorseEntity(const HorseEntity&) = delete;
    HorseEntity& operator=(const HorseEntity&) = delete;
    HorseEntity(HorseEntity&&) = delete;
    HorseEntity& operator=(HorseEntity&&) = delete;

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
     *
     * 使用基类的食物列表，但只有金胡萝卜和金苹果可以触发繁殖（在 handleEating 中处理）。
     * 这里返回 isFoodItem() 以支持 TemptGoal AI 目标。
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查是否可以与另一动物交配
     *
     * 马可以与马或驴交配。
     */
    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    /**
     * @brief 生成幼体
     *
     * 马 + 马 = 马，马 + 驴 = 骡。
     * 后代会遗传父母的属性（速度、跳跃力、生命值）和外观（毛色、花纹）。
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 马只有鞍槽和马铠槽
     */
    [[nodiscard]] i32 getInventorySize() const override { return 2; }

    /**
     * @brief 马支持马铠槽位
     */
    [[nodiscard]] bool hasArmorSlot() const override { return true; }

    /**
     * @brief 检查物品是否是有效的马铠
     *
     * 检查物品是否为 HorseArmorItem 实例。
     *
     * @param item 要检查的物品
     * @return 如果是有效的马铠返回 true
     */
    [[nodiscard]] bool isValidArmorForSlot(const ItemStack& item) const override;

    /**
     * @brief 处理玩家交互
     *
     * 在基类逻辑之前增加食物优先级和未驯服时的愤怒反应：
     * - 手持食物时优先喂食（使用 HORSE_FOOD 标签）
     * - 未驯服时让马愤怒（makeMad）
     * - 其余交给基类处理
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

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
     * 返回马的愤怒音效，扬蹄时播放。
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
};

} // namespace mc
