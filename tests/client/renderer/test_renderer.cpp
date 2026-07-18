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

#include <cmath>
#include <limits>
#include <sstream>
#include <gtest/gtest.h>

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "client/resource/BlockModelCache.hpp"
#include "client/resource/ResourceManager.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace mc;

namespace {

class InMemoryResourcePack final : public IResourcePack {
public:
    Result<void> initialize() override { return Result<void>::ok(); }

    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }

    [[nodiscard]] bool hasResource(resource::PackType type, std::string_view resourcePath) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string path(resourcePath);
        bool hasTypePrefix = path.size() > typeDir.size() && path.substr(0, typeDir.size() + 1) == typeDir + "/";
        std::string full;
        if (hasTypePrefix) {
            full = path;
        } else {
            full = typeDir + "/" + path;
        }
        return m_resources.find(full) != m_resources.end();
    }

    [[nodiscard]] Result<std::vector<u8>> readResource(
        resource::PackType type, std::string_view resourcePath) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string path(resourcePath);
        bool hasTypePrefix = path.size() > typeDir.size() && path.substr(0, typeDir.size() + 1) == typeDir + "/";
        std::string full;
        if (hasTypePrefix) {
            full = path;
        } else {
            full = typeDir + "/" + path;
        }
        const auto it = m_resources.find(full);
        if (it == m_resources.end()) {
            return Error(ErrorCode::NotFound, "Resource not found");
        }
        return it->second;
    }

    [[nodiscard]] Result<std::vector<std::string>> listResources(
        resource::PackType type, std::string_view directory, std::string_view extension) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string fullDirectory = typeDir + "/" + std::string(directory);
        std::vector<std::string> result;
        const std::string ext(extension);
        for (const auto& [path, _] : m_resources) {
            const bool inDir = fullDirectory.empty() || path.rfind(fullDirectory, 0) == 0;
            const bool extMatch =
                ext.empty() || (path.size() >= ext.size() && path.substr(path.size() - ext.size()) == ext);
            if (inDir && extMatch) {
                result.push_back(path);
            }
        }
        return result;
    }

    [[nodiscard]] Result<std::vector<std::string>> getResourceNamespaces(resource::PackType type) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string prefix = typeDir + "/";
        std::unordered_set<std::string> namespaces;
        for (const auto& [path, _] : m_resources) {
            if (path.size() > prefix.size() && path.substr(0, prefix.size()) == prefix) {
                std::string rest = path.substr(prefix.size());
                size_t slashPos = rest.find('/');
                if (slashPos != std::string::npos) {
                    namespaces.insert(rest.substr(0, slashPos));
                }
            }
        }
        std::vector<std::string> result(namespaces.begin(), namespaces.end());
        std::sort(result.begin(), result.end());
        return result;
    }

    [[nodiscard]] std::string name() const override { return "InMemoryResourcePack"; }

    void add(std::string path, std::vector<u8> bytes) { m_resources.emplace(std::move(path), std::move(bytes)); }

private:
    PackMetadata m_metadata{6, "test-pack"};
    std::unordered_map<std::string, std::vector<u8>> m_resources;
};

std::vector<u8> makeValid1x1Png()
{
    return {137,
        80,
        78,
        71,
        13,
        10,
        26,
        10,
        0,
        0,
        0,
        13,
        73,
        72,
        68,
        82,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        1,
        8,
        4,
        0,
        0,
        0,
        181,
        28,
        12,
        2,
        0,
        0,
        0,
        11,
        73,
        68,
        65,
        84,
        120,
        218,
        99,
        252,
        255,
        31,
        0,
        3,
        3,
        2,
        0,
        239,
        156,
        7,
        219,
        0,
        0,
        0,
        0,
        73,
        69,
        78,
        68,
        174,
        66,
        96,
        130};
}

std::vector<u8> toBytes(std::string_view content);

void addEmptyPlantModel(InMemoryResourcePack& pack, std::string_view blockName)
{
    const std::string name(blockName);

    std::ostringstream blockStateJson;
    blockStateJson << "{\n"
                   << "    \"variants\": {\n"
                   << "        \"\": { \"model\": \"minecraft:block/" << name << "\" }\n"
                   << "    }\n"
                   << "}";

    std::ostringstream modelJson;
    modelJson << "{\n"
              << "    \"textures\": {\n"
              << "        \"particle\": \"block/water_still\"\n"
              << "    }\n"
              << "}";

    const std::string blockStatePath = std::string("assets/minecraft/blockstates/") + name + ".json";
    const std::string modelPath = std::string("assets/minecraft/models/block/") + name + ".json";
    pack.add(blockStatePath, toBytes(blockStateJson.str()));
    pack.add(modelPath, toBytes(modelJson.str()));
}

std::unique_ptr<InMemoryResourcePack> createLiquidResourcePack()
{
    auto pack = std::make_unique<InMemoryResourcePack>();
    pack->add("assets/minecraft/blockstates/water.json", toBytes(R"({
    "variants": {
        "": { "model": "minecraft:block/water" }
    }
})"));
    pack->add("assets/minecraft/models/block/water.json", toBytes(R"({
    "textures": {
        "particle": "block/water_still"
    }
})"));
    pack->add("assets/minecraft/textures/block/water_still.png", makeValid1x1Png());
    addEmptyPlantModel(*pack, "kelp_plant");
    addEmptyPlantModel(*pack, "seagrass");
    return pack;
}

std::vector<u8> toBytes(std::string_view content)
{
    return std::vector<u8>(content.begin(), content.end());
}

// 测试用纹理区域查询回调：生产环境由 AtlasManager 的 blocks atlas 提供，
// 此处无 Vulkan 设备，无法构造 AtlasManager。ChunkMesher 的液体面生成只需
// 查询回调返回非空 TextureRegion 即可产生顶点（测试只校验拓扑/索引数，
// 不关心具体 UV），故返回一个静态占位区域。
std::function<const TextureRegion*(const ResourceLocation&)> makeTestRegionLookup()
{
    static const TextureRegion placeholderRegion(0.0, 0.0, 1.0, 1.0);
    return [](const ResourceLocation&) -> const TextureRegion* { return &placeholderRegion; };
}

