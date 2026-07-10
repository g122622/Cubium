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

// ChunkMeshBenchmark.cpp - 暂时禁用
//
// 此 benchmark 需要 ChunkMesher，而 ChunkMesher 依赖完整的客户端资源系统：
// - BlockModelCache（需要资源管理器）
// - MeshData（客户端模块）
// - 生物群系颜色混合（客户端模块）
//
// 要启用此 benchmark，需要：
// 1. 创建独立的 benchmark 目标链接客户端模块
// 2. 或创建最小化的 ChunkMesher 测试路径
//
// 当前保留此文件用于文档目的，未来可启用。

#if 0 // 禁用编译

#include "../BenchmarkRegistry.hpp"

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

using namespace mc::trace;

namespace mc::benchmark {
namespace {

class ChunkMeshBenchmark final : public IBenchmarkCase {
public:
    [[nodiscard]] std::string name() const override { return "chunk_mesh"; }

    [[nodiscard]] Result<void> validateConfig(const CaseRuntimeConfig& config) const override
    {
        if (!config.parameters.contains("fillBlock") || !config.parameters.at("fillBlock").is_string()) {
            return Error(ErrorCode::InvalidArgument, "chunk_mesh requires string parameter: fillBlock");
        }
        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> setUp(const CaseRuntimeConfig&) override
    {
        VanillaBlocks::initialize();
        m_chunk = std::make_unique<ChunkData>(0, 0);

        for (i32 y = 0; y < 16; ++y) {
            for (i32 z = 0; z < 16; ++z) {
                for (i32 x = 0; x < 16; ++x) {
                    m_chunk->setBlockState(x, 64 + y, z, &VanillaBlocks::STONE->defaultState());
                }
            }
        }

        ChunkMesher::setLightingEnabled(false);
        ChunkMesher::setGreedyMeshing(true);
        ChunkMesher::setModelCache(nullptr);
        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> runOnce() override
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run, "ChunkMeshBenchmark::runOnce");
        MeshData mesh;
        const ChunkData* neighbors[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        ChunkMesher::generateMesh(*m_chunk, mesh, neighbors, nullptr);
        return Result<void>::ok();
    }

    void tearDown() override { m_chunk.reset(); }

private:
    std::unique_ptr<ChunkData> m_chunk;
};

const bool g_registered = []() {
    BenchmarkRegistry::instance().registerCase("chunk_mesh", []() {
        return std::make_unique<ChunkMeshBenchmark>();
    });
    return true;
}();

} // namespace
} // namespace mc::benchmark

#endif // 禁用编译
