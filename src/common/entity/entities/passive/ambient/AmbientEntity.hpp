#pragma once

#include "../../../core/MobEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 环境生物基类
 *
 * 不主动与玩家交互的生物基类。
 *
 * 参考 MC 1.16.5 AmbientEntity
 */
class AmbientEntity : public MobEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    AmbientEntity(LegacyEntityType type, EntityId id);
    ~AmbientEntity() override = default;

    // 禁止拷贝
    AmbientEntity(const AmbientEntity&) = delete;
    AmbientEntity& operator=(const AmbientEntity&) = delete;

    // 允许移动
    AmbientEntity(AmbientEntity&&) = default;
    AmbientEntity& operator=(AmbientEntity&&) = default;

protected:
    void registerAttributes() override;
};

} // namespace mc
