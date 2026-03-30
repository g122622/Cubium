#pragma once

#include "AbstractIllagerEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 唤魔者实体
 *
 * 灾厄村民的法术使用者，会召唤恼鬼和施放尖牙攻击。
 *
 * 特性：
 * - 尖牙攻击：召唤地刺攻击目标
 * - 召唤恼鬼：召唤恼鬼协助战斗
 * - Boss血条：显示Boss血条
 * - 掠夺：参与掠夺事件
 *
 * 参考 MC 1.16.5 EvokerEntity
 */
class EvokerEntity : public AbstractIllagerEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    EvokerEntity(LegacyEntityType type, EntityId id);
    ~EvokerEntity() override = default;

    // 禁止拷贝
    EvokerEntity(const EvokerEntity&) = delete;
    EvokerEntity& operator=(const EvokerEntity&) = delete;

    // 允许移动
    EvokerEntity(EvokerEntity&&) = default;
    EvokerEntity& operator=(EvokerEntity&&) = default;

    /**
     * @brief 创建唤魔者实体
     * @param world 世界实例
     * @return 新的唤魔者实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 法术系统 ==========

    /**
     * @brief 是否正在施法
     */
    [[nodiscard]] bool isCasting() const { return m_casting; }

    /**
     * @brief 设置施法状态
     */
    void setCasting(bool casting) { m_casting = casting; }

    /**
     * @brief 获取法术类型
     */
    [[nodiscard]] i32 getSpellType() const { return m_spellType; }

    /**
     * @brief 开始施法
     * @param spellType 法术类型
     */
    void startCasting(i32 spellType);

    /**
     * @brief 完成施法
     */
    void finishCasting();

    /**
     * @brief 施放尖牙攻击
     */
    void castFangsAttack();

    /**
     * @brief 召唤恼鬼
     */
    void summonVex();

    // ========== 战斗系统 ==========

    /**
     * @brief 获取尖牙攻击冷却
     */
    [[nodiscard]] i32 getFangsCooldown() const { return m_fangsCooldown; }

    /**
     * @brief 获取召唤冷却
     */
    [[nodiscard]] i32 getSummonCooldown() const { return m_summonCooldown; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // 法术状态
    bool m_casting = false;
    i32 m_spellType = 0;        // 0=无, 1=尖牙, 2=召唤
    i32 m_castingTime = 0;

    // 冷却
    i32 m_fangsCooldown = 0;
    i32 m_summonCooldown = 0;

    // 常量
    static constexpr i32 CASTING_DURATION = 40;   // 施法动画时间
    static constexpr i32 FANGS_COOLDOWN = 100;    // 尖牙冷却
    static constexpr i32 SUMMON_COOLDOWN = 340;   // 召唤冷却
    static constexpr i32 MAX_VEX_COUNT = 8;       // 最大恼鬼数量
};

} // namespace mc
