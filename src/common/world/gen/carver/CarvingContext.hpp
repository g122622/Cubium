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

#include "common/core/Types.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"

namespace mc {

// 前向声明
class BlockState;
namespace world::gen::aquifer {
class Aquifer;
}

// ============================================================================
// CarvingContext — 雕刻上下文
// ============================================================================

/**
 * @brief 雕刻上下文（MC 1.21 CarvingContext）
 *
 * 提供雕刻器执行时所需的世界生成信息，继承自 WorldGenerationContext
 * 并添加含水层引用。
 *
 * 雕刻器通过此上下文访问含水层系统，以确定雕刻后应填充的方块
 * （空气、水或熔岩），替代旧的硬编码 Y < 11 填充熔岩逻辑。
 */
class CarvingContext : public world::gen::valueprovider::WorldGenerationContext {
public:
    /**
     * @brief 构造雕刻上下文
     * @param minGenY 最低生成高度
     * @param genDepth 生成深度
     * @param aquifer 含水层采样器引用（可为 nullptr 表示禁用含水层）
     */
    CarvingContext(i32 minGenY, i32 genDepth, world::gen::aquifer::Aquifer* aquifer)
        : WorldGenerationContext(minGenY, genDepth)
        , m_aquifer(aquifer)
    {}

    /**
     * @brief 获取含水层采样器
     * @return 含水层指针，如果含水层被禁用则为 nullptr
     */
    [[nodiscard]] world::gen::aquifer::Aquifer* aquifer() { return m_aquifer; }
    [[nodiscard]] const world::gen::aquifer::Aquifer* aquifer() const { return m_aquifer; }

    /**
     * @brief 含水层是否可用
     */
    [[nodiscard]] bool hasAquifer() const { return m_aquifer != nullptr; }

private:
    world::gen::aquifer::Aquifer* m_aquifer;
};

} // namespace mc
