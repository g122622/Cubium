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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/golem/GolemEntity.hpp"
#include "common/entity/interfaces/IRangedAttackMob.hpp"
#include "common/entity/interfaces/IShearable.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace mc {

// Forward declarations
class SnowballEntity;

/**
 * @brief 雪傀儡实体
 *
 * 由玩家创造的傀儡，用于保护玩家免受敌对生物攻击。
 *
 * 特性：
 * - 投掷雪球：向敌对生物投掷雪球攻击
 * - 留下雪迹：在寒冷生物群系行走时会留下雪层
 * - 融化：在高温生物群系（温度 > 1.0）或水中会融化（受到伤害）
 * - 掉落：雪球（0-15个）
 * - 南瓜头：可以用剪刀取下南瓜
 */
class SnowGolemEntity : public GolemEntity, public entity::IShearable, public entity::IRangedAttackMob {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     * @param registry 实体注册表（ECS）
     */
    SnowGolemEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~SnowGolemEntity() override = default;

    // 禁止拷贝
    SnowGolemEntity(const SnowGolemEntity&) = delete;
    SnowGolemEntity& operator=(const SnowGolemEntity&) = delete;

    // 允许移动
    SnowGolemEntity(SnowGolemEntity&&) = delete;
    SnowGolemEntity& operator=(SnowGolemEntity&&) = delete;

    /**
     * @brief 创建雪傀儡实体
     * @param world 世界实例
     * @return 新的雪傀儡实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 南瓜头 ==========

    /**
     * @brief 是否戴着南瓜
     */
    [[nodiscard]] bool hasPumpkin() const { return m_hasPumpkin; }

    /**
     * @brief 设置南瓜状态
     */
    void setPumpkin(bool hasPumpkin) { m_hasPumpkin = hasPumpkin; }

    // ========== IShearable 接口实现 ==========

    /**
     * @brief 检查是否可以被剪毛
     * @return 如果戴着南瓜返回 true
     */
    [[nodiscard]] bool isShearable() const override { return hasPumpkin(); }

    /**
     * @brief 剪毛
     * @param player 执行剪毛的玩家（可为 nullptr）
     * @return 获得的南瓜物品
     */
    std::vector<ItemStack> shear(Player* player = nullptr) override;

    // ========== 融化 ==========

    /**
     * @brief 是否会融化
     * 检查当前环境是否会导致融化（高温生物群系或水中）
     */
    [[nodiscard]] bool willMelt() const;

    // ========== 水敏感性 ==========

    /**
     * @brief 雪傀儡对水敏感
     *
     * 对齐 MC 1.21.11 SnowGolem.isSensitiveToWater() 返回 true。
     * 同时被 PotionEntity::onHitAsWater 查询：水瓶命中范围内水敏感实体
     * 受 1.0 indirectMagic 伤害（AbstractThrownPotion.onHitAsWater:93-95）。
     */
    [[nodiscard]] bool isWaterSensitive() const override { return true; }

    // ========== IRangedAttackMob 接口实现 ==========

    /**
     * @brief 对目标进行远程攻击（投掷雪球）
     * @param target 攻击目标
     * @param charge 蓄力程度（雪傀儡不使用）
     */
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    /**
     * @brief 获取攻击间隔
     * @return 攻击间隔（ticks）
     */
    [[nodiscard]] i32 getAttackInterval() const override { return ATTACK_COOLDOWN; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.7f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 0.7f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 1.9f; }

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // ========== 私有方法 ==========

    /**
     * @brief 检查是否可以放置雪层
     */
    [[nodiscard]] bool _canPlaceSnow() const;

    /**
     * @brief 在脚下放置雪层
     */
    void _placeSnowLayer();

private:
    // 南瓜头状态
    bool m_hasPumpkin = true;

    // 融化计时器
    i32 m_meltTimer = 0;

    // 常量
    static constexpr i32 ATTACK_COOLDOWN = 20;        // 雪球攻击冷却（ticks）
    static constexpr f32 SNOWBALL_VELOCITY = 1.6f;    // 雪球速度
    static constexpr f32 SNOWBALL_INACCURACY = 12.0f; // 雪球散布
    static constexpr f32 MELT_TEMPERATURE = 1.0f;     // 融化温度阈值
    static constexpr f32 SNOW_TEMPERATURE = 0.8f;     // 放置雪的温度阈值
    static constexpr i32 MELT_DAMAGE_INTERVAL = 20;   // 融化伤害间隔（ticks）
    static constexpr f32 MELT_DAMAGE = 1.0f;          // 融化伤害量
};

} // namespace mc
