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

#include "DamageSource.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/util/math/Vector3.hpp"
#include <optional>

namespace mc {

// DamageSource::sourcePosition() 默认实现在头文件中（返回 nullopt）。
// 此处仅实现需要 Entity::position() 的实体来源子类，避免在 DamageSource.hpp
// 中引入完整 Entity 定义造成循环包含。

std::optional<math::Vector3f> EntityDamageSource::sourcePosition() const
{
    return (m_source != nullptr) ? std::optional<math::Vector3f>{m_source->position()} : std::nullopt;
}

std::optional<math::Vector3f> IndirectEntityDamageSource::sourcePosition() const
{
    // DamageSource.getSourcePosition：优先 directEntity.position()。
    return (m_directSource != nullptr) ? std::optional<math::Vector3f>{m_directSource->position()} : std::nullopt;
}

} // namespace mc
