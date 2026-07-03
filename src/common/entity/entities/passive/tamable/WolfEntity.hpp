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

#include "TameableEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Crackiness.hpp"
#include "common/util/color/DyeColor.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 狼实体
 *
 * 可驯服的动物，驯服后成为狗。
 *
 * 特性：
 * - 驯服：用骨头驯服
 * - 愤怒：被攻击后反击
 * - 跟随主人：驯服后跟随
 * - 坐下/站起：右键切换
 * - 攻击目标：保护主人
 * - 尾巴角度：表示生命值和心情
 * - 颈圈颜色：驯服后可染色
 * - 狼铠：驯服后可装备狼铠，提供伤害吸收
 *
 * 狼铠交互逻辑（MC 1.21.11）：
 * - 装备：主人右键狼 + 手持狼铠 + 未装备 + 非幼年 → 装备狼铠
 * - 修复：主人右键坐下的狼 + 犰狳鳞甲 + 狼铠已受损 → 修复 12.5% 耐久
 * - 染色：主人右键狼 + 手持染料 + 已装备可染色狼铠 → 改变狼铠颜色
 * - 剪切：主人右键狼 + 手持剪刀 + 已装备狼铠 → 剪下狼铠
 * - 伤害吸收：穿戴狼铠时，非绕过护甲的伤害由狼铠吸收，狼不受伤
 * - 裂纹：狼铠受损到不同阈值时播放裂纹音效和粒子
 */
class WolfEntity : public TameableEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    WolfEntity(EntityId id);
    ~WolfEntity() override = default;

    // 禁止拷贝
    WolfEntity(const WolfEntity&) = delete;
    WolfEntity& operator=(const WolfEntity&) = delete;

    // 允许移动
    WolfEntity(WolfEntity&&) = delete;
    WolfEntity& operator=(WolfEntity&&) = delete;

    /**
     * @brief 创建狼实体
     * @param world 世界实例
     * @return 新的狼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 交互 ==========

    /**
     * @brief 玩家与狼交互
     *
     * 交互逻辑（按优先级）：
     * 1. 未驯服 + 骨头 → 尝试驯服（1/3概率）
     * 2. 已驯服 + 食物 + 未满血 → 喂食治疗
     * 3. 已驯服 + 狼铠 + 可装备 + 主人 + 非幼年 → 装备狼铠
     * 4. 已驯服 + 犰狳鳞甲 + 坐下 + 狼铠受损 + 主人 → 修复狼铠
     * 5. 已驯服 + 染料 + 已装备狼铠 + 主人 → 染色狼铠
     * 6. 已驯服 + 染料 + 颈圈 → 改变颈圈颜色
     * 7. 已驯服 + 调用父类（处理繁殖/成长）
     * 8. 已驯服 + 主人 + 父类未处理 → 切换坐下/站起
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    /**
     * @brief 检查物品是否可用于驯服
     * @param itemStack 物品堆
     * @return 如果是骨头返回true
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查物品是否可用于繁殖
     * @param itemStack 物品堆
     * @return 如果是肉类返回true
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查物品是否可用于治疗
     * @param itemStack 物品堆
     * @return 如果是肉类返回true
     */
    [[nodiscard]] bool isFoodItem(const ItemStack& itemStack) const;

    /**
     * @brief 判断此狼是否想要攻击指定目标
     *
     * 狼不会攻击以下目标：
     * - 苦力怕、恶魂、盔甲架
     * - 已驯服的其他驯服动物（包括已驯服的马）
     * - 与自己同主人的其他狼
     * - 主人不能伤害的玩家（PvP保护）
     *
     * @param target 待攻击的目标实体
     * @param owner 此狼的主人（可能为 nullptr）
     * @return true 如果此狼愿意攻击该目标
     */
    [[nodiscard]] bool wantsToAttack(const LivingEntity& target, const LivingEntity* owner) const override;

    // ========== 狼铠 ==========

    /**
     * @brief 检查玩家是否可以剪切实体装备
     *
     * 狼只允许主人剪切狼铠。
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.canShearEquipment()
     *
     * @param player 尝试剪切的玩家
     * @return 如果是主人返回 true
     */
    [[nodiscard]] bool canShearEquipment(const Player& player) const override;

    /**
     * @brief 实际受伤处理（狼铠伤害吸收）
     *
     * 当狼穿戴狼铠且伤害源不绕过护甲时，伤害由狼铠吸收（狼不扣血）。
     * 狼铠受损时检查裂纹等级变化，等级变化时播放裂纹音效和粒子。
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.actuallyHurt()
     *
     * @param source 伤害来源
     * @param amount 伤害量
     */
    void actuallyHurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 盔甲受损回调
     *
     * 狼穿戴狼铠时，将伤害传递给狼铠。
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.hurtArmor()
     *
     * @param source 伤害来源
     * @param amount 伤害量
     */
    void damageArmor(DamageSource& source, f32 amount) override;

    /**
     * @brief 获取受伤声音
     *
     * 穿戴狼铠且伤害由狼铠吸收时，播放狼铠受伤音效而非普通狼受伤音效。
     *
     * @param source 伤害来源
     * @return 声音事件资源位置
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    // ========== 繁殖 ==========

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 刻更新
     */
    void tick() override;

    // ========== 愤怒系统 ==========

    /**
     * @brief 获取尾巴角度
     * @return 尾巴角度（弧度）
     *
     * 尾巴角度基于生命值：
     * - 满血时约40度
     * - 低血量时约-10度
     */
    [[nodiscard]] f32 getTailAngle() const;

    /**
     * @brief 检查是否感兴趣（乞求食物）
     * @return 如果正在乞求食物返回true
     */
    [[nodiscard]] bool isInterested() const { return m_interested; }

    /**
     * @brief 设置感兴趣状态
     * @param interested 是否感兴趣
     */
    void setInterested(bool interested) { m_interested = interested; }

    // ========== 颈圈颜色 ==========

    /**
     * @brief 获取颈圈颜色
     * @return 染料颜色
     */
    [[nodiscard]] DyeColor getCollarColor() const { return m_collarColor; }

    /**
     * @brief 设置颈圈颜色
     * @param color 染料颜色
     */
    void setCollarColor(DyeColor color) { m_collarColor = color; }

    // ========== 水域行为 ==========

    /**
     * @brief 检查是否在水中
     * 狼在水中会减速
     */
    [[nodiscard]] bool isInWater() const override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.4f : 0.8f; }

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.6f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.85f; }

    // ========== 驯服回调 ==========
    void onTamed(bool tamed) override;

    /**
     * @brief 获取环境声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取声音音量
     */
    [[nodiscard]] f32 getSoundVolume() const override { return 0.4f; }

    /**
     * @brief 播放脚步声音
     */
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;
    void playStepSound();

    /**
     * @brief 播放甩水声音
     */
    void playShakingSound();

