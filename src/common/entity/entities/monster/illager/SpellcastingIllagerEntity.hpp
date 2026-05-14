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

namespace mc {

/**
 * @brief 施法型灾厄村民公共基类
 *
 * 对齐 1.16.5 `SpellcastingIllagerEntity` 的最小公共层：
 * - 当前激活法术类型
 * - 法术持续 tick
 * - 服务端施法状态判定
 */
class SpellcastingIllagerEntity : public AbstractIllagerEntity {
public:
    enum class SpellType : u8 { None = 0, SummonVex = 1, Fangs = 2, Wololo = 3, Disappear = 4, Blindness = 5 };

    SpellcastingIllagerEntity(LegacyEntityType type, EntityId id);
    ~SpellcastingIllagerEntity() override = default;

    [[nodiscard]] bool isSpellcasting() const { return m_spellTicks > 0; }
    [[nodiscard]] i32 spellTicks() const { return m_spellTicks; }
    [[nodiscard]] SpellType spellType() const { return m_activeSpell; }

    void setSpellType(SpellType spellType) { m_activeSpell = spellType; }
    void setSpellTicks(i32 ticks) { m_spellTicks = ticks; }

    void clearSpellcasting()
    {
        m_spellTicks = 0;
        m_activeSpell = SpellType::None;
    }

    [[nodiscard]] static SpellType spellTypeFromId(i32 id);

    void tick() override;

protected:
    [[nodiscard]] virtual const char* getSpellSoundId() const { return ""; }

private:
    i32 m_spellTicks = 0;
    SpellType m_activeSpell = SpellType::None;
};

} // namespace mc