struct MeshBounds {
    f32 minX = std::numeric_limits<f32>::max();
    f32 minY = std::numeric_limits<f32>::max();
    f32 minZ = std::numeric_limits<f32>::max();
    f32 maxX = std::numeric_limits<f32>::lowest();
    f32 maxY = std::numeric_limits<f32>::lowest();
    f32 maxZ = std::numeric_limits<f32>::lowest();
};

MeshBounds computeBounds(const MeshData& mesh)
{
    MeshBounds bounds;
    for (const auto& v : mesh.vertices) {
        const f32 x = static_cast<f32>(v.x);
        const f32 y = static_cast<f32>(v.y);
        const f32 z = static_cast<f32>(v.z);
        bounds.minX = std::min(bounds.minX, x);
        bounds.minY = std::min(bounds.minY, y);
        bounds.minZ = std::min(bounds.minZ, z);
        bounds.maxX = std::max(bounds.maxX, x);
        bounds.maxY = std::max(bounds.maxY, y);
        bounds.maxZ = std::max(bounds.maxZ, z);
    }
    return bounds;
}

} // namespace

// ============================================================================
// BlockGeometry 测试
// ============================================================================

TEST(BlockGeometry, FaceNormals)
{
    auto bottom = BlockGeometry::getFaceNormal(Face::Bottom);
    EXPECT_DOUBLE_EQ(bottom[0], 0.0);
    EXPECT_DOUBLE_EQ(bottom[1], -1.0);
    EXPECT_DOUBLE_EQ(bottom[2], 0.0);

    auto top = BlockGeometry::getFaceNormal(Face::Top);
    EXPECT_DOUBLE_EQ(top[0], 0.0);
    EXPECT_DOUBLE_EQ(top[1], 1.0);
    EXPECT_DOUBLE_EQ(top[2], 0.0);

    auto north = BlockGeometry::getFaceNormal(Face::North);
    EXPECT_DOUBLE_EQ(north[0], 0.0);
    EXPECT_DOUBLE_EQ(north[1], 0.0);
    EXPECT_DOUBLE_EQ(north[2], -1.0);

    auto south = BlockGeometry::getFaceNormal(Face::South);
    EXPECT_DOUBLE_EQ(south[0], 0.0);
    EXPECT_DOUBLE_EQ(south[1], 0.0);
    EXPECT_DOUBLE_EQ(south[2], 1.0);

    auto west = BlockGeometry::getFaceNormal(Face::West);
    EXPECT_DOUBLE_EQ(west[0], -1.0);
    EXPECT_DOUBLE_EQ(west[1], 0.0);
    EXPECT_DOUBLE_EQ(west[2], 0.0);

    auto east = BlockGeometry::getFaceNormal(Face::East);
    EXPECT_DOUBLE_EQ(east[0], 1.0);
    EXPECT_DOUBLE_EQ(east[1], 0.0);
    EXPECT_DOUBLE_EQ(east[2], 0.0);
}

TEST(BlockGeometry, FaceVertices)
{
    // 顶部面应该有4个顶点，每个顶点3个分量
    auto topVerts = BlockGeometry::getFaceVertices(Face::Top);
    EXPECT_EQ(topVerts.size(), 12u); // 4顶点 * 3分量

    // 顶部面Y坐标应该是1
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_DOUBLE_EQ(topVerts[i * 3 + 1], 1.0); // Y坐标
    }

    // 底部面Y坐标应该是0
    auto bottomVerts = BlockGeometry::getFaceVertices(Face::Bottom);
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_DOUBLE_EQ(bottomVerts[i * 3 + 1], 0.0); // Y坐标
    }
}

TEST(BlockGeometry, FaceIndices)
{
    auto indices = BlockGeometry::getFaceIndices();
    EXPECT_EQ(indices.size(), 6u); // 2三角形 * 3索引

    // 索引应该在0-3范围内
    for (u32 idx : indices) {
        EXPECT_LT(idx, 4u);
    }

    // 第一个三角形
    EXPECT_EQ(indices[0], 0u);
    EXPECT_EQ(indices[1], 1u);
    EXPECT_EQ(indices[2], 2u);

    // 第二个三角形
    EXPECT_EQ(indices[3], 0u);
    EXPECT_EQ(indices[4], 2u);
    EXPECT_EQ(indices[5], 3u);
}

TEST(BlockGeometry, FaceDirection)
{
    auto bottomDir = BlockGeometry::getFaceDirection(Face::Bottom);
    EXPECT_EQ(bottomDir[0], 0);
    EXPECT_EQ(bottomDir[1], -1);
    EXPECT_EQ(bottomDir[2], 0);

    auto topDir = BlockGeometry::getFaceDirection(Face::Top);
    EXPECT_EQ(topDir[1], 1);

    auto northDir = BlockGeometry::getFaceDirection(Face::North);
    EXPECT_EQ(northDir[2], -1);

    auto southDir = BlockGeometry::getFaceDirection(Face::South);
    EXPECT_EQ(southDir[2], 1);

    auto westDir = BlockGeometry::getFaceDirection(Face::West);
    EXPECT_EQ(westDir[0], -1);

    auto eastDir = BlockGeometry::getFaceDirection(Face::East);
    EXPECT_EQ(eastDir[0], 1);
}

TEST(BlockGeometry, ShouldRenderFace)
{
    // 如果邻居不透明，不渲染面
    EXPECT_FALSE(BlockGeometry::shouldRenderFace(Face::Top, true));
    EXPECT_TRUE(BlockGeometry::shouldRenderFace(Face::Top, false));
}

// ============================================================================
// TextureAtlas 测试
// ============================================================================

