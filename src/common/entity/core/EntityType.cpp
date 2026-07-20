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

#include "EntityType.hpp"
#include "Entity.hpp"
#include "common/entity/tag/EntityTypeTag.hpp"

namespace mc {
namespace entity {

// 未知/空实体类型哨兵：默认构造（m_factory=nullptr, m_name=""），
// 替代旧的数字哨兵，用于测试与默认值场景。
const EntityType EntityType::UNKNOWN{};

EntityType::~EntityType() = default;

std::unique_ptr<Entity> EntityType::create(IWorld* world) const
{
    if (!m_factory) {
        return nullptr;
    }

    auto entity = m_factory(world);
    if (entity) {
        entity->setTypeId(m_name);
    }
    return entity;
}

bool EntityType::isIn(const EntityTypeTag& tag) const
{
    return tag.contains(*this);
}

} // namespace entity
} // namespace mc
