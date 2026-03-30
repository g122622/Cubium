#pragma once

#include "../MonsterEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 苦力怕实体
 *
 * 会爆炸的敌对生物。
 *
 * 特性：
 * - 爆炸：靠近玩家时会爆炸
 * - 闪烁：爆炸前会闪烁
 * - 害怕猫：会被猫吓跑
 * - 雷击：被雷击中变成高压苦力怕
 *
 * 参考 MC 1.16.5 CreeperEntity
 */
class CreeperEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    CreeperEntity(LegacyEntityType type, EntityId id);
    ~CreeperEntity() override = default;

    // 禁止拷贝
    CreeperEntity(const CreeperEntity&) = delete;
    CreeperEntity& operator=(const CreeperEntity&) = delete;

    // 允许移动
    CreeperEntity(CreeperEntity&&) = default;
    CreeperEntity& operator=(CreeperEntity&&) = default;

    /**
     * @brief 创建苦力怕实体
     * @param world 世界实例
     * @return 新的苦力怕实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 爆炸系统 ==========

    /**
     * @brief 获取爆炸状态
     * 0 = 安全, 1 = 膨胀中
     */
    [[nodiscard]] i32 getFuseTime() const { return m_fuseTime; }

    /**
     * @brief 设置爆炸状态
     */
    void setFuseTime(i32 time) { m_fuseTime = time; }

    /**
     * @brief 是否点燃
     */
    [[nodiscard]] bool isIgnited() const { return m_ignited; }

    /**
     * @brief 设置点燃状态
     */
    void setIgnited(bool ignited) { m_ignited = ignited; }

    /**
     * @brief 是否正在膨胀
     */
    [[nodiscard]] bool isSwell() const { return m_swell > 0; }

    /**
     * @brief 获取膨胀值
     */
    [[nodiscard]] i32 getSwell() const { return m_swell; }

    /**
     * @brief 设置膨胀值
     */
    void setSwell(i32 swell) { m_swell = swell; }

    // ========== 高压 ==========

    /**
     * @brief 是否是高压苦力怕
     */
    [[nodiscard]] bool isPowered() const { return m_powered; }

    /**
     * @brief 设置高压状态
     */
    void setPowered(bool powered) { m_powered = powered; }

    // ========== 爆炸 ==========

    /**
     * @brief 引爆炸药
     */
    void explode();

    /**
     * @brief 获取爆炸威力
     */
    [[nodiscard]] f32 getExplosionPower() const { return m_powered ? POWERED_EXPLOSION_POWER : NORMAL_EXPLOSION_POWER; }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 苦力怕不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.54f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 爆炸状态
    i32 m_fuseTime = 0;
    i32 m_swell = 0;
    i32 m_oldSwell = 0;
    bool m_ignited = false;
    bool m_powered = false;

    // 常量
    static constexpr i32 FUSE_DURATION = 30;        // 1.5秒点燃时间
    static constexpr i32 MAX_SWELL = 30;            // 最大膨胀值
    static constexpr f32 NORMAL_EXPLOSION_POWER = 3.0f;
    static constexpr f32 POWERED_EXPLOSION_POWER = 6.0f;
    static constexpr f32 DETONATE_DISTANCE = 3.0f;  // 触发爆炸距离
};

} // namespace mc
