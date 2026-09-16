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

// ============================================================================
// 注意：本文件不在 CMakeLists.txt 编译列表中，仅供参考。
// 实际实现在 Template.hpp/cpp 中。
// 修改逻辑时，务必修改 Template.hpp/cpp，而非仅修改本文件。
// ============================================================================

#include "PlacementSettings.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

// ============================================================================
// PlacementSettings
// ============================================================================

PlacementSettings::PlacementSettings()
    : m_rotation(Rotation::None)
    , m_mirror(Mirror::None)
    , m_boundingBox(nullptr)
    , m_centerOffset(0, 0, 0)
    , m_blockUpdateFlags(18)
{}

PlacementSettings& PlacementSettings::setRotation(Rotation rotation)
{
    m_rotation = rotation;
    return *this;
}

PlacementSettings& PlacementSettings::setMirror(Mirror mirror)
{
    m_mirror = mirror;
    return *this;
}

PlacementSettings& PlacementSettings::setIgnoreEntities(bool ignore)
{
    m_ignoreEntities = ignore;
    return *this;
}

PlacementSettings& PlacementSettings::setBoundingBox(const structure::StructureBoundingBox* bounds)
{
    m_boundingBox = bounds;
    return *this;
}

PlacementSettings& PlacementSettings::setCenterOffset(const BlockPos& offset)
{
    m_centerOffset = offset;
    return *this;
}

PlacementSettings& PlacementSettings::setBlockUpdateFlags(u32 flags)
{
    m_blockUpdateFlags = flags;
    return *this;
}

PlacementSettings& PlacementSettings::setKeepLiquids(bool keep)
{
    m_keepLiquids = keep;
    return *this;
}

math::Random PlacementSettings::getRandom(const BlockPos& pos) const
{
    // 如果设置了预设随机数，则返回副本；否则基于位置种子创建
    if (m_random) {
        return *m_random;
    }
    // 使用位置种子创建确定性随机数
    return math::Random(math::getPositionRandom(pos.x, pos.y, pos.z));
}

PlacementSettings PlacementSettings::copy() const
{
    PlacementSettings result;
    result.m_rotation = m_rotation;
    result.m_mirror = m_mirror;
    result.m_ignoreEntities = m_ignoreEntities;
    result.m_keepLiquids = m_keepLiquids;
    result.m_boundingBox = m_boundingBox;
    result.m_centerOffset = m_centerOffset;
    result.m_blockUpdateFlags = m_blockUpdateFlags;
    result.m_processors = m_processors;
    result.m_world = m_world;
    result.m_random = m_random;
    return result;
}

PlacementSettings& PlacementSettings::setProcessors(const StructureProcessorList* processors)
{
    m_processors = processors;
    return *this;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
