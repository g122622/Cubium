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

#pragma once

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"

namespace mc {

class IWorld;

namespace world {
namespace gen {
namespace feature {
namespace template_ {

class StructureProcessorList;

// 使用 Direction.hpp 中定义的 Rotation 和 Mirror 枚举
using mc::Mirror;
using mc::Rotation;

/**
 * @brief 放置设置
 */
class PlacementSettings {
public:
    PlacementSettings();

    [[nodiscard]] Rotation getRotation() const { return m_rotation; }
    PlacementSettings& setRotation(Rotation rotation);

    [[nodiscard]] Mirror getMirror() const { return m_mirror; }
    PlacementSettings& setMirror(Mirror mirror);

    [[nodiscard]] bool ignoreEntities() const { return m_ignoreEntities; }
    PlacementSettings& setIgnoreEntities(bool ignore);

    [[nodiscard]] bool keepLiquids() const { return m_keepLiquids; }
    PlacementSettings& setKeepLiquids(bool keep);

    [[nodiscard]] const structure::StructureBoundingBox* getBoundingBox() const { return m_boundingBox; }
    PlacementSettings& setBoundingBox(const structure::StructureBoundingBox* bounds);

    [[nodiscard]] const BlockPos& getCenterOffset() const { return m_centerOffset; }
    PlacementSettings& setCenterOffset(const BlockPos& offset);

    [[nodiscard]] u32 getBlockUpdateFlags() const { return m_blockUpdateFlags; }
    PlacementSettings& setBlockUpdateFlags(u32 flags);

    [[nodiscard]] const StructureProcessorList* getProcessors() const { return m_processors; }
    PlacementSettings& setProcessors(const StructureProcessorList* processors);

    /**
     * @brief 获取世界读取器（用于 GravityStructureProcessor 等需要高度信息的处理器）
     */
    [[nodiscard]] const IWorld* getWorld() const { return m_world; }
    PlacementSettings& setWorld(const IWorld* world)
    {
        m_world = world;
        return *this;
    }

    /**
     * @brief 获取确定性随机数生成器
     * 如果设置了预设随机数，则返回副本；否则基于位置种子创建
     *
     * @param pos 位置种子
     * @return 随机数生成器
     */
    [[nodiscard]] math::Random getRandom(const BlockPos& pos) const;

    /**
     * @brief 设置预设随机数生成器
     *
     * 当需要固定随机序列时使用
     */
    PlacementSettings& setRandom(math::Random* random)
    {
        m_random = random;
        return *this;
    }

    [[nodiscard]] PlacementSettings copy() const;

private:
    Rotation m_rotation = Rotation::None;
    Mirror m_mirror = Mirror::None;
    bool m_ignoreEntities = false;
    bool m_keepLiquids = false;
    const structure::StructureBoundingBox* m_boundingBox = nullptr;
    BlockPos m_centerOffset = BlockPos(0, 0, 0);
    u32 m_blockUpdateFlags = 18;
    const StructureProcessorList* m_processors = nullptr;
    const IWorld* m_world = nullptr;
    math::Random* m_random = nullptr;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