TEST(TextureAtlas, Construction)
{
    TextureAtlas atlas(256, 256, 16);

    EXPECT_EQ(atlas.textureWidth(), 256u);
    EXPECT_EQ(atlas.textureHeight(), 256u);
    EXPECT_EQ(atlas.tileSize(), 16u);
    EXPECT_EQ(atlas.tilesPerRow(), 16u);
}

TEST(TextureAtlas, GetRegionByCoords)
{
    TextureAtlas atlas(256, 256, 16);

    // 第一个图块 (0,0)
    auto r0 = atlas.getRegion(0, 0);
    EXPECT_DOUBLE_EQ(r0.u0, 0.0);
    EXPECT_DOUBLE_EQ(r0.v0, 0.0);
    EXPECT_DOUBLE_EQ(r0.u1, 1.0 / 16.0);
    EXPECT_DOUBLE_EQ(r0.v1, 1.0 / 16.0);

    // 第一个图块 (1,0) - 第二列
    auto r1 = atlas.getRegion(1, 0);
    EXPECT_DOUBLE_EQ(r1.u0, 1.0 / 16.0);
    EXPECT_DOUBLE_EQ(r1.v0, 0.0);
    EXPECT_DOUBLE_EQ(r1.u1, 2.0 / 16.0);
    EXPECT_DOUBLE_EQ(r1.v1, 1.0 / 16.0);

    // 第一行第二列 (0,1)
    auto r2 = atlas.getRegion(0, 1);
    EXPECT_DOUBLE_EQ(r2.u0, 0.0);
    EXPECT_DOUBLE_EQ(r2.v0, 1.0 / 16.0);
    EXPECT_DOUBLE_EQ(r2.u1, 1.0 / 16.0);
    EXPECT_DOUBLE_EQ(r2.v1, 2.0 / 16.0);
}

TEST(TextureAtlas, GetRegionByIndex)
{
    TextureAtlas atlas(256, 256, 16);

    // 线性索引 0
    auto r0 = atlas.getRegion(0);
    EXPECT_DOUBLE_EQ(r0.u0, 0.0);
    EXPECT_DOUBLE_EQ(r0.v0, 0.0);

    // 线性索引 1
    auto r1 = atlas.getRegion(1);
    EXPECT_DOUBLE_EQ(r1.u0, 1.0 / 16.0);
    EXPECT_DOUBLE_EQ(r1.v0, 0.0);

    // 线性索引 16 (第二行开始)
    auto r16 = atlas.getRegion(16);
    EXPECT_DOUBLE_EQ(r16.u0, 0.0);
    EXPECT_DOUBLE_EQ(r16.v0, 1.0 / 16.0);
}

// ============================================================================
// MeshData 测试
// ============================================================================

TEST(MeshData, Construction)
{
    MeshData mesh;
    EXPECT_TRUE(mesh.empty());
    EXPECT_EQ(mesh.vertexCount(), 0u);
    EXPECT_EQ(mesh.indexCount(), 0u);
}

TEST(MeshData, Reserve)
{
    MeshData mesh;
    mesh.reserve(100, 200);
    // 预留容量，不增加大小
    EXPECT_TRUE(mesh.empty());
}

TEST(MeshData, Clear)
{
    MeshData mesh;
    mesh.vertices.push_back(Vertex());
    mesh.indices.push_back(0);
    EXPECT_FALSE(mesh.empty());

    mesh.clear();
    EXPECT_TRUE(mesh.empty());
}

// ============================================================================
// Vertex 测试
// ============================================================================

TEST(Vertex, Construction)
{
    Vertex v;
    EXPECT_DOUBLE_EQ(v.x, 0.0);
    EXPECT_DOUBLE_EQ(v.y, 0.0);
    EXPECT_DOUBLE_EQ(v.z, 0.0);
}

TEST(Vertex, ParameterizedConstruction)
{
    Vertex v(1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0xFF0000FF, 10);

    EXPECT_DOUBLE_EQ(v.x, 1.0);
    EXPECT_DOUBLE_EQ(v.y, 2.0);
    EXPECT_DOUBLE_EQ(v.z, 3.0);
    EXPECT_DOUBLE_EQ(v.nx, 0.0);
    EXPECT_DOUBLE_EQ(v.ny, 1.0);
    EXPECT_DOUBLE_EQ(v.nz, 0.0);
    EXPECT_DOUBLE_EQ(v.u, 0.5);
    EXPECT_DOUBLE_EQ(v.v, 0.5);
    EXPECT_EQ(v.color, 0xFF0000FFu);
    EXPECT_EQ(v.light, 10u);
}

// ============================================================================
// ChunkRenderData 测试
// ============================================================================

TEST(ChunkRenderData, Construction)
{
    ChunkRenderData data;
    EXPECT_TRUE(data.solidMesh.empty());
    EXPECT_TRUE(data.transparentMesh.empty());
    EXPECT_TRUE(data.needsUpdate);
    EXPECT_FALSE(data.isDirty);
    EXPECT_EQ(data.vertexCount, 0u);
    EXPECT_EQ(data.indexCount, 0u);
}

TEST(ChunkRenderData, MarkDirty)
{
    ChunkRenderData data;
    data.markClean();
    EXPECT_FALSE(data.isDirty);
    EXPECT_FALSE(data.needsUpdate);

    data.markDirty();
    EXPECT_TRUE(data.isDirty);
    EXPECT_TRUE(data.needsUpdate);
}

// ============================================================================
// ChunkMesher 测试
// ============================================================================
// 注意：ChunkMesher 现在需要 BlockModelCache 才能正常工作
// BlockModelCache 需要从 ResourceManager 获取方块外观
// 这些测试在没有设置 BlockModelCache 的情况下会返回空网格

class ChunkMesherTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        // 确保每个测试从一致状态开始，避免静态全局配置在测试间串扰。
        ChunkMesher::setModelCache(nullptr);

        // 创建一个简单的测试区块
        testChunk = std::make_unique<ChunkData>(0, 0);

        // 注意：不再使用 BlockModelRegistry，它已被移除
        // ChunkMesher 现在需要通过 setModelCache() 设置 BlockModelCache
        // 在没有 BlockModelCache 的情况下，ChunkMesher 将不会生成任何面
    }

    std::unique_ptr<ChunkData> testChunk;
};

TEST_F(ChunkMesherTest, GenerateEmptyChunk)
{
    MeshData mesh;
    ChunkMesher::generateMesh(*testChunk, mesh, nullptr, nullptr);

    // 空区块应该产生空网格
    EXPECT_TRUE(mesh.empty());
}

TEST_F(ChunkMesherTest, GenerateSingleBlockWithoutModelCache)
{
    // 放置一个方块
    testChunk->setBlockState(8, 64, 8, &VanillaBlocks::STONE->defaultState());

    MeshData mesh;
    ChunkMesher::generateMesh(*testChunk, mesh, nullptr, nullptr);

    // 在没有 BlockModelCache 的情况下，不会生成任何面
    // 因为 ChunkMesher 需要从 BlockModelCache 获取方块外观
    EXPECT_TRUE(mesh.empty());
}

TEST_F(ChunkMesherTest, SettingsTest)
{
    // 测试设置
    bool originalGreedy = ChunkMesher::isGreedyMeshingEnabled();
    bool originalLighting = ChunkMesher::isLightingEnabled();

    ChunkMesher::setGreedyMeshing(true);
    EXPECT_TRUE(ChunkMesher::isGreedyMeshingEnabled());

    ChunkMesher::setLightingEnabled(false);
    EXPECT_FALSE(ChunkMesher::isLightingEnabled());

    // 恢复原始设置
    ChunkMesher::setGreedyMeshing(originalGreedy);
    ChunkMesher::setLightingEnabled(originalLighting);
}

TEST_F(ChunkMesherTest, SampleCombinedLightUsesFaceAdjacentVoxel)
{
    // 模拟不透明方块内部（0光）与上方空气（15光）的典型地表场景
    testChunk->setSkyLight(8, 64, 8, 0);
    testChunk->setBlockLight(8, 64, 8, 0);
    testChunk->setSkyLight(8, 65, 8, 15);
    testChunk->setBlockLight(8, 65, 8, 0);

    const u8 centerLight = ChunkMesher::sampleCombinedLight(*testChunk, 8, 64, 8, nullptr);
    const u8 topFaceLight = ChunkMesher::sampleCombinedLight(*testChunk, 8, 65, 8, nullptr);

    EXPECT_EQ(centerLight, 0u);
    EXPECT_EQ(topFaceLight, 15u);
}

TEST_F(ChunkMesherTest, SampleCombinedLightReadsNeighborChunkAtBorder)
{
    auto eastNeighbor = std::make_unique<ChunkData>(1, 0);

    // 当前区块边界位置设为暗，邻居边界位置设为亮，验证跨区块采样
    testChunk->setSkyLight(15, 64, 8, 0);
    eastNeighbor->setSkyLight(0, 64, 8, 9);

    const ChunkData* neighbors[6] = {nullptr, eastNeighbor.get(), nullptr, nullptr, nullptr, nullptr};

    const u8 neighborLight = ChunkMesher::sampleCombinedLight(*testChunk, 16, 64, 8, neighbors);
    EXPECT_EQ(neighborLight, 9u);

    const ChunkData* noNeighbors[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    const u8 fallbackLight = ChunkMesher::sampleCombinedLight(*testChunk, 16, 64, 8, noNeighbors);
    EXPECT_EQ(fallbackLight, 15u);
}

TEST_F(ChunkMesherTest, SampleCombinedLight_DiagonalOutOfBounds_UsesAvailableNeighborApproximation)
{
    auto westNeighbor = std::make_unique<ChunkData>(-1, 0);

    // 目标采样点为 (-1, 64, -1)，缺少西北对角区块。
    // 期望：不再回退到固定全亮，而是近似使用可用西侧邻区 (15,64,15) 的值。
    westNeighbor->setSkyLight(15, 64, 15, 7);
    westNeighbor->setBlockLight(15, 64, 15, 0);

    const ChunkData* neighbors[6] = {westNeighbor.get(), nullptr, nullptr, nullptr, nullptr, nullptr};
    const u8 approxLight = ChunkMesher::sampleCombinedLight(*testChunk, -1, 64, -1, neighbors);

    EXPECT_EQ(approxLight, 7u);
}

TEST_F(ChunkMesherTest, ModelCacheIsNullByDefault)
{
    // 默认情况下 BlockModelCache 应该是 nullptr
    EXPECT_EQ(ChunkMesher::modelCache(), nullptr);
}

class ChunkMesherWithModelCacheTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_originalModelCache = ChunkMesher::modelCache();
        m_originalGreedy = ChunkMesher::isGreedyMeshingEnabled();
        m_originalLightingEnabled = ChunkMesher::isLightingEnabled();
        m_originalLightingMode = ChunkMesher::lightingMode();

        m_resourceManager = std::make_unique<ResourceManager>();
        ASSERT_TRUE(m_resourceManager->addResourcePack(createLiquidResourcePack()).success());
        ASSERT_TRUE(m_resourceManager->loadAllResources().success());

        m_regionLookup = makeTestRegionLookup();
        m_resourceManager->computeBlockAppearances(m_regionLookup);

        m_modelCache = std::make_unique<BlockModelCache>();
        ASSERT_TRUE(m_modelCache->initialize(*m_resourceManager));
        m_modelCache->setRegionLookup(m_regionLookup);
        ChunkMesher::setModelCache(m_modelCache.get());

        m_chunk = std::make_unique<ChunkData>(0, 0);
    }

    void TearDown() override
    {
        ChunkMesher::setModelCache(m_originalModelCache);
        ChunkMesher::setGreedyMeshing(m_originalGreedy);
        ChunkMesher::setLightingEnabled(m_originalLightingEnabled);
        ChunkMesher::setLightingMode(m_originalLightingMode);
    }

    std::unique_ptr<ResourceManager> m_resourceManager;
    std::unique_ptr<BlockModelCache> m_modelCache;
    std::function<const TextureRegion*(const ResourceLocation&)> m_regionLookup;
    std::unique_ptr<ChunkData> m_chunk;

    BlockModelCache* m_originalModelCache = nullptr;
    bool m_originalGreedy = false;
    bool m_originalLightingEnabled = true;
    LightingMode m_originalLightingMode = LightingMode::Smooth;
};

