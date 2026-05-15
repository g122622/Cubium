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

#include "AbstractChestedHorseEntity.hpp"

#include <memory>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 羊驼实体
 *
 * 对齐 1.16.5 `LlamaEntity` 的基础层次。当前先把箱子马类层抽出来，
 * 并保留强度、地毯颜色、商队和吐口水相关状态。
 */
class LlamaEntity : public AbstractChestedHorseEntity {
public:
    /**
     * @brief 羊驼颜色
     */
    enum class LlamaColor : u8 { Creamy = 0, White = 1, Brown = 2, Gray = 3 };

    /**
     * @brief 构造羊驼实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    LlamaEntity(LegacyEntityType type, EntityId id);
    ~LlamaEntity() override = default;

    LlamaEntity(const LlamaEntity&) = delete;
    LlamaEntity& operator=(const LlamaEntity&) = delete;
    LlamaEntity(LlamaEntity&&) = default;
    LlamaEntity& operator=(LlamaEntity&&) = default;

    /**
     * @brief 创建羊驼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 获取羊驼颜色
     */
    [[nodiscard]] LlamaColor getColor() const { return m_color; }

    /**
     * @brief 设置羊驼颜色
     */
    void setColor(LlamaColor color) { m_color = color; }

    /**
     * @brief 随机设置颜色和强度
     */
    void randomizeAppearance();

    /**
     * @brief 羊驼可被骑乘但不能控制方向
     */
    [[nodiscard]] bool canBeRiddenBy(Player* player) const;

    /**
     * @brief 羊驼不能装备鞍
     *
     * MC 1.16.5: LlamaEntity.func_230264_L_() 返回 false
     */
    [[nodiscard]] bool canEquipSaddle() const override { return false; }

    /**
     * @brief 羊驼支持装饰槽位（地毯）
     *
     * MC 1.16.5: LlamaEntity.func_230276_fq_() 返回 true
     */
    [[nodiscard]] bool hasArmorSlot() const override { return true; }

    /**
     * @brief 检查物品是否是有效的装饰（地毯）
     *
     * MC 1.16.5: LlamaEntity.isArmor(ItemStack)
     * 检查物品是否在 ItemTags.CARPETS 中
     *
     * @param item 要检查的物品
     * @return 如果是有效的地毯返回 true
     */
    [[nodiscard]] bool isValidArmorForSlot(const ItemStack& item) const override;

    /**
     * @brief 返回背包列数
     *
     * 对齐 vanilla，等于 strength。
     */
    [[nodiscard]] i32 getInventoryColumns() const override;

    /**
     * @brief 获取强度
     */
    [[nodiscard]] i32 getStrength() const { return m_strength; }

    /**
     * @brief 设置强度
     */
    void setStrength(i32 strength);

    /**
     * @brief 获取地毯颜色，-1 表示没有地毯
     */
    [[nodiscard]] i32 getCarpetColor() const { return m_carpetColor; }

    /**
     * @brief 设置地毯颜色
     */
    void setCarpetColor(i32 color) { m_carpetColor = color; }

    /**
     * @brief 当前是否处于商队中
     */
    [[nodiscard]] bool isInCaravan() const { return m_inCaravan; }

    /**
     * @brief 设置商队状态
     */
    void setInCaravan(bool inCaravan) { m_inCaravan = inCaravan; }

    /**
     * @brief 获取商队领头羊驼
     */
    [[nodiscard]] LlamaEntity* getCaravanLeader() const { return m_caravanLeader; }

    /**
     * @brief 设置商队领头羊驼
     */
    void setCaravanLeader(LlamaEntity* leader) { m_caravanLeader = leader; }

    /**
     * @brief 当前是否正在吐口水
     */
    [[nodiscard]] bool isSpitting() const { return m_spitting; }

    /**
     * @brief 设置吐口水状态
     */
    void setSpitting(bool spitting) { m_spitting = spitting; }

    /**
     * @brief 羊驼最大驯服进度为 30
     *
     * MC 1.16.5: LlamaEntity.getMaxTemper() 返回 30
     */
    [[nodiscard]] i32 getMaxTemper() const { return 30; }

    /**
     * @brief 羊驼使用干草块繁殖
     *
     * MC 1.16.5: LlamaEntity.isBreedingItem()
     * 检查是否为小麦或干草块。
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 羊驼不依赖专门驯服食物
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查是否可以与另一动物交配
     *
     * MC 1.16.5: LlamaEntity.canMateWith()
     * 羊驼只能与羊驼交配。
     */
    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    /**
     * @brief 生成幼体
     *
     * MC 1.16.5: LlamaEntity.func_241840_a()
     * 羊驼后代遗传强度和颜色。
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 处理喂食
     *
     * MC 1.16.5: LlamaEntity.handleEating()
     * 羊驼的食物效果与马不同：
     * - 小麦：治疗 2，成长 10 ticks，驯服 +3
     * - 干草块：治疗 10，成长 90 ticks，驯服 +6，可触发繁殖
     */
    bool handleEating(Player* player, ItemStack& itemStack) override;

    /**
     * @brief 检查物品是否为羊驼的食物
     *
     * MC 1.16.5: LlamaEntity.field_234243_bC_
     * 羊驼食物：小麦、干草块
     */
    [[nodiscard]] bool isFoodItem(const ItemStack& itemStack) const override;

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.77f; }

    /**
     * @brief 获取进食音效
     *
     * MC 1.16.5: 羊驼进食音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getEatSound() const override;

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    LlamaColor m_color = LlamaColor::Creamy;
    i32 m_strength = 1;
    i32 m_carpetColor = -1;
    bool m_inCaravan = false;
    LlamaEntity* m_caravanLeader = nullptr;
    bool m_spitting = false;
    i32 m_spitCooldown = 0;
};

} // namespace mc
