#pragma once

#include "SpiderEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 洞穴蜘蛛实体
 *
 * 在废弃矿井中生成的小型蜘蛛变种。
 *
 * 特性：
 * - 中毒：攻击会造成中毒效果
 * - 小型：比普通蜘蛛更小
 * - 爬墙：可以爬墙
 *
 * 参考 MC 1.16.5 CaveSpiderEntity
 */
class CaveSpiderEntity : public SpiderEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    CaveSpiderEntity(LegacyEntityType type, EntityId id);

    ~CaveSpiderEntity() override = default;

    // 禁止拷贝
    CaveSpiderEntity(const CaveSpiderEntity&) = delete;
    CaveSpiderEntity& operator=(const CaveSpiderEntity&) = delete;

    // 允许移动
    CaveSpiderEntity(CaveSpiderEntity&&) = default;
    CaveSpiderEntity& operator=(CaveSpiderEntity&&) = default;

    /**
     * @brief 创建洞穴蜘蛛实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 中毒攻击 ==========

    /**
     * @brief 攻击是否会中毒
     */
    [[nodiscard]] bool canPoison() const { return true; }

    /**
     * @brief 获取中毒持续时间（秒）
     */
    [[nodiscard]] i32 getPoisonDuration() const { return m_poisonDuration; }

    /**
     * @brief 设置中毒持续时间
     */
    void setPoisonDuration(i32 duration) { m_poisonDuration = duration; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.45f; }

protected:
    void registerAttributes() override;

private:
    i32 m_poisonDuration = 7; // 默认7秒中毒
};

} // namespace mc
