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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHERWISE DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "../../../interfaces/IRangedAttackMob.hpp"
#include "AbstractChestedHorseEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class Player;
class ItemStack;
class LivingEntity;

/**
 * @brief 羊驼实体
 *
 * 羊驼实体实现，包含强度、地毯颜色、商队系统和吐口水相关状态。
 *
 * 商队系统：
 * - 羊驼可以形成商队链表结构（最多 8 只）
 * - 链表头是被拴绳拴住的羊驼
 * - 后续羊驼跟随前一只羊驼
 *
 * 远程攻击：
 * - 羊驼可以吐口水攻击目标
 * - 口水造成 1 点伤害（0.5 颗心）
 * - 攻击间隔 40 ticks
 */
class LlamaEntity : public AbstractChestedHorseEntity, public entity::IRangedAttackMob {
public:
    /**
     * @brief 羊驼颜色
     */
    enum class LlamaColor : u8 { Creamy = 0, White = 1, Brown = 2, Gray = 3 };

    /**
     * @brief 构造羊驼实体
     * @param id 实体 ID
     */
    LlamaEntity(EntityInstanceId id);
    ~LlamaEntity() override = default;

    LlamaEntity(const LlamaEntity&) = delete;
    LlamaEntity& operator=(const LlamaEntity&) = delete;
    LlamaEntity(LlamaEntity&&) = delete;
    LlamaEntity& operator=(LlamaEntity&&) = delete;

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
     */
    [[nodiscard]] bool canEquipSaddle() const override { return false; }

    /**
     * @brief 羊驼不能扬蹄
     *
     * MC 1.21.11: Llama.canPerformRearing() 返回 false
     */
    [[nodiscard]] bool canPerformRearing() const override { return false; }

    /**
     * @brief 羊驼支持装饰槽位（地毯）
     */
    [[nodiscard]] bool hasArmorSlot() const override { return true; }

    /**
     * @brief 检查物品是否是有效的装饰（地毯）
     *
     * 检查物品是否在 ItemTags.CARPETS 中
     *
     * @param item 要检查的物品
     * @return 如果是有效的地毯返回 true
     */
    [[nodiscard]] bool isValidArmorForSlot(const ItemStack& item) const override;

    /**
     * @brief 返回背包列数
     *
     * 等于 strength。
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

    // ========== 商队系统 ==========

    /**
     * @brief 检查是否在商队中（有商队头领）
     *
     * @return 如果有商队头领返回 true
     */
    [[nodiscard]] bool isInCaravan() const { return m_caravanHead != nullptr; }

    /**
     * @brief 检查是否有商队跟随者
     *
     * @return 如果有跟随的羊驼返回 true
     */
    [[nodiscard]] bool hasCaravanTail() const { return m_caravanTail != nullptr; }

    /**
     * @brief 获取商队头领
     *
     * @return 商队头领指针，如果没有返回 nullptr
     */
    [[nodiscard]] LlamaEntity* getCaravanHead() const { return m_caravanHead; }

    /**
     * @brief 获取商队跟随者
     *
     * @return 跟随者的指针，如果没有返回 nullptr
     */
    [[nodiscard]] LlamaEntity* getCaravanTail() const { return m_caravanTail; }

    /**
     * @brief 加入商队
     *
     * 设置当前羊驼跟随指定的头领羊驼
     *
     * @param head 要跟随的商队头领
     */
    void joinCaravan(LlamaEntity* head);

    /**
     * @brief 离开商队
     *
     * 清除商队关系，同时更新前后羊驼的引用
     */
    void leaveCaravan();

    // ========== 远程攻击系统 ==========

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
     * @return 30
     */
    [[nodiscard]] i32 getMaxTemper() const noexcept { return 30; }

    /**
     * @brief 羊驼使用干草块繁殖
     *
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
     * 羊驼只能与羊驼交配。
     */
    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    /**
     * @brief 生成幼体
     *
     * 羊驼后代遗传强度和颜色。
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 处理喂食
     *
     * 羊驼的食物效果与马不同：
     * - 小麦：治疗 2，成长 10 ticks，驯服 +3
     * - 干草块：治疗 10，成长 90 ticks，驯服 +6，可触发繁殖
     */
    bool handleEating(Player* player, ItemStack& itemStack) override;

    /**
     * @brief 检查物品是否为羊驼的食物
     *
     * 羊驼食物：小麦、干草块
     */
    [[nodiscard]] bool isFoodItem(const ItemStack& itemStack) const override;

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const noexcept override { return 1.77f; }

    /**
     * @brief 获取箱子装备音效
     *
     * 羊驼使用专属音效 ENTITY_LLAMA_CHEST
     */
    [[nodiscard]] const ResourceLocation& getChestEquipSound() const override;

    /**
     * @brief 获取进食音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getEatSound() const override;

    /**
     * @brief 获取愤怒音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAngrySound() const override;

    // ========== IRangedAttackMob 接口实现 ==========

    /**
     * @brief 对目标进行远程攻击
     *
     * 发射羊驼口水攻击目标
     *
     * @param target 攻击目标
     * @param charge 蓄力程度（羊驼忽略此参数）
     */
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    /**
     * @brief 获取攻击间隔时间
     * @return 40 ticks
     */
    [[nodiscard]] i32 getAttackInterval() const noexcept override { return 40; }

    /**
     * @brief 检查是否可以进行远程攻击
     * @return 吐口水冷却已结束返回 true
     */
    [[nodiscard]] bool canRangedAttack() const override { return m_spitCooldown <= 0; }

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    /**
     * @brief 吐口水攻击目标
     *
     * 创建并发射羊驼口水实体
     *
     * @param target 攻击目标
     */
    void _spit(LivingEntity* target);

    LlamaColor m_color = LlamaColor::Creamy;
    i32 m_strength = 1;
    i32 m_carpetColor = -1;

    // 商队系统 - 双向链表
    LlamaEntity* m_caravanHead = nullptr; ///< 跟随的商队头领
    LlamaEntity* m_caravanTail = nullptr; ///< 跟随自己的羊驼

    // 远程攻击
    bool m_spitting = false;
    i32 m_spitCooldown = 0;
};

} // namespace mc