private:
    // 兴趣状态（乞求食物）
    bool m_interested = false;

    // 颈圈颜色（默认红色，对应 DyeColor::Red = 14）
    DyeColor m_collarColor = DyeColor::Red;

    // 常量
    static constexpr f32 TAIL_ANGLE_HEALTHY = 0.698f;    // 健康时尾巴角度（弧度）
    static constexpr f32 TAIL_ANGLE_UNHEALTHY = -0.175f; // 不健康时尾巴角度（弧度）
    static constexpr f32 ARMOR_REPAIR_UNIT = 0.125F;     // 狼铠修复单位（12.5%最大耐久）

    // 声音状态
    bool m_wasInWater = false;
    f32 m_stepSoundDistance = 0.0f;
    f32 m_nextStepSoundDistance = 1.0f;

    // ========== 私有辅助方法 ==========

    /**
     * @brief 尝试驯服狼
     * @param player 尝试驯服的玩家
     *
     * 1/3 概率驯服成功，成功时设置驯服状态和主人，
     * 广播 TamingSucceeded/TamingFailed 粒子效果。
     */
    void _tryToTame(Player& player);

    /**
     * @brief 获取狼食物的治疗量
     * @param item 食物物品
     * @return 治疗量（生命值），非食物返回 0
     *
     * MC 原版中治疗量为 2.0 * food.nutrition，
     * 此处使用硬编码映射近似 MC 原版的营养值。
     */
    [[nodiscard]] f32 _getFoodHealAmount(const Item* item) const;

    /**
     * @brief 获取染料物品对应的颜色
     * @param item 物品指针
     * @return 对应的 DyeColor，如果不是染料返回 std::nullopt
     */
    [[nodiscard]] static std::optional<DyeColor> _getDyeColorFromItem(const Item* item);

    /**
     * @brief 检查狼铠是否可以吸收此伤害
     *
     * 条件：穿戴狼铠 且 伤害源不绕过护甲
     * 参考: net.minecraft.world.entity.animal.wolf.Wolf.canArmorAbsorb()
     *
     * @param source 伤害来源
     * @return 如果狼铠可以吸收此伤害返回 true
     */
    [[nodiscard]] bool _canArmorAbsorb(const DamageSource& source) const;
};

} // namespace mc
