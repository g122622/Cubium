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

// 在macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include <gtest/gtest.h>

#include <glm/gtc/matrix_transform.hpp>

#include "client/world/ClientWorld.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

#undef BYTE_SIZE // Re-undef after includes which may re-define BYTE_SIZE

using namespace mc;
using namespace mc::client;

namespace {

MeshSchedulerConfig createSchedulerConfig()
{
    MeshSchedulerConfig config;
    config.maxDispatchedTaskCount = 1;
    config.reprioritizeIntervalFrames = 1;
    config.cameraMoveThreshold = 0.0f;
    config.cameraDirectionDotThreshold = 1.0f;
    config.behindCancelDotThreshold = -0.4f;
    config.behindCancelDistanceChunks = 0.0f;
    return config;
}

// 构造能看到区块 (0,0) 的最小视图状态。onChunkData 异步反序列化后需 update() drain 续延队列
// 才会调度网格任务（_scheduleChunkMeshRebuild），update() 内部还会按 viewState 做可见性剔除。
MeshSchedulerViewState createViewState(i32 renderDistanceChunks)
{
    MeshSchedulerViewState viewState;
    viewState.cameraPosition = glm::vec3(8.0f, 64.0f, 8.0f);
    viewState.cameraForward = glm::vec3(0.0f, 0.0f, 1.0f);

    const glm::mat4 projection = glm::perspective(glm::radians(70.0f), 1.0f, 0.1f, 1024.0f);
    const glm::mat4 view = glm::lookAt(
        viewState.cameraPosition, viewState.cameraPosition + viewState.cameraForward, glm::vec3(0.0f, 1.0f, 0.0f));

    viewState.viewProjectionMatrix = projection * view;
    viewState.renderDistanceChunks = renderDistanceChunks;
    viewState.minBuildHeight = world::MIN_BUILD_HEIGHT;
    viewState.maxBuildHeight = world::MAX_BUILD_HEIGHT;
    return viewState;
}

std::vector<u8> createSerializedChunkData()
{
    ChunkData chunkData(0, 0);
    const u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    chunkData.fill(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT, stoneStateId);
    chunkData.setFullyGenerated(true);
    chunkData.setLoaded(true);

    auto serialized = network::ChunkSerializer::serializeChunk(chunkData);
    EXPECT_TRUE(serialized.success());
    if (!serialized.success()) {
        return {};
    }

    return serialized.value();
}

std::vector<u8> createLightData(u8 value)
{
    return std::vector<u8>(NibbleArray::BYTE_SIZE, value);
}

} // namespace

class ClientWorldLightUpdateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        m_pool.start();
    }

    void TearDown() override { m_pool.shutdown(); }

    util::UniversalWorkerPool m_pool{1, "TestCompute", 900};
};

TEST_F(ClientWorldLightUpdateTest, LightUpdateBurstDoesNotResubmitPendingChunkMesh)
{
    ClientWorld world;
    ASSERT_TRUE(world.initialize(12345).success());

    auto dataPool = std::make_shared<MeshDataPool>();
    auto resultQueue = std::make_shared<MeshResultQueue>();
    world.initializeMeshSystem(m_pool, dataPool, resultQueue, createSchedulerConfig());

    std::vector<u8> chunkBytes = createSerializedChunkData();
    ASSERT_FALSE(chunkBytes.empty());

    // onChunkData 在异步路径下只把反序列化任务投递到 worker 池，不直接调度网格任务。
    world.onChunkData(0, 0, 0, std::move(chunkBytes));

    // 等 worker 完成反序列化（结果进 m_pendingDeserializedChunks 续延队列），
    // 再 update() drain → _applyChunkData → _scheduleChunkMeshRebuild 真正提交网格任务。
    m_pool.waitForCompletion();
    world.update(createViewState(8));

    const MeshBuildScheduler* scheduler = world.meshBuildScheduler();
    ASSERT_NE(scheduler, nullptr);

    const MeshSchedulerStats initialStats = scheduler->stats();
    ASSERT_EQ(initialStats.submittedTaskCount, 1u);
    ASSERT_EQ(initialStats.trackedTaskCount, 1u);

    ClientChunk* chunk = world.getChunk(ChunkId(0, 0, 0));
    ASSERT_NE(chunk, nullptr);

    const u64 initialTaskId = chunk->activeMeshTaskId;
    ASSERT_NE(initialTaskId, 0u);

    const std::vector<u8> skyLight = createLightData(0xFF);
    const std::vector<u8> blockLight = createLightData(0x00);

    // 连续 8 次光照更新应走 _requestChunkMeshRebuild：发现 activeMeshTaskId != 0 即早退，
    // 不得重复提交网格任务（meshRebuildPending 仅置标记，等当前任务完成后由调度器复跑）。
    // onLightSection 按 lightType 单独下发：sky 与 block 各调一次（1.21.11 ClientboundLightUpdatePacket
    // 的 BitSet mask 逐层处理）。
    for (i32 i = 0; i < 8; ++i) {
        world.onLightSection(0, 0, 0, true, skyLight);
        world.onLightSection(0, 0, 0, false, blockLight);
    }

    const MeshSchedulerStats afterStats = scheduler->stats();
    EXPECT_EQ(afterStats.submittedTaskCount, initialStats.submittedTaskCount);
    EXPECT_EQ(afterStats.trackedTaskCount, initialStats.trackedTaskCount);
    EXPECT_EQ(world.getChunk(ChunkId(0, 0, 0))->activeMeshTaskId, initialTaskId);

    world.shutdownMeshSystem();
}
#pragma pop_macro("BYTE_SIZE")
