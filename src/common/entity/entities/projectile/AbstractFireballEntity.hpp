#pragma once

#include "DamagingProjectileEntity.hpp"

#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 抽象火球实体基类
 *
 * 对齐 1.16.5 `AbstractFireballEntity` 的层次位置，
 * 当前只保留火球族共用的最小公共语义。
 */
class AbstractFireballEntity : public DamagingProjectileEntity {
public:
    ~AbstractFireballEntity() override = default;

    [[nodiscard]] f32 width() const override { return 1.0f; }
    [[nodiscard]] f32 height() const override { return 1.0f; }

protected:
    AbstractFireballEntity(LegacyEntityType type, EntityId id);
};

class FireballEntity : public AbstractFireballEntity {
public:
    static std::unique_ptr<Entity> create(IWorld* world);

    FireballEntity(LegacyEntityType type, EntityId id);

    [[nodiscard]] f32 width() const override { return 1.0f; }
    [[nodiscard]] f32 height() const override { return 1.0f; }

    [[nodiscard]] i32 explosionPower() const { return m_explosionPower; }
    void setExplosionPower(i32 power) { m_explosionPower = power; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;

private:
    i32 m_explosionPower = 1;
};

class SmallFireballEntity : public AbstractFireballEntity {
public:
    static std::unique_ptr<Entity> create(IWorld* world);

    SmallFireballEntity(LegacyEntityType type, EntityId id);

    [[nodiscard]] f32 width() const override { return 0.3125f; }
    [[nodiscard]] f32 height() const override { return 0.3125f; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;
};

class DragonFireballEntity : public AbstractFireballEntity {
public:
    static std::unique_ptr<Entity> create(IWorld* world);

    DragonFireballEntity(LegacyEntityType type, EntityId id);

    [[nodiscard]] f32 width() const override { return 1.0f; }
    [[nodiscard]] f32 height() const override { return 1.0f; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;
};

class WitherSkullEntity : public AbstractFireballEntity {
public:
    static std::unique_ptr<Entity> create(IWorld* world);

    WitherSkullEntity(LegacyEntityType type, EntityId id);

    [[nodiscard]] f32 width() const override { return 0.3125f; }
    [[nodiscard]] f32 height() const override { return 0.3125f; }

    [[nodiscard]] bool isBlue() const { return m_blue; }
    void setBlue(bool blue) { m_blue = blue; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;

private:
    bool m_blue = false;
};

} // namespace entity
} // namespace mc
