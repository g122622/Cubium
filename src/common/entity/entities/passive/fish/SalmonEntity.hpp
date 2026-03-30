#pragma once

#include "AbstractFishEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 鲑鱼实体
 *
 * 常见的海洋和河流鱼类。
 *
 * 特性：
 * - 群居：会与其他鲑鱼聚在一起
 * - 掉落：生鲑鱼、骨头
 *
 * 参考 MC 1.16.5 SalmonEntity
 */
class SalmonEntity : public AbstractFishEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    SalmonEntity(LegacyEntityType type, EntityId id);
    ~SalmonEntity() override = default;

    // 禁止拷贝
    SalmonEntity(const SalmonEntity&) = delete;
    SalmonEntity& operator=(const SalmonEntity&) = delete;

    // 允许移动
    SalmonEntity(SalmonEntity&&) = default;
    SalmonEntity& operator=(SalmonEntity&&) = default;

    /**
     * @brief 创建鲑鱼实体
     * @param world 世界实例
     * @return 新的鲑鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 群居 ==========

    /**
     * @brief 鲑鱼会群游
     */
    [[nodiscard]] bool canSchool() const override { return true; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.1f; }

protected:
    // ========== 属性注册 ==========
    void registerAttributes() override;
};

} // namespace mc
