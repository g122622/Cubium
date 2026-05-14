#pragma once

#include "../../../../core/Types.hpp"
#include "../basic/AnimalEntity.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 豹猫实体
 *
 * 生活在丛林中的害羞动物。
 *
 * 特性：
 * - 信任机制：不完全驯服，建立信任后会跟随
 * - 逃跑：靠近时会逃跑，需要悄悄靠近
 * - 变种：驯服后变成猫（不同皮肤）
 * - 生鱼：使用生鱼驯服和繁殖
 * - 狩猎：会攻击小鸡和海龟
 *
 * 参考 MC 1.16.5 OcelotEntity
 */
class OcelotEntity : public AnimalEntity {
public:
    /**
     * @brief 豹猫类型
     */
    enum class OcelotType : u8 {
        Wild = 0,    // 野生豹猫
        Tuxedo = 1,  // 黑白猫（驯服后）
        Tabby = 2,   // 虎斑猫（驯服后）
        Red = 3,     // 红猫（驯服后）
        Siamese = 4, // 暹罗猫（驯服后）
        British = 5, // 英短（驯服后）
        Calico = 6,  // 三花猫（驯服后）
        Persian = 7, // 波斯猫（驯服后）
        Ragdoll = 8, // 布偶猫（驯服后）
        White = 9,   // 白猫（驯服后）
        Jellie = 10  // Jellie猫（驯服后）
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    OcelotEntity(LegacyEntityType type, EntityId id);
    ~OcelotEntity() override = default;

    // 禁止拷贝
    OcelotEntity(const OcelotEntity&) = delete;
    OcelotEntity& operator=(const OcelotEntity&) = delete;

    // 允许移动
    OcelotEntity(OcelotEntity&&) = default;
    OcelotEntity& operator=(OcelotEntity&&) = default;

    /**
     * @brief 创建豹猫实体
     * @param world 世界实例
     * @return 新的豹猫实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 信任系统 ==========

    /**
     * @brief 是否信任玩家
     * @param playerId 玩家ID
     * @return 是否信任
     */
    [[nodiscard]] bool trustsPlayer(u64 playerId) const;

    /**
     * @brief 设置信任玩家
     * @param playerId 玩家ID
     * @param trust 是否信任
     */
    void setPlayerTrust(u64 playerId, bool trust);

    /**
     * @brief 是否已被驯服
     * 豹猫的"驯服"实际上是建立信任
     */
    [[nodiscard]] bool isTrusting() const { return m_trusting; }

    /**
     * @brief 设置驯服状态
     */
    void setTrusting(bool trusting);

    /**
     * @brief 获取驯服玩家ID
     */
    [[nodiscard]] u64 getTrustingPlayerId() const { return m_trustingPlayerId; }

    // ========== 逃跑状态 ==========

    /**
     * @brief 是否正在逃跑
     */
    [[nodiscard]] bool isFleeing() const { return m_fleeing; }

    /**
     * @brief 设置逃跑状态
     */
    void setFleeing(bool fleeing) { m_fleeing = fleeing; }

    // ========== 类型 ==========

    /**
     * @brief 获取豹猫类型
     */
    [[nodiscard]] OcelotType getOcelotType() const { return m_ocelotType; }

    /**
     * @brief 设置豹猫类型
     */
    void setOcelotType(OcelotType type) { m_ocelotType = type; }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 豹猫使用生鱼繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.3f : 0.6f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 信任状态
    bool m_trusting = false;
    u64 m_trustingPlayerId = 0;

    // 逃跑状态
    bool m_fleeing = false;

    // 类型
    OcelotType m_ocelotType = OcelotType::Wild;

    // 常量
    static constexpr f32 FLEE_SPEED = 2.5f;   // 逃跑速度
    static constexpr f32 FOLLOW_SPEED = 0.8f; // 跟随速度
};

} // namespace mc