TEST_F(ChunkMesherWithModelCacheTest, GreedyMeshing_MergesFlatStoneLayerToSixQuads)
{
    constexpr i32 y = 64;
    for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
        for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
            m_chunk->setBlockState(x, y, z, &VanillaBlocks::STONE->defaultState());
        }
    }

    ChunkMesher::setLightingEnabled(false);
    ChunkMesher::setLightingMode(LightingMode::Flat);

    MeshData simpleMesh;
    ChunkMesher::setGreedyMeshing(false);
    ChunkMesher::generateMesh(*m_chunk, simpleMesh, nullptr, nullptr);
    ASSERT_FALSE(simpleMesh.empty());

    MeshData greedyMesh;
    ChunkMesher::setGreedyMeshing(true);
    ChunkMesher::generateMesh(*m_chunk, greedyMesh, nullptr, nullptr);
    ASSERT_FALSE(greedyMesh.empty());

    EXPECT_LT(greedyMesh.vertexCount(), simpleMesh.vertexCount());
    EXPECT_LT(greedyMesh.indexCount(), simpleMesh.indexCount());
    EXPECT_EQ(greedyMesh.vertexCount(), 24u);
    EXPECT_EQ(greedyMesh.indexCount(), 36u);
}

TEST_F(ChunkMesherWithModelCacheTest, GreedyMeshing_SmoothLightingFallsBackToSimplePath)
{
    m_chunk->setBlockState(8, 64, 8, &VanillaBlocks::STONE->defaultState());

    ChunkMesher::setLightingEnabled(true);
    ChunkMesher::setLightingMode(LightingMode::Smooth);

    MeshData simpleMesh;
    ChunkMesher::setGreedyMeshing(false);
    ChunkMesher::generateMesh(*m_chunk, simpleMesh, nullptr, nullptr);

    MeshData greedyMesh;
    ChunkMesher::setGreedyMeshing(true);
    ChunkMesher::generateMesh(*m_chunk, greedyMesh, nullptr, nullptr);

    EXPECT_EQ(greedyMesh.vertexCount(), simpleMesh.vertexCount());
    EXPECT_EQ(greedyMesh.indexCount(), simpleMesh.indexCount());
}

TEST_F(ChunkMesherWithModelCacheTest, SplitMesh_PlacesWaterIntoTransparentLayer)
{
    auto pack = createLiquidResourcePack();
    ResourceManager resourceManager;
    ASSERT_TRUE(resourceManager.addResourcePack(std::move(pack)).success());
    ASSERT_TRUE(resourceManager.loadAllResources().success());

    auto regionLookup = makeTestRegionLookup();
    resourceManager.computeBlockAppearances(regionLookup);

    BlockModelCache modelCache;
    ASSERT_TRUE(modelCache.initialize(resourceManager));
    modelCache.setRegionLookup(regionLookup);

    BlockModelCache* oldModelCache = ChunkMesher::modelCache();
    const bool oldGreedy = ChunkMesher::isGreedyMeshingEnabled();
    const bool oldLightingEnabled = ChunkMesher::isLightingEnabled();
    const LightingMode oldLightingMode = ChunkMesher::lightingMode();
    const auto restoreGuard =
        std::shared_ptr<void>(nullptr, [oldModelCache, oldGreedy, oldLightingEnabled, oldLightingMode](void*) {
            ChunkMesher::setModelCache(oldModelCache);
            ChunkMesher::setGreedyMeshing(oldGreedy);
            ChunkMesher::setLightingEnabled(oldLightingEnabled);
            ChunkMesher::setLightingMode(oldLightingMode);
        });

    ChunkMesher::setModelCache(&modelCache);
    m_chunk->setBlockState(8, 64, 8, &VanillaBlocks::STONE->defaultState());
    m_chunk->setBlockState(9, 64, 8, &VanillaBlocks::WATER->defaultState());

    MeshData solidMesh;
    MeshData transparentMesh;
    ChunkMesher::setGreedyMeshing(false);
    ChunkMesher::setLightingEnabled(false);
    ChunkMesher::setLightingMode(LightingMode::Flat);
    ChunkMesher::generateSplitMesh(*m_chunk, solidMesh, transparentMesh, nullptr, nullptr);

    EXPECT_FALSE(solidMesh.empty());
    EXPECT_FALSE(transparentMesh.empty());
}

TEST_F(ChunkMesherWithModelCacheTest, SplitMesh_TransparentFaceAgainstOpaqueIsKept)
{
    m_chunk->setBlockState(8, 64, 8, &VanillaBlocks::GLASS->defaultState());
    m_chunk->setBlockState(9, 64, 8, &VanillaBlocks::STONE->defaultState());

    MeshData solidMesh;
    MeshData transparentMesh;
    ChunkMesher::setGreedyMeshing(false);
    ChunkMesher::setLightingEnabled(false);
    ChunkMesher::generateSplitMesh(*m_chunk, solidMesh, transparentMesh, nullptr, nullptr);

    // 玻璃单方块应保留 6 个面（含与石头交界面）= 36 索引。
    EXPECT_EQ(transparentMesh.indexCount(), 36u);
}

