#pragma once

#include "../../../../core/Types.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../MonsterEntity.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class IWorld;
class DamageSource;
class LivingEntity;

/**
 * @brief 史莱姆实体
 *
 * 弹跳的绿色果冻状怪物。
 *
 * 特性：
 * - 分裂：被杀死时分裂成小史莱姆
 * - 弹跳：持续弹跳移动
 * - 尺寸：有4种尺寸（微小、小、中、大）
 * - 掉落：粘液球（仅小尺寸）
 * - 生成：只在特定区块
 *
 * 参考 MC 1.16.5 SlimeEntity
 */
class SlimeEntity : public MonsterEntity {
public:
    using Entity::onCollideWithPlayer;

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    SlimeEntity(LegacyEntityType type, EntityId id);
    ~SlimeEntity() override = default;

    // 禁止拷贝
    SlimeEntity(const SlimeEntity&) = delete;
    SlimeEntity& operator=(const SlimeEntity&) = delete;

    // 允许移动
    SlimeEntity(SlimeEntity&&) = default;
    SlimeEntity& operator=(SlimeEntity&&) = default;

    /**
     * @brief 创建史莱姆实体
     * @param world 世界实例
     * @return 新的史莱姆实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     * MC 1.16.5: 无环境音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override
    {
        return std::nullopt; // 史莱姆无环境音效
    }

    /**
     * @brief 获取受伤声音
     * MC 1.16.5: 小史莱姆用 hurt_small，大史莱姆用 hurt
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     * MC 1.16.5: 小史莱姆用 death_small，大史莱姆用 death
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取挤压声音
     * MC 1.16.5: 着地时播放
     */
    [[nodiscard]] std::optional<ResourceLocation> getSquishSound() const;

    /**
     * @brief 获取跳跃声音
     * MC 1.16.5: 跳跃时播放
     */
    [[nodiscard]] std::optional<ResourceLocation> getJumpSound() const;

    // ========== 尺寸系统 ==========

    /**
     * @brief 获取史莱姆尺寸
     * 尺寸范围 1-4，1=微小，2=小，4=大
     */
    [[nodiscard]] i32 getSlimeSize() const { return m_size; }

    /**
     * @brief 设置史莱姆尺寸
     * @param size 尺寸（1-4）
     * @param resetHealth 是否重置生命值
     */
    void setSlimeSize(i32 size, bool resetHealth = true);

    /**
     * @brief 是否是小史莱姆
     * MC 1.16.5: isSmallSlime() - size <= 1
     */
    [[nodiscard]] bool isSmallSlime() const { return m_size <= 1; }

    /**
     * @brief 是否可以对玩家造成伤害
     * MC 1.16.5: canDamagePlayer() - !isSmallSlime() && isServerWorld()
     */
    [[nodiscard]] bool canDamagePlayer() const;

    // ========== 挤压动画 ==========

    /**
     * @brief 获取挤压量
     */
    [[nodiscard]] f32 squishAmount() const { return m_squishAmount; }

    /**
     * @brief 获取挤压因子
     */
    [[nodiscard]] f32 squishFactor() const { return m_squishFactor; }

    /**
     * @brief 获取上一帧挤压因子
     */
    [[nodiscard]] f32 prevSquishFactor() const { return m_prevSquishFactor; }

    // ========== 弹跳 ==========

    /**
     * @brief 获取跳跃延迟
     * MC 1.16.5: getJumpDelay() - random 10-30 ticks
     */
    [[nodiscard]] i32 getJumpDelay() const;

    /**
     * @brief 跳跃时是否发出声音
     * MC 1.16.5: makesSoundOnJump() - size > 0
     */
    [[nodiscard]] bool makesSoundOnJump() const { return m_size > 0; }

    // ========== 分裂 ==========

    /**
     * @brief 分裂成小史莱姆
     * MC 1.16.5: 在 remove() 中调用
     * @deprecated 使用 performSplit() 替代
     */
    void split();

    /**
     * @brief 执行分裂逻辑
     *
     * 在实体被移除时生成 2-4 个小史莱姆。
     * MC 1.16.5: SlimeEntity.remove() 中的分裂逻辑
     */
    void performSplit();

    /**
     * @brief 检查是否可以分裂
     */
    [[nodiscard]] bool canSplit() const { return m_size > 1; }

    // ========== 攻击 ==========

    /**
     * @brief 对目标造成伤害
     * MC 1.16.5: dealDamage()
     */
    void dealDamage(LivingEntity& target);

    // ========== 碰撞 ==========

    /**
     * @brief 玩家碰撞处理
     * MC 1.16.5: onCollideWithPlayer()
     */
    void onCollideWithPlayer(LivingEntity& player);

    // ========== 阳光燃烧 ==========

    /**
     * @brief 史莱姆不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     * MC 1.16.5: 0.625F * height
     */
    [[nodiscard]] f32 eyeHeight() const override;

    /**
     * @brief 获取实体尺寸
     * MC 1.16.5: scale by 0.255F * size
     */
    [[nodiscard]] entity::EntitySize getDimensions(EntityPose pose) const override;

    /**
     * @brief 获取声音音量
     * MC 1.16.5: 0.4F * size
     */
    [[nodiscard]] f32 getSoundVolume() const override { return 0.4f * static_cast<f32>(m_size); }

    /**
     * @brief 获取垂直面部旋转速度
     * MC 1.16.5: 0 (史莱姆不会抬头低头)
     */
    [[nodiscard]] f32 getVerticalFaceSpeed() const override { return 0.0f; }

    // ========== 经验值 ==========

    /**
     * @brief 死亡时掉落经验
     * MC 1.16.5: 经验值等于尺寸
     */
    void dropExperience() override;

    // ========== 生命周期 ==========

    /**
     * @brief 移除实体
     *
     * 重写以实现史莱姆分裂逻辑。
     * MC 1.16.5: 在 remove() 中触发分裂
     */
    void remove() override;

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 根据尺寸更新属性
     */
    void updateSizeAttributes();

    /**
     * @brief 更新挤压量
     * MC 1.16.5: alterSquishAmount()
     */
    void alterSquishAmount();

private:
    // 尺寸
    i32 m_size = 1;

    // 挤压动画
    f32 m_squishAmount = 0.0f;
    f32 m_squishFactor = 0.0f;
    f32 m_prevSquishFactor = 0.0f;

    // 地面状态追踪
    bool m_wasOnGround = false;

    // MC 1.16.5 常量
    static constexpr f32 SIZE_SCALE = 0.255f;        // 尺寸缩放因子
    static constexpr f32 EYE_HEIGHT_FACTOR = 0.625f; // 眼睛高度因子
    static constexpr i32 SPLIT_COUNT_MIN = 2;        // 分裂最小数量
    static constexpr i32 SPLIT_COUNT_MAX = 4;        // 分裂最大数量
};

} // namespace mc
