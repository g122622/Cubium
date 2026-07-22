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

#include "client/renderer/MeshTypes.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/base/ChunkId.hpp"

namespace mc::client {

/**
 * @brief 网格执行结果
 *
 * chunkmesh 执行层（MeshBuildTask）产出，经 MeshResultQueue 回传主线程。
 * 迁移到 UniversalWorkerPool 后不再携带 workerId（单桶 MeshDataPool 回收，
 * 不需要按 worker 分桶路由）。
 */
struct MeshWorkerResult {
    ChunkId chunkId;
    u64 taskId = 0;
    MeshData solidMesh;
    MeshData transparentMesh;
    bool success = false;
    bool cancelled = false;
};

} // namespace mc::client