TEST_F(ChunkMesherWithModelCacheTest, SplitMesh_WaterUsesTranslucentVertexAlpha)
{
    auto pack = createLiquidResourcePack();
    ResourceManager resourceManager;
    ASSERT_TRUE(resourceManager.addResourcePack(std::move(pack)).success());
    ASSERT_TRUE(resourceManager.loadAllResources().success());

    auto regionLookup = makeTestRegionLookup();
    resourceManager.computeBlockAppearances(regionLookup);

    BlockModelCache modelCache;
    ASSERT_TRUE(modelCache.initialize(resourceManager));
    modelCache.setRegionLookup(regionLookup);

    BlockModelCache* oldModelCache = ChunkMesher::modelCache();
    const bool oldGreedy = ChunkMesher::isGreedyMeshingEnabled();
    const bool oldLightingEnabled = ChunkMesher::isLightingEnabled();
    const LightingMode oldLightingMode = ChunkMesher::lightingMode();
    const auto restoreGuard =
        std::shared_ptr<void>(nullptr, [oldModelCache, oldGreedy, oldLightingEnabled, oldLightingMode](void*) {
            ChunkMesher::setModelCache(oldModelCache);
            ChunkMesher::setGreedyMeshing(oldGreedy);
            ChunkMesher::setLightingEnabled(oldLightingEnabled);
            ChunkMesher::setLightingMode(oldLightingMode);
        });

    ChunkMesher::setModelCache(&modelCache);
    m_chunk->setBlockState(8, 64, 8, &VanillaBlocks::WATER->defaultState());

    MeshData solidMesh;
    MeshData transparentMesh;
    ChunkMesher::setGreedyMeshing(false);
    ChunkMesher::setLightingEnabled(false);
    ChunkMesher::setLightingMode(LightingMode::Flat);
    ChunkMesher::generateSplitMesh(*m_chunk, solidMesh, transparentMesh, nullptr, nullptr);

    ASSERT_FALSE(transparentMesh.empty());

    bool hasExpectedAlphaVertex = false;
    for (const auto& vertex : transparentMesh.vertices) {
        const u8 alpha = static_cast<u8>((vertex.color >> 24) & 0xFFu);
        if (alpha == 180u) {
            hasExpectedAlphaVertex = true;
            break;
        }
    }

    EXPECT_TRUE(hasExpectedAlphaVertex);
}

TEST_F(ChunkMesherWithModelCacheTest, SplitMesh_WaterCullsFaceAgainstSeagrass)
{
    auto pack = createLiquidResourcePack();
    ResourceManager resourceManager;
    ASSERT_TRUE(resourceManager.addResourcePack(std::move(pack)).success());
    ASSERT_TRUE(resourceManager.loadAllResources().success());

    auto regionLookup = makeTestRegionLookup();
    resourceManager.computeBlockAppearances(regionLookup);

    BlockModelCache modelCache;
    ASSERT_TRUE(modelCache.initialize(resourceManager));
    modelCache.setRegionLookup(regionLookup);

    BlockModelCache* oldModelCache = ChunkMesher::modelCache();
    const bool oldGreedy = ChunkMesher::isGreedyMeshingEnabled();
    const bool oldLightingEnabled = ChunkMesher::isLightingEnabled();
    const LightingMode oldLightingMode = ChunkMesher::lightingMode();
    const auto restoreGuard =
        std::shared_ptr<void>(nullptr, [oldModelCache, oldGreedy, oldLightingEnabled, oldLightingMode](void*) {
            ChunkMesher::setModelCache(oldModelCache);
            ChunkMesher::setGreedyMeshing(oldGreedy);
            ChunkMesher::setLightingEnabled(oldLightingEnabled);
            ChunkMesher::setLightingMode(oldLightingMode);
        });

    ChunkMesher::setModelCache(&modelCache);
    m_chunk->setBlockState(8, 64, 8, &VanillaBlocks::WATER->defaultState());
    m_chunk->setBlockState(9, 64, 8, &VanillaBlocks::SEAGRASS->defaultState());

    MeshData solidMesh;
    MeshData transparentMesh;
    ChunkMesher::setGreedyMeshing(false);
    ChunkMesher::setLightingEnabled(false);
    ChunkMesher::setLightingMode(LightingMode::Flat);
    ChunkMesher::generateSplitMesh(*m_chunk, solidMesh, transparentMesh, nullptr, nullptr);

    EXPECT_TRUE(solidMesh.empty());
    EXPECT_EQ(transparentMesh.indexCount(), 30u);
}

TEST_F(ChunkMesherWithModelCacheTest, SplitMesh_WaterCullsFaceAgainstKelpPlant)
{
    auto pack = createLiquidResourcePack();
    ResourceManager resourceManager;
    ASSERT_TRUE(resourceManager.addResourcePack(std::move(pack)).success());
    ASSERT_TRUE(resourceManager.loadAllResources().success());

    auto regionLookup = makeTestRegionLookup();
    resourceManager.computeBlockAppearances(regionLookup);

    BlockModelCache modelCache;
    ASSERT_TRUE(modelCache.initialize(resourceManager));
    modelCache.setRegionLookup(regionLookup);

    BlockModelCache* oldModelCache = ChunkMesher::modelCache();
    const bool oldGreedy = ChunkMesher::isGreedyMeshingEnabled();
    const bool oldLightingEnabled = ChunkMesher::isLightingEnabled();
    const LightingMode oldLightingMode = ChunkMesher::lightingMode();
    const auto restoreGuard =
        std::shared_ptr<void>(nullptr, [oldModelCache, oldGreedy, oldLightingEnabled, oldLightingMode](void*) {
            ChunkMesher::setModelCache(oldModelCache);
            ChunkMesher::setGreedyMeshing(oldGreedy);
            ChunkMesher::setLightingEnabled(oldLightingEnabled);
            ChunkMesher::setLightingMode(oldLightingMode);
        });

    ChunkMesher::setModelCache(&modelCache);
    m_chunk->setBlockState(8, 64, 8, &VanillaBlocks::WATER->defaultState());
    m_chunk->setBlockState(9, 64, 8, &VanillaBlocks::KELP_PLANT->defaultState());

    MeshData solidMesh;
    MeshData transparentMesh;
    ChunkMesher::setGreedyMeshing(false);
    ChunkMesher::setLightingEnabled(false);
    ChunkMesher::setLightingMode(LightingMode::Flat);
    ChunkMesher::generateSplitMesh(*m_chunk, solidMesh, transparentMesh, nullptr, nullptr);

    EXPECT_TRUE(solidMesh.empty());
    EXPECT_EQ(transparentMesh.indexCount(), 30u);
}

