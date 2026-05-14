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

#include "EntitySenses.hpp"

#include "../core/Entity.hpp"
#include "../core/MobEntity.hpp"

#include <algorithm>

namespace mc::entity::ai {

EntitySenses::EntitySenses(MobEntity* mob)
    : m_mob(mob)
{}

void EntitySenses::tick()
{
    m_seenEntities.clear();
    m_unseenEntities.clear();
}

bool EntitySenses::canSee(const Entity& entity)
{
    if (std::find(m_seenEntities.begin(), m_seenEntities.end(), &entity) != m_seenEntities.end()) {
        return true;
    }

    if (std::find(m_unseenEntities.begin(), m_unseenEntities.end(), &entity) != m_unseenEntities.end()) {
        return false;
    }

    const bool visible = m_mob->canSee(entity);
    if (visible) {
        m_seenEntities.push_back(&entity);
    } else {
        m_unseenEntities.push_back(&entity);
    }

    return visible;
}

} // namespace mc::entity::ai
