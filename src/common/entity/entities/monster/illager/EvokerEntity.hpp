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

#include "SpellcastingIllagerEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 唤魔者实体
 *
 * 灾厄村民的一种，能够施放尖牙攻击和召唤恼鬼。
 */
class EvokerEntity : public SpellcastingIllagerEntity {
public:
    EvokerEntity(EntityInstanceId id);
    ~EvokerEntity() override = default;

    EvokerEntity(const EvokerEntity&) = delete;
    EvokerEntity& operator=(const EvokerEntity&) = delete;
    EvokerEntity(EvokerEntity&&) = delete;
    EvokerEntity& operator=(EvokerEntity&&) = delete;

    static std::unique_ptr<Entity> create(IWorld* world);

    [[nodiscard]] bool isCasting() const { return isSpellcasting(); }
    [[nodiscard]] i32 getSpellType() const { return static_cast<i32>(spellType()); }

    void startCasting(i32 spellType);
    void finishCasting();

    /**
     * @brief 施放尖牙攻击
     */
    void castFangsAttack();

    /**
     * @brief 召唤恼鬼
     */
    void summonVex();

    [[nodiscard]] i32 getFangsCooldown() const { return m_fangsCooldown; }
    [[nodiscard]] i32 getSummonCooldown() const { return m_summonCooldown; }

    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;
    [[nodiscard]] const char* getSpellSoundId() const noexcept override { return "entity.evoker.cast_spell"; }

private:
    /**
     * @brief 在指定位置生成尖牙
     *
     * @param posX X 坐标
     * @param posZ Z 坐标
     * @param minY 最小 Y 坐标
     * @param maxY 最大 Y 坐标
     * @param angle 尖牙朝向角度（弧度）
     * @param warmupDelay 预热延迟（ticks）
     */
    void _spawnFangs(f32 posX, f32 posZ, f32 minY, f32 maxY, f32 angle, i32 warmupDelay);

    i32 m_fangsCooldown = 0;
    i32 m_summonCooldown = 0;

    static constexpr i32 CASTING_DURATION = 40;
    static constexpr i32 FANGS_COOLDOWN = 100;
    static constexpr i32 SUMMON_COOLDOWN = 340;
};

} // namespace mc
