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

#include "AbstractIllagerEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc {

/**
 * @brief 施法型灾厄村民公共基类
 *
 * 提供施法型灾厄村民的公共功能：
 * - 当前激活法术类型
 * - 法术持续 tick
 * - 服务端施法状态判定
 * - 施法粒子颜色反馈
 */
class SpellcastingIllagerEntity : public AbstractIllagerEntity {
public:
    /**
     * @brief 法术类型枚举
     *
     * 每种法术类型对应不同的粒子颜色（RGB 值作为速度参数）
     */
    enum class SpellType : u8 {
        None = 0,      ///< 无施法
        SummonVex = 1, ///< 召唤恼鬼（唤魔者）- 淡蓝白色 (0.7, 0.7, 0.8)
        Fangs = 2,     ///< 尖牙攻击（唤魔者）- 棕色 (0.4, 0.3, 0.35)
        Wololo = 3,    ///< 唔噜噜法术（唤魔者）- 橙黄色 (0.7, 0.5, 0.2)
        Disappear = 4, ///< 消失/镜像法术（幻术师）- 蓝色 (0.3, 0.3, 0.8)
        Blindness = 5  ///< 失明法术（幻术师）- 深蓝/深紫色 (0.1, 0.1, 0.2)
    };

    SpellcastingIllagerEntity(EntityInstanceId id);
    ~SpellcastingIllagerEntity() override = default;

    /// 本类继承链标识（parent = AbstractIllagerEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段（m_spellTicks/m_activeSpell 用普通成员承载、不同步），
    // classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    [[nodiscard]] bool isSpellcasting() const noexcept { return m_spellTicks > 0; }
    [[nodiscard]] i32 spellTicks() const noexcept { return m_spellTicks; }
    [[nodiscard]] SpellType spellType() const noexcept { return m_activeSpell; }

    void setSpellType(SpellType spellType) noexcept { m_activeSpell = spellType; }
    void setSpellTicks(i32 ticks) noexcept { m_spellTicks = ticks; }

    void clearSpellcasting() noexcept
    {
        m_spellTicks = 0;
        m_activeSpell = SpellType::None;
    }

    [[nodiscard]] static SpellType spellTypeFromId(i32 id) noexcept;

    void tick() override;

    /**
     * @brief 获取法术类型的粒子颜色（RGB 速度参数）
     *
     * 粒子颜色通过速度参数 (dx, dy, dz) 传递给 ENTITY_EFFECT 粒子
     *
     * @param type 法术类型
     * @return 包含 RGB 颜色值的 Vector3（范围 0.0-1.0）
     */
    [[nodiscard]] static Vector3 getSpellParticleColor(SpellType type) noexcept;

protected:
    [[nodiscard]] virtual const char* getSpellSoundId() const noexcept { return ""; }

private:
    i32 m_spellTicks = 0;
    SpellType m_activeSpell = SpellType::None;
};

} // namespace mc
