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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/entities/passive/water/WaterMobEntity.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/block/BlockPos.hpp"

#include <memory>

namespace mc {

// Forward declarations
class Player;
class LivingEntity;
class DamageSource;

/**
 * @brief 美西螈变体枚举
 *
 * 美西螈有5种颜色变体，其中蓝色为稀有变体（1/1200概率）。
 */
enum class AxolotlVariant : i32 {
    Lucy = 0, ///< 白化（粉色）
    Wild = 1, ///< 野生（棕色）
    Gold = 2, ///< 金色
    Cyan = 3, ///< 青色
    Blue = 4  ///< 蓝色（稀有）
};

/**
 * @brief 美西螈实体
 *
 * 生活在繁茂洞穴水中的两栖动物，是美西螈的唯一生成地。
 *
 * 特性：
 * - 五种颜色变体（Lucy/Wild/Gold/Cyan/Blue）
 * - 装死行为：在水中受击33%概率装死，持续200tick
 * - 攻击水生敌对生物（溺尸、守卫者）和鱼类
 * - 击杀后给予附近玩家再生效果
 * - 可用热带鱼桶繁殖
 * - 可用水桶拾取
 * - 6000tick空气储备（5分钟）
 *
 * AI 目标（MC 1.17+ 优先级）：
 * - 0: SwimGoal - 水中浮起
 * - 0: FindWaterGoal - 寻找水源
 * - 1: AxolotlPlayDeadGoal - 装死
 * - 2: PanicGoal - 恐慌逃跑
 * - 3: TemptGoal - 跟随食物（热带鱼桶）
 * - 4: MeleeAttackGoal - 近战攻击
 * - 5: RandomSwimmingGoal - 随机游泳
 * - 6: LookAtGoal - 看向玩家
 * - 7: LookRandomlyGoal - 随机看向
 *
 * 注意：BreedGoal 和 FollowParentGoal 需要 AnimalEntity，美西螈继承自
 * WaterMobEntity 而非 AnimalEntity，繁殖通过水桶交互机制实现。
 *
 * 参考: net.minecraft.world.entity.animal.axolotl.Axolotl
 */
class AxolotlEntity : public WaterMobEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    AxolotlEntity(EntityInstanceId id);
    ~AxolotlEntity() override = default;

    // 禁止拷贝
    AxolotlEntity(const AxolotlEntity&) = delete;
    AxolotlEntity& operator=(const AxolotlEntity&) = delete;

    // 允许移动
    AxolotlEntity(AxolotlEntity&&) = delete;
    AxolotlEntity& operator=(AxolotlEntity&&) = delete;

    /**
     * @brief 创建美西螈实体
     * @param world 世界实例
     * @return 新的美西螈实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 变体 ==========

    /**
     * @brief 获取变体
     */
    [[nodiscard]] AxolotlVariant getVariant() const noexcept { return m_variant; }

    /**
     * @brief 设置变体
     */
    void setVariant(AxolotlVariant variant) noexcept { m_variant = variant; }

    /**
     * @brief 随机选择一个普通变体（Lucy/Wild/Gold/Cyan）
     */
    void randomizeVariant();

    // ========== 装死 ==========

    /**
     * @brief 是否正在装死
     */
    [[nodiscard]] bool isPlayingDead() const noexcept { return m_playingDead; }

    /**
     * @brief 设置装死状态
     */
    void setPlayingDead(bool playingDead);

    /**
     * @brief 装死剩余tick数
     */
    [[nodiscard]] i32 getPlayingDeadTimer() const noexcept { return m_playingDeadTimer; }

    /**
     * @brief 设置装死计时器
     */
    void setPlayingDeadTimer(i32 ticks) { m_playingDeadTimer = ticks; }

    // ========== 桶装 ==========

    /**
     * @brief 是否来自桶
     * 来自桶的美西螈不会消失
     */
    [[nodiscard]] bool isFromBucket() const noexcept { return m_fromBucket; }

    /**
     * @brief 设置来自桶标记
     */
    void setFromBucket(bool fromBucket) noexcept { m_fromBucket = fromBucket; }

