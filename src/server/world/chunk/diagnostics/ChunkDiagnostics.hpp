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
#include "server/world/chunk/diagnostics/ChunkDiagnosticSnapshot.hpp"
#include <vector>

namespace mc::server {
class ChunkRegistry;
}

namespace mc::server {
/** @brief 基于注册表保活列表采集诊断；不参与生命周期推进。 */
class ChunkDiagnostics {
public:
    /** @brief 调用方须暂停生成写入后采集 primer 内存信息。 */
    static std::vector<ChunkDiagnosticSnapshot> collect(const ChunkRegistry& registry);
    /** @brief 记录尚未完成目标或不能退场的实例。 */
    static void dump(const ChunkRegistry& registry);
};
} // namespace mc::server
