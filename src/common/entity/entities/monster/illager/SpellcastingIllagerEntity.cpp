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

#include "SpellcastingIllagerEntity.hpp"

namespace mc {

SpellcastingIllagerEntity::SpellcastingIllagerEntity(LegacyEntityType type, EntityId id)
    : AbstractIllagerEntity(type, id)
{}

SpellcastingIllagerEntity::SpellType SpellcastingIllagerEntity::spellTypeFromId(i32 id)
{
    switch (id) {
        case 1:
            return SpellType::SummonVex;
        case 2:
            return SpellType::Fangs;
        case 3:
            return SpellType::Wololo;
        case 4:
            return SpellType::Disappear;
        case 5:
            return SpellType::Blindness;
        default:
            return SpellType::None;
    }
}

void SpellcastingIllagerEntity::tick()
{
    AbstractIllagerEntity::tick();

    if (m_spellTicks > 0) {
        --m_spellTicks;
    }

    // TODO: 接入客户端粒子与同步后，补齐施法粒子颜色反馈
}

} // namespace mc
