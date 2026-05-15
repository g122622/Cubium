/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

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

    // MC 1.16.5: 蓝色凋灵之首运动因子为 0.73，普通为 0.95
    [[nodiscard]] f32 getMotionFactor() const override;
    // MC 1.16.5: 凋灵之首不燃烧
    [[nodiscard]] bool isFiery() const override;

private:
    bool m_blue = false;
};

} // namespace entity
} // namespace mc
