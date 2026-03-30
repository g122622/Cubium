#pragma once

#include "AbstractFishEntity.hpp"
#include "../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 鳕鱼实体
 *
 * 常见的海洋鱼类。
 *
 * 特性：
 * - 群居：会与其他鳕鱼聚在一起
 * - 掉落：生鳕鱼、骨头
 *
 * 参考 MC 1.16.5 CodEntity
 */
class CodEntity : public AbstractFishEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    CodEntity(LegacyEntityType type, EntityId id);
    ~CodEntity() override = default;

    // 禁止拷贝
    CodEntity(const CodEntity&) = delete;
    CodEntity& operator=(const CodEntity&) = delete;

    // 允许移动
    CodEntity(CodEntity&&) = default;
    CodEntity& operator=(CodEntity&&) = default;

    /**
     * @brief 创建鳕鱼实体
     * @param world 世界实例
     * @return 新的鳕鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 群居 ==========

    /**
     * @brief 鳕鱼会群游
     */
    [[nodiscard]] bool canSchool() const override { return true; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.15f; }

protected:
    // ========== 属性注册 ==========
    void registerAttributes() override;
};

} // namespace mc