TEST_F(ChunkMesherWithModelCacheTest, SplitMesh_ShallowWaterLowersSurfaceHeight)
{
    auto pack = createLiquidResourcePack();
    ResourceManager resourceManager;
    ASSERT_TRUE(resourceManager.addResourcePack(std::move(pack)).success());
    ASSERT_TRUE(resourceManager.loadAllResources().success());

    auto regionLookup = makeTestRegionLookup();
    resourceManager.computeBlockAppearances(regionLookup);

    BlockModelCache modelCache;
    ASSERT_TRUE(modelCache.initialize(resourceManager));
    modelCache.setRegionLookup(regionLookup);

    BlockModelCache* oldModelCache = ChunkMesher::modelCache();
    const bool oldGreedy = ChunkMesher::isGreedyMeshingEnabled();
    const bool oldLightingEnabled = ChunkMesher::isLightingEnabled();
    const LightingMode oldLightingMode = ChunkMesher::lightingMode();
    const auto restoreGuard =
        std::shared_ptr<void>(nullptr, [oldModelCache, oldGreedy, oldLightingEnabled, oldLightingMode](void*) {
            ChunkMesher::setModelCache(oldModelCache);
            ChunkMesher::setGreedyMeshing(oldGreedy);
            ChunkMesher::setLightingEnabled(oldLightingEnabled);
            ChunkMesher::setLightingMode(oldLightingMode);
        });

    ChunkMesher::setModelCache(&modelCache);
    const BlockState& shallowWater = VanillaBlocks::WATER->defaultState().with(BlockStateProperties::LEVEL_0_15(), 7);
    m_chunk->setBlockState(8, 64, 8, &shallowWater);

    MeshData solidMesh;
    MeshData transparentMesh;
    ChunkMesher::setGreedyMeshing(false);
    ChunkMesher::setLightingEnabled(false);
    ChunkMesher::setLightingMode(LightingMode::Flat);
    ChunkMesher::generateSplitMesh(*m_chunk, solidMesh, transparentMesh, nullptr, nullptr);

    ASSERT_FALSE(transparentMesh.empty());

    const MeshBounds bounds = computeBounds(transparentMesh);
    EXPECT_LT(bounds.maxY - bounds.minY, 0.5f);
    EXPECT_GT(bounds.maxY - bounds.minY, 0.01f);
}

TEST_F(ChunkMesherWithModelCacheTest, ShapeFallback_RedstoneTorchIsNotFullCube)
{
    m_chunk->setBlockState(8, 64, 8, &VanillaBlocks::REDSTONE_TORCH->defaultState());

    ChunkMesher::setGreedyMeshing(false);
    ChunkMesher::setLightingEnabled(false);

    MeshData mesh;
    ChunkMesher::generateMesh(*m_chunk, mesh, nullptr, nullptr);
    ASSERT_FALSE(mesh.empty());

    const MeshBounds bounds = computeBounds(mesh);
    EXPECT_LT(bounds.maxX - bounds.minX, 1.0f);
    EXPECT_LT(bounds.maxY - bounds.minY, 1.0f);
    EXPECT_LT(bounds.maxZ - bounds.minZ, 1.0f);
}

TEST_F(ChunkMesherWithModelCacheTest, ShapeFallback_RepeaterAndPressurePlateAreNotFullCube)
{
    ChunkMesher::setGreedyMeshing(false);
    ChunkMesher::setLightingEnabled(false);

    {
        ChunkData repeaterChunk(0, 0);
        repeaterChunk.setBlockState(8, 64, 8, &VanillaBlocks::REDSTONE_REPEATER->defaultState());

        MeshData mesh;
        ChunkMesher::generateMesh(repeaterChunk, mesh, nullptr, nullptr);
        ASSERT_FALSE(mesh.empty());

        const MeshBounds bounds = computeBounds(mesh);
        const f32 repeaterHeight = static_cast<f32>(bounds.maxY - bounds.minY);
        EXPECT_LT(repeaterHeight, 1.0f);
    }

    {
        ChunkData pressurePlateChunk(0, 0);
        pressurePlateChunk.setBlockState(8, 64, 8, &VanillaBlocks::STONE_PRESSURE_PLATE->defaultState());

        MeshData mesh;
        ChunkMesher::generateMesh(pressurePlateChunk, mesh, nullptr, nullptr);
        ASSERT_FALSE(mesh.empty());

        const MeshBounds bounds = computeBounds(mesh);
        const f32 pressurePlateHeight = static_cast<f32>(bounds.maxY - bounds.minY);
        EXPECT_LT(pressurePlateHeight, 1.0f);
    }
}