    // ========== 狩猎冷却 ==========

    /**
     * @brief 是否在狩猎冷却中
     */
    [[nodiscard]] bool hasHuntingCooldown() const noexcept { return m_huntingCooldown > 0; }

    /**
     * @brief 设置狩猎冷却
     * @param ticks 冷却tick数
     */
    void setHuntingCooldown(i32 ticks) { m_huntingCooldown = ticks; }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否是繁殖食物（热带鱼桶）
     */
    [[nodiscard]] bool isFoodItem(const ItemStack& itemStack) const;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const noexcept override { return 0.2751f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const noexcept override { return 0.75f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const noexcept override { return 0.42f; }

    /**
     * @brief 获取最大空气供应量
     * 美西螈有 6000 tick (5分钟) 的空气储备
     */
    [[nodiscard]] i32 maxAir() const noexcept override { return MAX_AIR_SUPPLY; }

    /**
     * @brief 是否可以被拴绳牵引
     */
    [[nodiscard]] bool canBeLeashed() const override { return true; }

    /**
     * @brief 是否会被水流推动
     * 美西螈不会被水流推动
     */
    [[nodiscard]] bool canBePushedByWater() const override { return false; }

    /**
     * @brief 是否可以被作为敌人看到
     * 装死时不能被作为敌人看到
     */
    [[nodiscard]] bool canBeSeenAsEnemy() const;

    /**
     * @brief 检查是否应消失
     * 来自桶或自定义名称的美西螈不会消失
     */
    [[nodiscard]] bool canDespawn(double distanceToClosestPlayer) const override;

    /**
     * @brief 检查是否需要持久化
     * 来自桶的美西螈需要持久化
     */
    [[nodiscard]] bool preventDespawn() const override;

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     * 在水中和陆地播放不同的音效
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
     * @brief 播放攻击音效
     */
    void playAttackSound(LivingEntity& target) override;

    // ========== 支援效果 ==========

    /**
     * @brief 给予附近玩家再生效果
     *
     * 当美西螈击杀目标后，给予20格内的玩家再生I效果。
     * 持续时间：基础100tick + 现有剩余时间（上限2400tick）。
     */
    void applySupportingEffects(Player& player);

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 受伤处理 ==========
    bool hurt(DamageSource& source, f32 amount) override;

private:
    // 变体
    AxolotlVariant m_variant = AxolotlVariant::Lucy;

    // 装死状态
    bool m_playingDead = false;
    i32 m_playingDeadTimer = 0;

    // 桶装标记
    bool m_fromBucket = false;

    // 狩猎冷却（击杀目标后进入2分钟冷却）
    i32 m_huntingCooldown = 0;

    // 常量
    static constexpr i32 MAX_AIR_SUPPLY = 6000;               ///< 5分钟空气储备 (ticks)
    static constexpr i32 PLAY_DEAD_DURATION = 200;            ///< 装死持续时间 (ticks)
    static constexpr i32 REGEN_BUFF_BASE_DURATION = 100;      ///< 再生效果基础持续时间 (ticks)
    static constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;      ///< 再生效果最大持续时间 (ticks)
    static constexpr f64 PLAYER_REGEN_DETECTION_RANGE = 20.0; ///< 再生效果检测玩家范围 (方块)
    static constexpr i32 HUNTING_COOLDOWN_DURATION = 2400;    ///< 狩猎冷却持续时间 (ticks)

    /**
     * @brief 更新装死状态
     */
    void _updatePlayingDead();

    /**
     * @brief 更新狩猎冷却
     */
    void _updateHuntingCooldown();

    /**
     * @brief 检查是否需要给予附近玩家支援效果
     *
     * 当美西螈的攻击目标死亡时，检查最后一击是否由玩家造成，
     * 如果该玩家在20格范围内，则给予再生I效果并移除挖掘疲劳。
     */
    void _checkSupportingEffects();

    // 上一tick攻击目标是否存活（用于检测目标死亡时刻）
    bool m_wasTargetAlive = false;
};

} // namespace mc
