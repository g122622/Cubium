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
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <string>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief Jigsaw 连接点信息（用于结构模板）
 */
struct TemplateJigsawBlockInfo {
    BlockPos pos;
    std::string name;
    std::string targetPool;
    std::string targetName;
    i32 jointType = 0;    // 0=rollable, 1=aligned
    u32 blockStateId = 0; // 方块状态ID，用于读取 orientation 属性

    TemplateJigsawBlockInfo() = default;
    TemplateJigsawBlockInfo(const BlockPos& p,
        const std::string& n,
        const std::string& pool,
        const std::string& tgt,
        i32 joint = 0,
        u32 stateId = 0)
        : pos(p)
        , name(n)
        , targetPool(pool)
        , targetName(tgt)
        , jointType(joint)
        , blockStateId(stateId)
    {}
};

/**
 * @brief 实体信息
 * 包含两个位置：
 * - pos: 精确位置（Double 列表），用于实体精确放置
 * - blockPos: 方块坐标（Int 列表），用于方块对齐
 */
struct TemplateEntityInfo {
    std::string typeId;
    f64 posx = 0.0;    // 精确位置 X
    f64 posy = 0.0;    // 精确位置 Y
    f64 posz = 0.0;    // 精确位置 Z
    BlockPos blockPos; // 方块坐标
    std::unique_ptr<nbt::CompoundTag> nbt;

    TemplateEntityInfo();
    TemplateEntityInfo(const TemplateEntityInfo& other);
    TemplateEntityInfo(TemplateEntityInfo&& other) noexcept;
    TemplateEntityInfo& operator=(const TemplateEntityInfo& other);
    TemplateEntityInfo& operator=(TemplateEntityInfo&& other) noexcept;
    ~TemplateEntityInfo();
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