TEST(ChunkMesherLiquidMaterialTest, ParticleOnlyWaterModelStillRendersWithLiquidTextures)
{
    VanillaBlocks::initialize();

    auto pack = std::make_shared<InMemoryResourcePack>();
    pack->add("assets/minecraft/blockstates/water.json", toBytes(R"({
    "variants": {
        "": { "model": "minecraft:block/water" }
    }
})"));
    pack->add("assets/minecraft/models/block/water.json", toBytes(R"({
    "textures": {
        "particle": "block/water_still"
    }
})"));
    pack->add("assets/minecraft/textures/block/water_still.png", makeValid1x1Png());
    pack->add("assets/minecraft/textures/block/water_flow.png", makeValid1x1Png());

    ResourceManager resourceManager;
    ASSERT_TRUE(resourceManager.addResourcePack(pack).success());
    ASSERT_TRUE(resourceManager.loadAllResources().success());

    auto regionLookup = makeTestRegionLookup();
    resourceManager.computeBlockAppearances(regionLookup);

    const BlockAppearance* waterAppearance =
        resourceManager.getBlockAppearance(ResourceLocation("minecraft:water"), {});
    ASSERT_NE(waterAppearance, nullptr);
    EXPECT_TRUE(waterAppearance->faceTextures.empty());

    BlockModelCache modelCache;
    ASSERT_TRUE(modelCache.initialize(resourceManager));
    modelCache.setRegionLookup(regionLookup);

    const auto& waterState = VanillaBlocks::WATER->defaultState();
    EXPECT_EQ(modelCache.getBlockAppearance(&waterState), modelCache.getBlockAppearance(waterState.stateId()));

    BlockModelCache* oldModelCache = ChunkMesher::modelCache();
    const bool oldGreedy = ChunkMesher::isGreedyMeshingEnabled();
    const bool oldLightingEnabled = ChunkMesher::isLightingEnabled();
    const LightingMode oldLightingMode = ChunkMesher::lightingMode();

    ChunkMesher::setModelCache(&modelCache);
    ChunkMesher::setGreedyMeshing(false);
    ChunkMesher::setLightingEnabled(false);
    ChunkMesher::setLightingMode(LightingMode::Flat);

    ChunkData chunk(0, 0);
    chunk.setBlockState(8, 64, 8, &VanillaBlocks::WATER->defaultState());

    MeshData solidMesh;
    MeshData transparentMesh;
    ChunkMesher::generateSplitMesh(chunk, solidMesh, transparentMesh, nullptr, nullptr);

    ChunkMesher::setModelCache(oldModelCache);
    ChunkMesher::setGreedyMeshing(oldGreedy);
    ChunkMesher::setLightingEnabled(oldLightingEnabled);
    ChunkMesher::setLightingMode(oldLightingMode);

    EXPECT_FALSE(transparentMesh.empty());
}

// ============================================================================
// ChunkMesher::getDefaultBlockTintColor 测试
// ============================================================================

/**
 * @brief 测试 getDefaultBlockTintColor 对于空指针返回白色
 */
TEST(GetDefaultBlockTintColor, NullBlockReturnsWhite)
{
    VanillaBlocks::initialize();
    const u32 color = ChunkMesher::getDefaultBlockTintColor(nullptr);

    // 白色 = 0xFFFFFFFF
    EXPECT_EQ(color, 0xFFFFFFFFu);
}

/**
 * @brief 测试 getDefaultBlockTintColor 对于石头（无着色）返回白色
 */
TEST(GetDefaultBlockTintColor, StoneReturnsWhite)
{
    VanillaBlocks::initialize();
    const u32 color = ChunkMesher::getDefaultBlockTintColor(&VanillaBlocks::STONE->defaultState());

    // 石头不需要着色
    EXPECT_EQ(color, 0xFFFFFFFFu);
}

/**
 * @brief 测试 getDefaultBlockTintColor 对于云杉树叶返回固定颜色
 */
TEST(GetDefaultBlockTintColor, SpruceLeavesReturnsFixedColor)
{
    VanillaBlocks::initialize();
    const u32 color = ChunkMesher::getDefaultBlockTintColor(&VanillaBlocks::SPRUCE_LEAVES->defaultState());

    // 云杉树叶使用固定颜色 0x619961
    const u8 r = color & 0xFFu;
    const u8 g = (color >> 8) & 0xFFu;
    const u8 b = (color >> 16) & 0xFFu;

    EXPECT_EQ(r, 0x61u);
    EXPECT_EQ(g, 0x99u);
    EXPECT_EQ(b, 0x61u);
}

/**
 * @brief 测试 getDefaultBlockTintColor 对于桦树叶返回固定颜色
 */
TEST(GetDefaultBlockTintColor, BirchLeavesReturnsFixedColor)
{
    VanillaBlocks::initialize();
    const u32 color = ChunkMesher::getDefaultBlockTintColor(&VanillaBlocks::BIRCH_LEAVES->defaultState());

    // 桦树叶使用固定颜色 0x80A755
    // packRgb 将 RGB(0xRRGGBB) 打包为 R|(G<<8)|(B<<16)|255<<24
    // 所以: 低字节=R, 次字节=G, 再次字节=B
    const u8 r = color & 0xFFu;
    const u8 g = (color >> 8) & 0xFFu;
    const u8 b = (color >> 16) & 0xFFu;

    EXPECT_EQ(r, 0x80u);
    EXPECT_EQ(g, 0xA7u);
    EXPECT_EQ(b, 0x55u);
}

/**
 * @brief 测试 getDefaultBlockTintColor 对于水返回默认水颜色
 */
TEST(GetDefaultBlockTintColor, WaterReturnsDefaultColor)
{
    VanillaBlocks::initialize();
    const u32 color = ChunkMesher::getDefaultBlockTintColor(&VanillaBlocks::WATER->defaultState());

    // 水使用默认颜色 0x3F76E4
    const u8 r = color & 0xFFu;
    const u8 g = (color >> 8) & 0xFFu;
    const u8 b = (color >> 16) & 0xFFu;

    EXPECT_EQ(r, 0x3Fu);
    EXPECT_EQ(g, 0x76u);
    EXPECT_EQ(b, 0xE4u);
}

/**
 * @brief 测试 getDefaultBlockTintColor 对于岩浆返回白色（不着色）
 */
TEST(GetDefaultBlockTintColor, LavaReturnsWhite)
{
    VanillaBlocks::initialize();
    const u32 color = ChunkMesher::getDefaultBlockTintColor(&VanillaBlocks::LAVA->defaultState());

    // 岩浆不使用着色
    EXPECT_EQ(color, 0xFFFFFFFFu);
}
