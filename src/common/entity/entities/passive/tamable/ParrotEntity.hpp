#pragma once

#include "../tamable/TameableEntity.hpp"
#include "../../../core/Types.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Player;
class LivingEntity;

/**
 * @brief 鹦鹉实体
 *
 * 生活在丛林中的可驯服鸟类。
 *
 * 特性：
 * - 驯服：使用种子驯服
 * - 站肩膀：驯服后可以站在玩家肩膀上
 * - 飞行：可以飞行
 * - 模仿声音：会模仿附近生物的声音
 * - 坐下/站起：可以命令坐下
 * - 变种：5种不同颜色
 *
 * 参考 MC 1.16.5 ParrotEntity
 */
class ParrotEntity : public TameableEntity {
public:
    /**
     * @brief 鹦鹉变种
     */
    enum class ParrotVariant : u8 {
        RedBlue = 0,    // 红蓝鹦鹉
        Blue = 1,       // 蓝色鹦鹉
        Green = 2,      // 绿色鹦鹉
        YellowBlue = 3, // 黄蓝鹦鹉
        Gray = 4        // 灰色鹦鹉
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    ParrotEntity(LegacyEntityType type, EntityId id);
    ~ParrotEntity() override = default;

    // 禁止拷贝
    ParrotEntity(const ParrotEntity&) = delete;
    ParrotEntity& operator=(const ParrotEntity&) = delete;

    // 允许移动
    ParrotEntity(ParrotEntity&&) = default;
    ParrotEntity& operator=(ParrotEntity&&) = default;

    /**
     * @brief 创建鹦鹉实体
     * @param world 世界实例
     * @return 新的鹦鹉实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 变种系统 ==========

    /**
     * @brief 获取鹦鹉变种
     */
    [[nodiscard]] ParrotVariant getVariant() const { return m_variant; }

    /**
     * @brief 设置鹦鹉变种
     */
    void setVariant(ParrotVariant variant) { m_variant = variant; }

    /**
     * @brief 随机设置变种
     */
    void randomizeVariant();

    // ========== 飞行系统 ==========

    /**
     * @brief 是否正在飞行
     */
    [[nodiscard]] bool isFlying() const { return m_flying; }

    /**
     * @brief 设置飞行状态
     */
    void setFlying(bool flying) { m_flying = flying; }

    /**
     * @brief 是否可以飞行
     */
    [[nodiscard]] bool canFly() const { return true; }

    // ========== 站肩膀系统 ==========

    /**
     * @brief 是否站在玩家肩膀上
     */
    [[nodiscard]] bool isOnShoulder() const { return m_onShoulder; }

    /**
     * @brief 设置站肩膀状态
     */
    void setOnShoulder(bool onShoulder) { m_onShoulder = onShoulder; }

    /**
     * @brief 获取肩膀上的玩家ID
     */
    [[nodiscard]] u64 getShoulderPlayerId() const { return m_shoulderPlayerId; }

    /**
     * @brief 站到玩家肩膀上
     * @param playerId 玩家ID
     * @return 是否成功
     */
    bool mountShoulder(u64 playerId);

    /**
     * @brief 从肩膀上下来
     */
    void dismountShoulder();

    // ========== 模仿声音 ==========

    /**
     * @brief 是否正在模仿声音
     */
    [[nodiscard]] bool isImitating() const { return m_imitating; }

    /**
     * @brief 设置模仿状态
     */
    void setImitating(bool imitating) { m_imitating = imitating; }

    /**
     * @brief 获取模仿的目标类型
     */
    [[nodiscard]] u32 getImitatingTarget() const { return m_imitatingTarget; }

    /**
     * @brief 设置模仿目标
     */
    void setImitatingTarget(u32 entityType) {
        m_imitatingTarget = entityType;
        m_imitating = true;
    }

    // ========== 驯服 ==========

    /**
     * @brief 检查物品是否可用于驯服
     * 鹦鹉使用种子驯服
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const;

    /**
     * @brief 检查物品是否可用于繁殖
     * 鹦鹉不能繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override {
        (void)itemStack;
        return false;
    }

    /**
     * @brief 生成幼体
     * 鹦鹉不能繁殖
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override {
        (void)partner;
        return nullptr;
    }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.25f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 驯服回调 ==========
    void onTamed(bool tamed) override;

private:
    // 变种
    ParrotVariant m_variant = ParrotVariant::RedBlue;

    // 飞行状态
    bool m_flying = false;

    // 站肩膀状态
    bool m_onShoulder = false;
    u64 m_shoulderPlayerId = 0;

    // 模仿状态
    bool m_imitating = false;
    u32 m_imitatingTarget = 0;
    i32 m_imitateTimer = 0;

    // 飞行计时器
    i32 m_flapTimer = 0;
    f32 m_flapSpeed = 0.0f;

    // 常量
    static constexpr i32 IMITATE_INTERVAL_MIN = 100;  // 最小模仿间隔
    static constexpr i32 IMITATE_INTERVAL_MAX = 600;  // 最大模仿间隔
    static constexpr f32 FLAP_SPEED_GROUND = 0.0f;    // 地面上的扑翼速度
    static constexpr f32 FLAP_SPEED_FLYING = 0.4f;    // 飞行时的扑翼速度
};

} // namespace mc
