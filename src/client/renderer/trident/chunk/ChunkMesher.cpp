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

#include "ChunkMesher.hpp"
#include "AmbientOcclusionCalculator.hpp"
#include "client/resource/BlockModelCache.hpp"
#include "client/resource/ResourceManager.hpp"
#include "client/world/color/BiomeColors.hpp"
#include "client/world/color/blend/ChunkBiomeAccessor.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc {

// ============================================================================
// 静态成员初始化
// ============================================================================

BlockModelCache* ChunkMesher::s_modelCache = nullptr;
bool ChunkMesher::s_useGreedyMeshing = true;
bool ChunkMesher::s_lightingEnabled = true;
LightingMode ChunkMesher::s_lightingMode = LightingMode::Smooth;
std::array<u32, 65536> ChunkMesher::s_grassColorMap{};
std::array<u32, 65536> ChunkMesher::s_foliageColorMap{};
bool ChunkMesher::s_grassColorMapLoaded = false;
bool ChunkMesher::s_foliageColorMapLoaded = false;
client::BiomeColorBlender ChunkMesher::s_biomeColorBlender;

namespace {

enum class MeshPass : u8 {
    All = 0,
    SolidOnly = 1,
    TransparentOnly = 2,
};

thread_local MeshPass t_meshPass = MeshPass::All;

class MeshPassScope {
public:
    explicit MeshPassScope(MeshPass pass)
        : m_prevPass(t_meshPass)
    {
        t_meshPass = pass;
    }

    ~MeshPassScope() { t_meshPass = m_prevPass; }

private:
    MeshPass m_prevPass;
};

/**
 * @brief 将 RGBA 分量打包为顶点颜色（与 VK_FORMAT_R8G8B8A8_UNORM 对齐）
 *
 * @note 低字节为 R，高字节为 A。这样在小端内存布局下可与 Vulkan 直接对应。
 */
[[nodiscard]] constexpr u32 packVertexColor(u8 r, u8 g, u8 b, u8 a)
{
    return static_cast<u32>(r) | (static_cast<u32>(g) << 8) | (static_cast<u32>(b) << 16) | (static_cast<u32>(a) << 24);
}

/**
 * @brief 返回面向明暗系数
 *
 * DOWN=0.5, UP=1.0, NORTH/SOUTH=0.8, WEST/EAST=0.6
 */
[[nodiscard]] constexpr f32 getFaceShade(Face face)
{
    switch (face) {
        case Face::Bottom:
            return 0.5f;
        case Face::Top:
            return 1.0f;
        case Face::North:
        case Face::South:
            return 0.8f;
        case Face::West:
        case Face::East:
            return 0.6f;
        default:
            return 1.0f;
    }
}

[[nodiscard]] constexpr u32 packRgb(u32 rgb)
{
    return packVertexColor(
        static_cast<u8>((rgb >> 16) & 0xFF), static_cast<u8>((rgb >> 8) & 0xFF), static_cast<u8>(rgb & 0xFF), 255);
}

[[nodiscard]] size_t estimateReservedFaceCount(const ChunkData& chunk, MeshPass pass)
{
    size_t nonAirBlockCount = 0;
    for (i32 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        const ChunkSection* section = chunk.getSection(sectionY);
        if (section != nullptr && !section->isEmpty()) {
            nonAirBlockCount += section->getBlockCount();
        }
    }

    switch (pass) {
        case MeshPass::TransparentOnly:
            // 透明层通常比实心层稀疏得多，给更小的初始容量可以显著降低 split mesh 峰值。
            return std::clamp(nonAirBlockCount / 8, static_cast<size_t>(256), static_cast<size_t>(2048));
        case MeshPass::SolidOnly:
        case MeshPass::All:
        default:
            return std::clamp(
                nonAirBlockCount * static_cast<size_t>(2), static_cast<size_t>(1024), static_cast<size_t>(8192));
    }
}

[[nodiscard]] u32 applyShadeToPackedColor(u32 packedColor, f32 factor)
{
    const f32 clamped = std::clamp(factor, 0.0f, 1.0f);
    const f32 r = static_cast<f32>(packedColor & 0xFFu) / 255.0f;
    const f32 g = static_cast<f32>((packedColor >> 8) & 0xFFu) / 255.0f;
    const f32 b = static_cast<f32>((packedColor >> 16) & 0xFFu) / 255.0f;

    return packVertexColor(static_cast<u8>(std::round(std::clamp(r * clamped, 0.0f, 1.0f) * 255.0f)),
        static_cast<u8>(std::round(std::clamp(g * clamped, 0.0f, 1.0f) * 255.0f)),
        static_cast<u8>(std::round(std::clamp(b * clamped, 0.0f, 1.0f) * 255.0f)),
        255);
}

[[nodiscard]] u32 applyBlockAlpha(u32 packedColor, const BlockState* block)
{
    if (block != nullptr && block->isLiquid()) {
        // 水体处于半透明层时仍保留一定透视能力。
        // 纹理 alpha 在不同资源包下差异较大，这里提供稳定 alpha 兜底。
        constexpr u8 LIQUID_ALPHA = 180;
        return (packedColor & 0x00FFFFFFu) | (static_cast<u32>(LIQUID_ALPHA) << 24);
    }
    return packedColor;
}

struct FaceLayerRenderData {
    TextureRegion texture;
    i32 tintIndex = -1;
};

struct CachedLiquidFaceLayers {
    std::vector<FaceLayerRenderData> stillLayers;
    std::vector<FaceLayerRenderData> flowLayers;
};

struct LiquidFaceLayerCacheState {
    ResourceManager* resourceManager = nullptr;
    std::unordered_map<const BlockState*, CachedLiquidFaceLayers> layersByBlockState;

    void reset(ResourceManager* currentResourceManager)
    {
        resourceManager = currentResourceManager;
        layersByBlockState.clear();
    }
};

thread_local LiquidFaceLayerCacheState t_liquidFaceLayerCache;

[[nodiscard]] std::vector<FaceLayerRenderData> collectFaceLayers(
    const BlockAppearance* appearance, const std::string& faceName)
{
    if (!appearance) {
        return {};
    }

    const auto tryLayerKey = [&](const std::string& key) -> std::vector<FaceLayerRenderData> {
        auto it = appearance->faceTextureLayers.find(key);
        if (it == appearance->faceTextureLayers.end() || it->second.empty()) {
            return {};
        }

        std::vector<FaceLayerRenderData> layers;
        layers.reserve(it->second.size());
        for (const auto& layer : it->second) {
            layers.push_back(FaceLayerRenderData{layer.texture, layer.tintIndex});
        }
        return layers;
    };

    if (auto layers = tryLayerKey(faceName); !layers.empty()) {
        return layers;
    }
    if (auto layers = tryLayerKey("side"); !layers.empty()) {
        return layers;
    }
    if (auto layers = tryLayerKey("all"); !layers.empty()) {
        return layers;
    }

    auto texIt = appearance->faceTextures.find(faceName);
    std::string resolvedKey = faceName;
    if (texIt == appearance->faceTextures.end()) {
        texIt = appearance->faceTextures.find("side");
        resolvedKey = "side";
        if (texIt == appearance->faceTextures.end()) {
            texIt = appearance->faceTextures.find("all");
            resolvedKey = "all";
            if (texIt == appearance->faceTextures.end()) {
                return {};
            }
        }
    }

    i32 tintIndex = -1;
    auto tintIt = appearance->faceTintIndices.find(resolvedKey);
    if (tintIt != appearance->faceTintIndices.end()) {
        tintIndex = tintIt->second;
    } else {
        tintIt = appearance->faceTintIndices.find(faceName);
        if (tintIt != appearance->faceTintIndices.end()) {
            tintIndex = tintIt->second;
        }
    }

    return {FaceLayerRenderData{texIt->second, tintIndex}};
}

[[nodiscard]] std::vector<FaceLayerRenderData> collectLiquidFaceLayers(const BlockState* block, Face face)
{
    if (block == nullptr || !block->isLiquid()) {
        return {};
    }

    BlockModelCache* modelCache = ChunkMesher::modelCache();
    if (modelCache == nullptr || modelCache->resourceManager() == nullptr) {
        return {};
    }

    ResourceManager* resourceManager = modelCache->resourceManager();
    if (t_liquidFaceLayerCache.resourceManager != resourceManager) {
        t_liquidFaceLayerCache.reset(resourceManager);
    }

    auto cacheIt = t_liquidFaceLayerCache.layersByBlockState.find(block);
    if (cacheIt == t_liquidFaceLayerCache.layersByBlockState.end()) {
        CachedLiquidFaceLayers cachedLayers;
        const ResourceLocation& blockLocation = block->blockLocation();

        const bool isWater = blockLocation.namespace_() == "minecraft" && blockLocation.path() == "water";
        const bool isLava = blockLocation.namespace_() == "minecraft" && blockLocation.path() == "lava";

        const std::string stillName = isWater ? "water_still" : isLava ? "lava_still" : blockLocation.path() + "_still";
        const std::string flowName = isWater ? "water_flow" : isLava ? "lava_flow" : blockLocation.path() + "_flow";

        const ResourceLocation stillTexture(blockLocation.namespace_(), "textures/block/" + stillName);
        const ResourceLocation flowTexture(blockLocation.namespace_(), "textures/block/" + flowName);

        const TextureRegion* stillRegion = resourceManager->getTextureRegion(stillTexture);
        const TextureRegion* flowRegion = resourceManager->getTextureRegion(flowTexture);

        if (stillRegion == nullptr) {
            stillRegion = flowRegion;
        }
        if (flowRegion == nullptr) {
            flowRegion = stillRegion;
        }

        if (stillRegion != nullptr) {
            cachedLayers.stillLayers.push_back(FaceLayerRenderData{*stillRegion, -1});
        }
        if (flowRegion != nullptr) {
            cachedLayers.flowLayers.push_back(FaceLayerRenderData{*flowRegion, -1});
        }

        cacheIt = t_liquidFaceLayerCache.layersByBlockState.emplace(block, std::move(cachedLayers)).first;
    }

    const bool useStillTexture = (face == Face::Top || face == Face::Bottom);
    const auto& cachedResult = useStillTexture ? cacheIt->second.stillLayers : cacheIt->second.flowLayers;
    return cachedResult;
}

[[nodiscard]] const BlockState* getMeshBlockStateAt(
    const ChunkData& chunk, i32 x, i32 y, i32 z, const ChunkData* neighborChunks[6])
{
    constexpr i32 SIZE = world::CHUNK_WIDTH;

    if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
        return nullptr;
    }

    const ChunkData* sampleChunk = &chunk;
    i32 localX = x;
    i32 localZ = z;

    if (localX < 0) {
        localX += SIZE;
        sampleChunk = neighborChunks ? neighborChunks[0] : nullptr;
    } else if (localX >= SIZE) {
        localX -= SIZE;
        sampleChunk = neighborChunks ? neighborChunks[1] : nullptr;
    }

    if (localZ < 0) {
        localZ += SIZE;
        if (sampleChunk == &chunk || sampleChunk == nullptr) {
            sampleChunk = neighborChunks ? neighborChunks[2] : nullptr;
        }
    } else if (localZ >= SIZE) {
        localZ -= SIZE;
        if (sampleChunk == &chunk || sampleChunk == nullptr) {
            sampleChunk = neighborChunks ? neighborChunks[3] : nullptr;
        }
    }

    if (sampleChunk == nullptr) {
        return nullptr;
    }

    return sampleChunk->getBlockState(localX, y, localZ);
}

[[nodiscard]] const fluid::FluidState* getMeshFluidStateAt(
    const ChunkData& chunk, i32 x, i32 y, i32 z, const ChunkData* neighborChunks[6])
{
    const BlockState* blockState = getMeshBlockStateAt(chunk, x, y, z, neighborChunks);
    if (blockState == nullptr) {
        return nullptr;
    }

    return blockState->getFluidState();
}

[[nodiscard]] f32 getLiquidActualHeightAt(
    const ChunkData& chunk, i32 x, i32 y, i32 z, const fluid::Fluid& fluid, const ChunkData* neighborChunks[6])
{
    const fluid::FluidState* fluidState = getMeshFluidStateAt(chunk, x, y, z, neighborChunks);
    if (fluidState == nullptr || !fluidState->getFluid().isEquivalentTo(fluid)) {
        return 0.0f;
    }

    const fluid::FluidState* aboveFluid = getMeshFluidStateAt(chunk, x, y + 1, z, neighborChunks);
    if (aboveFluid != nullptr && aboveFluid->getFluid().isEquivalentTo(fluid)) {
        return 1.0f;
    }

    return fluidState->getHeight();
}

[[nodiscard]] f32 getLiquidCornerHeight(
    const ChunkData& chunk, i32 x, i32 y, i32 z, const fluid::Fluid& fluid, const ChunkData* neighborChunks[6])
{
    i32 contributionCount = 0;
    f32 heightSum = 0.0f;

    for (i32 index = 0; index < 4; ++index) {
        const i32 sampleX = x - (index & 1);
        const i32 sampleZ = z - ((index >> 1) & 1);

        const f32 fluidHeight = getLiquidActualHeightAt(chunk, sampleX, y, sampleZ, fluid, neighborChunks);
        if (fluidHeight >= 0.8f) {
            heightSum += fluidHeight * 10.0f;
            contributionCount += 10;
        } else if (fluidHeight > 0.0f) {
            heightSum += fluidHeight;
            ++contributionCount;
        } else {
            const BlockState* blockState = getMeshBlockStateAt(chunk, sampleX, y, sampleZ, neighborChunks);
            if (blockState == nullptr || !blockState->owner().material().isSolid()) {
                ++contributionCount;
            }
        }
    }

    if (contributionCount == 0) {
        return 0.0f;
    }

    return heightSum / static_cast<f32>(contributionCount);
}

template <typename TintResolver>
void emitLiquidFace(MeshData& mesh,
    Face face,
    f64 x,
    f64 y,
    f64 z,
    const ChunkData& chunk,
    i32 blockX,
    i32 blockY,
    i32 blockZ,
    u8 skyLight,
    u8 blockLight,
    const BlockState* block,
    const std::vector<FaceLayerRenderData>& faceLayers,
    const ChunkData* neighborChunks[6],
    TintResolver&& resolveTintColor)
{
    if (block == nullptr || faceLayers.empty()) {
        return;
    }

    const fluid::FluidState* fluidState = block->getFluidState();
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return;
    }

    const fluid::Fluid& fluid = fluidState->getFluid();
    const f32 southWestHeight = getLiquidCornerHeight(chunk, blockX, blockY, blockZ + 1, fluid, neighborChunks);
    const f32 southEastHeight = getLiquidCornerHeight(chunk, blockX + 1, blockY, blockZ + 1, fluid, neighborChunks);
    const f32 northEastHeight = getLiquidCornerHeight(chunk, blockX + 1, blockY, blockZ, fluid, neighborChunks);
    const f32 northWestHeight = getLiquidCornerHeight(chunk, blockX, blockY, blockZ, fluid, neighborChunks);

    std::array<const ChunkData*, 4> biomeNeighbors = {};
    if (neighborChunks) {
        biomeNeighbors[0] = neighborChunks[0];
        biomeNeighbors[1] = neighborChunks[1];
        biomeNeighbors[2] = neighborChunks[2];
        biomeNeighbors[3] = neighborChunks[3];
    }
    client::ChunkBiomeAccessor biomeAccessor(chunk, biomeNeighbors, chunk.x(), chunk.z());

    const i32 worldX = chunk.x() * world::CHUNK_WIDTH + blockX;
    const i32 worldZ = chunk.z() * world::CHUNK_WIDTH + blockZ;
    const u8 packedLight = static_cast<u8>(((skyLight & 0x0F) << 4) | (blockLight & 0x0F));
    const auto faceNormal = BlockGeometry::getFaceNormal(face);
    auto faceVertices = BlockGeometry::getFaceVertices(face);

    const f32 topOffset = face == Face::Top ? -0.001f : 0.0f;
    const f32 bottomOffset = face == Face::Bottom ? 0.001f : 0.0f;

    auto applyCornerHeight = [&](size_t vertexIndex, f32 height) {
        faceVertices[vertexIndex * 3 + 1] = static_cast<f64>(height);
    };

    switch (face) {
        case Face::Top:
            applyCornerHeight(0, southWestHeight + topOffset);
            applyCornerHeight(1, southEastHeight + topOffset);
            applyCornerHeight(2, northEastHeight + topOffset);
            applyCornerHeight(3, northWestHeight + topOffset);
            break;
        case Face::Bottom:
            applyCornerHeight(0, bottomOffset);
            applyCornerHeight(1, bottomOffset);
            applyCornerHeight(2, bottomOffset);
            applyCornerHeight(3, bottomOffset);
            break;
        case Face::North:
            applyCornerHeight(2, northWestHeight);
            applyCornerHeight(3, northEastHeight);
            break;
        case Face::South:
            applyCornerHeight(2, southEastHeight);
            applyCornerHeight(3, southWestHeight);
            break;
        case Face::West:
            applyCornerHeight(2, southWestHeight);
            applyCornerHeight(3, northWestHeight);
            break;
        case Face::East:
            applyCornerHeight(2, northEastHeight);
            applyCornerHeight(3, southEastHeight);
            break;
        default:
            break;
    }

    for (size_t layerIndex = 0; layerIndex < faceLayers.size(); ++layerIndex) {
        const auto& layer = faceLayers[layerIndex];
        const u32 tintColor = resolveTintColor(biomeAccessor, worldX, blockY, worldZ, block, layer.tintIndex);
        const u32 shadedColor = applyBlockAlpha(applyShadeToPackedColor(tintColor, getFaceShade(face)), block);

        const f64 layerOffset = static_cast<f64>(layerIndex) * 0.001f;
        f64 uvs[4][2] = {{layer.texture.u0, layer.texture.v1},
            {layer.texture.u1, layer.texture.v1},
            {layer.texture.u1, layer.texture.v0},
            {layer.texture.u0, layer.texture.v0}};

        std::array<Vertex, 4> faceVerts;
        for (size_t i = 0; i < 4; ++i) {
            faceVerts[i] = Vertex(static_cast<f32>(x + faceVertices[i * 3 + 0] + faceNormal[0] * layerOffset),
                static_cast<f32>(y + faceVertices[i * 3 + 1] + faceNormal[1] * layerOffset),
                static_cast<f32>(z + faceVertices[i * 3 + 2] + faceNormal[2] * layerOffset),
                static_cast<f32>(uvs[i][0]),
                static_cast<f32>(uvs[i][1]),
                shadedColor,
                packedLight);
        }

        u16 baseIndex = static_cast<u16>(mesh.vertices.size());
        for (const auto& vertex : faceVerts) {
            mesh.vertices.push_back(vertex);
        }

        const auto indices = BlockGeometry::getFaceIndices();
        for (u16 index : indices) {
            mesh.indices.push_back(static_cast<u16>(baseIndex + index));
        }
    }
}

} // namespace

// ============================================================================
// ChunkMesher 实现
// ============================================================================

void ChunkMesher::setModelCache(BlockModelCache* cache)
{
    s_modelCache = cache;
    _refreshBiomeColorMaps();
    if (cache) {
        spdlog::info("ChunkMesher: Using BlockModelCache for block appearances");
    } else {
        spdlog::warn("ChunkMesher: BlockModelCache set to null, no models will be rendered");
    }
}

void ChunkMesher::generateMesh(
    const ChunkData& chunk, MeshData& outMesh, const ChunkData* neighbors[6], const std::atomic<bool>* abortSignal)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.ChunkMesh, "GenerateMesh", "phase", "mesh");

    outMesh.clear();

    if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
        return;
    }

    const size_t estimatedFaces = estimateReservedFaceCount(chunk, t_meshPass);
    outMesh.reserve(
        estimatedFaces * BlockGeometry::VERTICES_PER_FACE, estimatedFaces * BlockGeometry::INDICES_PER_FACE);

    // 遍历所有区块段
    for (i32 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
            outMesh.clear();
            return;
        }

        if (chunk.hasSection(sectionY)) {
            generateSectionMesh(chunk, sectionY, outMesh, neighbors, abortSignal);
        }
    }
}

void ChunkMesher::generateSplitMesh(const ChunkData& chunk,
    MeshData& outSolidMesh,
    MeshData& outTransparentMesh,
    const ChunkData* neighbors[6],
    const std::atomic<bool>* abortSignal)
{
    if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
        outSolidMesh.clear();
        outTransparentMesh.clear();
        return;
    }

    // 第一遍：实心层
    {
        MeshPassScope passScope(MeshPass::SolidOnly);
        generateMesh(chunk, outSolidMesh, neighbors, abortSignal);
    }

    if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
        outSolidMesh.clear();
        outTransparentMesh.clear();
        return;
    }

    // 第二遍：半透明层（含水体/玻璃等）
    {
        MeshPassScope passScope(MeshPass::TransparentOnly);
        generateMesh(chunk, outTransparentMesh, neighbors, abortSignal);
    }
}

void ChunkMesher::generateSectionMesh(const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6],
    const std::atomic<bool>* abortSignal)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.ChunkMesh, "GenerateSectionMesh", "phase", "section");

    if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
        return;
    }

    if (s_useGreedyMeshing) {
        _greedyMeshSection(chunk, sectionIndex, outMesh, neighborChunks, abortSignal);
    } else {
        _simpleMeshSection(chunk, sectionIndex, outMesh, neighborChunks, abortSignal);
    }
}

bool ChunkMesher::_shouldRenderBlock(const BlockState* state)
{
    if (!state || state->isAir()) {
        return false;
    }

    const bool isTransparentLike = state->isTransparent() || state->isLiquid();
    switch (t_meshPass) {
        case MeshPass::All:
            return true;
        case MeshPass::SolidOnly:
            return !isTransparentLike;
        case MeshPass::TransparentOnly:
            return isTransparentLike;
        default:
            return true;
    }
}

// 将 Face 转换为 Direction（枚举值一一对应）
[[nodiscard]] static Direction faceToDirection(Face face)
{
    return static_cast<Direction>(static_cast<u8>(face));
}

bool ChunkMesher::_shouldRenderFace(const BlockState* block, const BlockState* neighbor, Face face)
{
    if (!block) {
        return false;
    }

    // 邻居是空气（或越界）时渲染外露面
    if (!neighbor || neighbor->isAir()) {
        return true;
    }

    // 相同状态对象之间不渲染内部面
    if (block == neighbor) {
        return false;
    }

    // 液体渲染规则
    if (block->isLiquid()) {
        if (neighbor->isLiquid()) {
            return false;
        }
        if (neighbor->getCollisionShape().isEmpty()) {
            return false;
        }
        if (neighbor->isTransparent()) {
            return true;
        }
        return false;
    }

    // 非液体方块与液体相邻，需要渲染交界面
    if (neighbor->isLiquid()) {
        return true;
    }

    // 树叶渲染规则
    if (BlockTags::LEAVES().contains(*block)) {
        const bool neighborIsLeaves = BlockTags::LEAVES().contains(*neighbor);
        if (neighborIsLeaves) {
            return false;
        }
        if (!neighbor->isTransparent()) {
            return false;
        }
        return true;
    }

    // 透明方块规则
    if (neighbor->isTransparent()) {
        if (block->isTransparent() && block->blockId() == neighbor->blockId()) {
            return false;
        }

        // 方块自定义面剔除逻辑（如铁栏杆/铜栏杆之间的连接面剔除）
        const Direction dir = faceToDirection(face);
        if (block->getBlock().skipRendering(*block, *neighbor, dir)) {
            return false;
        }

        return true;
    }

    // 透明方块贴着不透明方块时必须保留面
    if (block->isTransparent()) {
        return true;
    }

    // ========== 形状遮挡检测 ==========
    //
    // 面剔除逻辑：
    // 1. 如果邻居不是实心方块（!isSolid()），渲染该面
    // 2. 否则使用 VoxelShape 面遮挡检测：
    //    - 获取当前方块在指定方向的面遮挡形状
    //    - 获取邻居方块在相反方向的面遮挡形状
    //    - 使用 ONLY_FIRST 检测是否有独占区域
    //
    // 注意：isSolid() 对应方块是否为实心方块，与 isOpaque() 不同。

    // 如果邻居不是实心方块，渲染该面
    if (!neighbor->isSolid()) {
        return true;
    }

    // 当前方块是否使用形状进行遮挡检测
    // 大多数实心方块返回 false（使用简单的 isSolid 检测）
    // 台阶、楼梯、栅栏等非完整方块返回 true（需要精确形状检测）
    const bool useShapeOcclusion = block->useShapeForLightOcclusion() || neighbor->useShapeForLightOcclusion();

    return false; // TODO 下面的形状遮挡检测逻辑有很深的bug，很难解决，暂时先放行所有面，后续重构时再完善

    if (!useShapeOcclusion) {
        // 两个都是实心方块，不需要形状检测，遮挡
        return false;
    }

    // 使用形状遮挡检测
    const Direction dir = faceToDirection(face);
    const Direction oppositeDir = Directions::opposite(dir);

    // 获取当前方块在指定方向的面遮挡形状
    const CollisionShape blockFaceShape = block->getFaceOcclusionShape(dir);

    // 获取邻居方块在相反方向的面遮挡形状
    const CollisionShape neighborFaceShape = neighbor->getFaceOcclusionShape(oppositeDir);

    // 如果任一面形状是完整方块，完全遮挡
    if (blockFaceShape.isFullBlock() && neighborFaceShape.isFullBlock()) {
        return false;
    }

    // 如果任一面形状为空，不遮挡
    if (blockFaceShape.isEmpty()) {
        return true;
    }
    if (neighborFaceShape.isEmpty()) {
        return true;
    }

    // 使用形状遮挡检测
    // 如果 blockFaceShape 有区域不在 neighborFaceShape 中，则需要渲染
    // 转换为 VoxelShape 进行精确比较
    const VoxelShape blockVoxel = Shapes::fromCollisionShape(blockFaceShape);
    const VoxelShape neighborVoxel = Shapes::fromCollisionShape(neighborFaceShape);

    // 使用 ONLY_FIRST 检测：如果 blockVoxel 有独占区域，则需要渲染
    // faceShapeOccludes 返回 true 表示完全遮挡，返回 false 表示有独占区域需要渲染
    return !Shapes::faceShapeOccludes(blockVoxel, neighborVoxel);
}

u8 ChunkMesher::sampleCombinedLight(const ChunkData& chunk, i32 x, i32 y, i32 z, const ChunkData* neighborChunks[6])
{
    return std::max(_sampleSkyLight(chunk, x, y, z, neighborChunks), _sampleBlockLight(chunk, x, y, z, neighborChunks));
}

u8 ChunkMesher::_sampleSkyLight(const ChunkData& chunk, i32 x, i32 y, i32 z, const ChunkData* neighborChunks[6])
{
    constexpr i32 SIZE = world::CHUNK_SECTION_HEIGHT;

    // 垂直越界：上方视为天空，下方视为无光
    if (y >= world::MAX_BUILD_HEIGHT) {
        return 15;
    }
    if (y < world::MIN_BUILD_HEIGHT) {
        return 0;
    }

    // 解析采样来源区块（当前区块或 X/Z 邻居）
    const ChunkData* sampleChunk = &chunk;
    i32 localX = x;
    i32 localZ = z;

    if (localX < 0) {
        localX += SIZE;
        sampleChunk = neighborChunks ? neighborChunks[0] : nullptr;
    } else if (localX >= SIZE) {
        localX -= SIZE;
        sampleChunk = neighborChunks ? neighborChunks[1] : nullptr;
    }

    if (localZ < 0) {
        localZ += SIZE;
        // 双轴越界时优先使用已选定的 X 邻区做近似，避免边界全亮接缝。
        if (sampleChunk == &chunk || sampleChunk == nullptr) {
            sampleChunk = neighborChunks ? neighborChunks[2] : nullptr;
        }
    } else if (localZ >= SIZE) {
        localZ -= SIZE;
        if (sampleChunk == &chunk || sampleChunk == nullptr) {
            sampleChunk = neighborChunks ? neighborChunks[3] : nullptr;
        }
    }

    // 邻区块尚未就绪时，按天空处理，避免边界黑边
    if (!sampleChunk) {
        return 15;
    }

    return sampleChunk->getSkyLight(localX, y, localZ);
}

u8 ChunkMesher::_sampleBlockLight(const ChunkData& chunk, i32 x, i32 y, i32 z, const ChunkData* neighborChunks[6])
{
    constexpr i32 SIZE = world::CHUNK_SECTION_HEIGHT;

    // 垂直越界：上下都视为无方块光照
    if (y >= world::MAX_BUILD_HEIGHT || y < world::MIN_BUILD_HEIGHT) {
        return 0;
    }

    const ChunkData* sampleChunk = &chunk;
    i32 localX = x;
    i32 localZ = z;

    if (localX < 0) {
        localX += SIZE;
        sampleChunk = neighborChunks ? neighborChunks[0] : nullptr;
    } else if (localX >= SIZE) {
        localX -= SIZE;
        sampleChunk = neighborChunks ? neighborChunks[1] : nullptr;
    }

    if (localZ < 0) {
        localZ += SIZE;
        if (sampleChunk == &chunk || sampleChunk == nullptr) {
            sampleChunk = neighborChunks ? neighborChunks[2] : nullptr;
        }
    } else if (localZ >= SIZE) {
        localZ -= SIZE;
        if (sampleChunk == &chunk || sampleChunk == nullptr) {
            sampleChunk = neighborChunks ? neighborChunks[3] : nullptr;
        }
    }

    if (!sampleChunk) {
        return 0;
    }

    return sampleChunk->getBlockLight(localX, y, localZ);
}

u32 ChunkMesher::_resolveTintColorBlended(const client::ChunkBiomeAccessor& accessor,
    i32 worldX,
    i32 worldY,
    i32 worldZ,
    const BlockState* block,
    i32 tintIndex)
{
    if (!block) {
        return packVertexColor(255, 255, 255, 255);
    }

    // 水体颜色处理
    if (block->isLiquid()) {
        if (block->is(VanillaBlocks::WATER)) {
            // 使用颜色混合器获取混合后的水体颜色
            const u32 waterColor = s_biomeColorBlender.getBlendedColorCached(accessor,
                worldX,
                worldY,
                worldZ,
                client::BiomeColors::waterColorResolver(),
                client::BiomeColorBlender::ResolverId::Water);
            return packRgb(waterColor);
        }
        // 岩浆不使用着色
        return packVertexColor(255, 255, 255, 255);
    }

    // 非 tint 着色的情况
    if (tintIndex < 0) {
        return packVertexColor(255, 255, 255, 255);
    }

    const bool isLeaves = block->is(VanillaBlocks::OAK_LEAVES) || block->is(VanillaBlocks::JUNGLE_LEAVES) ||
        block->is(VanillaBlocks::ACACIA_LEAVES) || block->is(VanillaBlocks::DARK_OAK_LEAVES) ||
        block->is(VanillaBlocks::SPRUCE_LEAVES) || block->is(VanillaBlocks::BIRCH_LEAVES);

    if (isLeaves) {
        // 云杉和桦树叶使用固定颜色，不进行混合
        if (block->is(VanillaBlocks::SPRUCE_LEAVES)) {
            return packRgb(client::BiomeColors::SPRUCE_LEAVES_COLOR);
        }
        if (block->is(VanillaBlocks::BIRCH_LEAVES)) {
            return packRgb(client::BiomeColors::BIRCH_LEAVES_COLOR);
        }

        // 其他树叶使用颜色混合器（混合器会处理 colormap）
        const u32 foliageColor = s_biomeColorBlender.getBlendedColorCached(accessor,
            worldX,
            worldY,
            worldZ,
            client::BiomeColors::foliageColorResolver(),
            client::BiomeColorBlender::ResolverId::Foliage);

        return packRgb(foliageColor);
    }

    // 草色系：使用颜色混合器（混合器会处理 colormap）
    const u32 grassColor = s_biomeColorBlender.getBlendedColorCached(accessor,
        worldX,
        worldY,
        worldZ,
        client::BiomeColors::grassColorResolver(),
        client::BiomeColorBlender::ResolverId::Grass);

    return packRgb(grassColor);
}

bool ChunkMesher::_tryLoadColorMap(std::string_view path, std::array<u32, 65536>& outColorMap)
{
    if (!s_modelCache || !s_modelCache->resourceManager()) {
        return false;
    }

    auto* resourceManager = s_modelCache->resourceManager();
    auto colorMapResult = resourceManager->loadTextureRGBA(ResourceLocation(std::string(path)));
    if (colorMapResult.failed()) {
        return false;
    }

    const auto& colorMap = colorMapResult.value();
    if (colorMap.width != 256 || colorMap.height != 256 || colorMap.pixels.size() < 256u * 256u * 4u) {
        return false;
    }

    for (size_t i = 0; i < outColorMap.size(); ++i) {
        const size_t offset = i * 4;
        const u32 r = static_cast<u32>(colorMap.pixels[offset]);
        const u32 g = static_cast<u32>(colorMap.pixels[offset + 1]);
        const u32 b = static_cast<u32>(colorMap.pixels[offset + 2]);
        outColorMap[i] = (r << 16) | (g << 8) | b;
    }

    return true;
}

void ChunkMesher::_refreshBiomeColorMaps()
{
    s_grassColorMapLoaded = _tryLoadColorMap("minecraft:textures/colormap/grass", s_grassColorMap);
    s_foliageColorMapLoaded = _tryLoadColorMap("minecraft:textures/colormap/foliage", s_foliageColorMap);

    // 设置 BiomeColorBlender 的 colormap 指针
    s_biomeColorBlender.setGrassColorMap(s_grassColorMapLoaded ? &s_grassColorMap : nullptr);
    s_biomeColorBlender.setFoliageColorMap(s_foliageColorMapLoaded ? &s_foliageColorMap : nullptr);

    spdlog::info(
        "ChunkMesher: Biome color maps loaded (grass={}, foliage={})", s_grassColorMapLoaded, s_foliageColorMapLoaded);
}

void ChunkMesher::setBiomeBlendRadius(i32 radius)
{
    s_biomeColorBlender.setBlendRadius(radius);
    // 清除缓存，因为混合半径变化会使所有缓存失效
    s_biomeColorBlender.clearCache();
    spdlog::info("ChunkMesher: Biome blend radius set to {} ({}x{} area)", radius, radius * 2 + 1, radius * 2 + 1);
}

i32 ChunkMesher::biomeBlendRadius()
{
    return s_biomeColorBlender.blendRadius();
}

void ChunkMesher::invalidateBiomeColorCache(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    s_biomeColorBlender.invalidateChunk(chunkX, chunkZ);
}

u32 ChunkMesher::getDefaultBlockTintColor(const BlockState* block)
{
    if (!block) {
        return packVertexColor(255, 255, 255, 255);
    }

    // 水体颜色 - 使用默认水颜色
    if (block->isLiquid()) {
        if (block->is(VanillaBlocks::WATER)) {
            // 默认水颜色
            return packRgb(world::biome::BiomeEffects::DEFAULT_WATER_COLOR);
        }
        // 岩浆不使用着色
        return packVertexColor(255, 255, 255, 255);
    }

    // 树叶颜色处理
    const bool isLeaves = block->is(VanillaBlocks::OAK_LEAVES) || block->is(VanillaBlocks::JUNGLE_LEAVES) ||
        block->is(VanillaBlocks::ACACIA_LEAVES) || block->is(VanillaBlocks::DARK_OAK_LEAVES) ||
        block->is(VanillaBlocks::SPRUCE_LEAVES) || block->is(VanillaBlocks::BIRCH_LEAVES);

    if (isLeaves) {
        // 云杉和桦树叶使用固定颜色
        if (block->is(VanillaBlocks::SPRUCE_LEAVES)) {
            return packRgb(client::BiomeColors::SPRUCE_LEAVES_COLOR);
        }
        if (block->is(VanillaBlocks::BIRCH_LEAVES)) {
            return packRgb(client::BiomeColors::BIRCH_LEAVES_COLOR);
        }

        // 其他树叶使用 foliage colormap 中心点颜色
        if (s_foliageColorMapLoaded) {
            // colormap 中心点索引: (128 << 8) | 128 = 32896
            // 对应 temperature=0.5, humidity=0.5
            return packRgb(s_foliageColorMap[32896]);
        }
        // 默认树叶颜色
        return packRgb(0x48B518); // FoliageColors.getDefault()
    }

    // 草方块和其他需要草颜色的方块
    if (block->is(VanillaBlocks::GRASS_BLOCK) || block->is(VanillaBlocks::SHORT_GRASS) ||
        block->is(VanillaBlocks::TALL_GRASS)) {
        // 草颜色使用 grass colormap 中心点颜色
        if (s_grassColorMapLoaded) {
            // colormap 中心点索引: (128 << 8) | 128 = 32896
            // 对应 temperature=0.5, humidity=0.5
            return packRgb(s_grassColorMap[32896]);
        }
        // 默认草颜色
        return packRgb(0xFF757F); // GrassColors.getDefault() - 品红表示缺失
    }

    // 其他方块不使用着色
    return packVertexColor(255, 255, 255, 255);
}

void ChunkMesher::_addFaceFromAppearance(MeshData& mesh,
    Face face,
    f64 x,
    f64 y,
    f64 z,
    const ChunkData& chunk,
    i32 blockX,
    i32 blockY,
    i32 blockZ,
    u8 skyLight,
    u8 blockLight,
    const BlockState* block,
    const BlockAppearance* appearance,
    const ChunkData* neighborChunks[6])
{
    if (!appearance) {
        return;
    }

    // 查找面的纹理
    std::string faceName;
    switch (face) {
        case Face::Bottom:
            faceName = "down";
            break;
        case Face::Top:
            faceName = "up";
            break;
        case Face::North:
            faceName = "north";
            break;
        case Face::South:
            faceName = "south";
            break;
        case Face::West:
            faceName = "west";
            break;
        case Face::East:
            faceName = "east";
            break;
        default:
            return;
    }

    auto faceLayers = block != nullptr && block->isLiquid() ? collectLiquidFaceLayers(block, face)
                                                            : collectFaceLayers(appearance, faceName);
    if (faceLayers.empty()) {
        return;
    }

    if (block != nullptr && block->isLiquid()) {
        auto resolveTintColor = [](const client::ChunkBiomeAccessor& biomeAccessor,
                                    i32 worldX,
                                    i32 worldY,
                                    i32 worldZ,
                                    const BlockState* blockState,
                                    i32 tintIndex) {
            return _resolveTintColorBlended(biomeAccessor, worldX, worldY, worldZ, blockState, tintIndex);
        };
        emitLiquidFace(mesh,
            face,
            x,
            y,
            z,
            chunk,
            blockX,
            blockY,
            blockZ,
            skyLight,
            blockLight,
            block,
            faceLayers,
            neighborChunks,
            resolveTintColor);
        return;
    }

    auto normal = BlockGeometry::getFaceNormal(face);
    auto vertices = BlockGeometry::getFaceVertices(face);

    // 打包双通道光照：高4位=天空光，低4位=方块光
    const u8 packedLight = static_cast<u8>(((skyLight & 0x0F) << 4) | (blockLight & 0x0F));

    // 创建生物群系访问器用于颜色混合
    std::array<const ChunkData*, 4> biomeNeighbors = {};
    if (neighborChunks) {
        biomeNeighbors[0] = neighborChunks[0]; // -X
        biomeNeighbors[1] = neighborChunks[1]; // +X
        biomeNeighbors[2] = neighborChunks[2]; // -Z
        biomeNeighbors[3] = neighborChunks[3]; // +Z
    }
    client::ChunkBiomeAccessor biomeAccessor(chunk, biomeNeighbors, chunk.x(), chunk.z());

    // 计算世界坐标用于生物群系颜色混合
    const i32 worldX = chunk.x() * world::CHUNK_WIDTH + blockX;
    const i32 worldZ = chunk.z() * world::CHUNK_WIDTH + blockZ;

    for (size_t layerIndex = 0; layerIndex < faceLayers.size(); ++layerIndex) {
        const auto& layer = faceLayers[layerIndex];
        const u32 tintColor = _resolveTintColorBlended(biomeAccessor, worldX, blockY, worldZ, block, layer.tintIndex);
        const u32 shadedColor = applyBlockAlpha(applyShadeToPackedColor(tintColor, getFaceShade(face)), block);

        // UV坐标根据顶点位置设置
        // 顶点顺序: 左下、右下、右上、左上
        f64 uvs[4][2] = {
            {layer.texture.u0, layer.texture.v1}, // 左下
            {layer.texture.u1, layer.texture.v1}, // 右下
            {layer.texture.u1, layer.texture.v0}, // 右上
            {layer.texture.u0, layer.texture.v0}  // 左上
        };

        // 叠加层沿法线轻微外移，避免与底层完全重合导致闪烁
        const f64 layerOffset = static_cast<f64>(layerIndex) * 0.001f;

        std::array<Vertex, 4> faceVerts;
        for (size_t i = 0; i < 4; ++i) {
            faceVerts[i] = Vertex(static_cast<f32>(x + vertices[i * 3 + 0] + normal[0] * layerOffset),
                static_cast<f32>(y + vertices[i * 3 + 1] + normal[1] * layerOffset),
                static_cast<f32>(z + vertices[i * 3 + 2] + normal[2] * layerOffset),
                static_cast<f32>(uvs[i][0]),
                static_cast<f32>(uvs[i][1]),
                shadedColor,
                packedLight);
        }

        // 添加顶点和索引
        u16 baseIndex = static_cast<u16>(mesh.vertices.size());
        for (const auto& v : faceVerts) {
            mesh.vertices.push_back(v);
        }

        auto indices = BlockGeometry::getFaceIndices();
        for (u16 idx : indices) {
            mesh.indices.push_back(static_cast<u16>(baseIndex + idx));
        }
    }
}

void ChunkMesher::_addFaceFromAppearanceSmooth(MeshData& mesh,
    Face face,
    f64 x,
    f64 y,
    f64 z,
    const ChunkData& chunk,
    i32 blockX,
    i32 blockY,
    i32 blockZ,
    const BlockState* block,
    const BlockAppearance* appearance,
    const ChunkData* neighborChunks[6])
{
    if (!appearance) {
        return;
    }

    // 查找面的纹理
    std::string faceName;
    switch (face) {
        case Face::Bottom:
            faceName = "down";
            break;
        case Face::Top:
            faceName = "up";
            break;
        case Face::North:
            faceName = "north";
            break;
        case Face::South:
            faceName = "south";
            break;
        case Face::West:
            faceName = "west";
            break;
        case Face::East:
            faceName = "east";
            break;
        default:
            return;
    }

    auto faceLayers = block != nullptr && block->isLiquid() ? collectLiquidFaceLayers(block, face)
                                                            : collectFaceLayers(appearance, faceName);
    if (faceLayers.empty()) {
        return;
    }

    auto normal = BlockGeometry::getFaceNormal(face);
    auto vertices = BlockGeometry::getFaceVertices(face);

    // 计算 AO
    client::renderer::AmbientOcclusionCalculator aoCalc;
    auto aoResult = aoCalc.calculate(chunk, blockX, blockY, blockZ, face, neighborChunks);

    if (block != nullptr && block->isLiquid()) {
        const u8 skyLight = _sampleSkyLight(chunk, blockX, blockY, blockZ, neighborChunks);
        const u8 blockLight = _sampleBlockLight(chunk, blockX, blockY, blockZ, neighborChunks);
        auto resolveTintColor = [](const client::ChunkBiomeAccessor& biomeAccessor,
                                    i32 worldX,
                                    i32 worldY,
                                    i32 worldZ,
                                    const BlockState* blockState,
                                    i32 tintIndex) {
            return _resolveTintColorBlended(biomeAccessor, worldX, worldY, worldZ, blockState, tintIndex);
        };
        emitLiquidFace(mesh,
            face,
            x,
            y,
            z,
            chunk,
            blockX,
            blockY,
            blockZ,
            skyLight,
            blockLight,
            block,
            faceLayers,
            neighborChunks,
            resolveTintColor);
        return;
    }

    // 创建生物群系访问器用于颜色混合
    std::array<const ChunkData*, 4> biomeNeighbors = {};
    if (neighborChunks) {
        biomeNeighbors[0] = neighborChunks[0]; // -X
        biomeNeighbors[1] = neighborChunks[1]; // +X
        biomeNeighbors[2] = neighborChunks[2]; // -Z
        biomeNeighbors[3] = neighborChunks[3]; // +Z
    }
    client::ChunkBiomeAccessor biomeAccessor(chunk, biomeNeighbors, chunk.x(), chunk.z());

    // 计算世界坐标用于生物群系颜色混合
    const i32 worldX = chunk.x() * world::CHUNK_WIDTH + blockX;
    const i32 worldZ = chunk.z() * world::CHUNK_WIDTH + blockZ;

    const f32 faceShade = getFaceShade(face);
    for (size_t layerIndex = 0; layerIndex < faceLayers.size(); ++layerIndex) {
        const auto& layer = faceLayers[layerIndex];
        const u32 tintColor = _resolveTintColorBlended(biomeAccessor, worldX, blockY, worldZ, block, layer.tintIndex);

        // UV坐标根据顶点位置设置
        // 顶点顺序: 左下、右下、右上、左上
        f64 uvs[4][2] = {
            {layer.texture.u0, layer.texture.v1}, // 左下
            {layer.texture.u1, layer.texture.v1}, // 右下
            {layer.texture.u1, layer.texture.v0}, // 右上
            {layer.texture.u0, layer.texture.v0}  // 左上
        };

        // 叠加层沿法线轻微外移，避免与底层完全重合导致闪烁
        const f64 layerOffset = static_cast<f64>(layerIndex) * 0.001f;

        // 创建4个顶点，每个顶点有独立的光照和AO
        std::array<Vertex, 4> faceVerts;
        for (size_t i = 0; i < 4; ++i) {
            // 打包双通道光照：高4位=天空光，低4位=方块光
            const u8 packedLight =
                static_cast<u8>(((aoResult.vertexSkyLight[i] & 0x0F) << 4) | (aoResult.vertexBlockLight[i] & 0x0F));

            // AO 乘数与面向明暗共同作用于顶点颜色。
            // 注意：颜色打包必须遵循 RGBA 字节顺序，避免出现偏色（例如偏红）问题。
            const f32 ao = aoResult.vertexColorMultiplier[i];
            const f32 finalShade = std::clamp(ao * faceShade, 0.0f, 1.0f);
            const u32 color = applyBlockAlpha(applyShadeToPackedColor(tintColor, finalShade), block);

            faceVerts[i] = Vertex(static_cast<f32>(x + vertices[i * 3 + 0] + normal[0] * layerOffset),
                static_cast<f32>(y + vertices[i * 3 + 1] + normal[1] * layerOffset),
                static_cast<f32>(z + vertices[i * 3 + 2] + normal[2] * layerOffset),
                static_cast<f32>(uvs[i][0]),
                static_cast<f32>(uvs[i][1]),
                color,
                packedLight);
        }

        // 添加顶点和索引
        u16 baseIndex = static_cast<u16>(mesh.vertices.size());
        for (const auto& v : faceVerts) {
            mesh.vertices.push_back(v);
        }

        auto indices = BlockGeometry::getFaceIndices();
        for (u16 idx : indices) {
            mesh.indices.push_back(static_cast<u16>(baseIndex + idx));
        }
    }
}

bool ChunkMesher::_isCrossLikeAppearance(const BlockAppearance* appearance)
{
    if (appearance == nullptr || appearance->elements.size() != 2) {
        return false;
    }

    // 交叉模型通常只有水平四个方向面（无上下）
    bool hasHorizontalFace = false;
    for (const auto& element : appearance->elements) {
        for (const auto& [dir, face] : element.faces) {
            (void)face;
            if (dir == Direction::Up || dir == Direction::Down) {
                return false;
            }
            if (dir == Direction::North || dir == Direction::South || dir == Direction::West ||
                dir == Direction::East) {
                hasHorizontalFace = true;
            }
        }
    }

    return hasHorizontalFace;
}

void ChunkMesher::_addCrossedPlantGeometry(MeshData& mesh,
    f64 x,
    f64 y,
    f64 z,
    const ChunkData& chunk,
    i32 blockX,
    i32 blockY,
    i32 blockZ,
    u8 skyLight,
    u8 blockLight,
    const BlockState* block,
    const BlockAppearance* appearance,
    const ChunkData* neighborChunks[6])
{
    if (appearance == nullptr || block == nullptr) {
        return;
    }

    auto layerA = collectFaceLayers(appearance, "north");
    if (layerA.empty()) {
        layerA = collectFaceLayers(appearance, "south");
    }
    if (layerA.empty()) {
        layerA = collectFaceLayers(appearance, "all");
    }

    auto layerB = collectFaceLayers(appearance, "west");
    if (layerB.empty()) {
        layerB = collectFaceLayers(appearance, "east");
    }
    if (layerB.empty()) {
        layerB = layerA;
    }

    if (layerA.empty() || layerB.empty()) {
        return;
    }

    std::array<const ChunkData*, 4> biomeNeighbors = {};
    if (neighborChunks) {
        biomeNeighbors[0] = neighborChunks[0];
        biomeNeighbors[1] = neighborChunks[1];
        biomeNeighbors[2] = neighborChunks[2];
        biomeNeighbors[3] = neighborChunks[3];
    }

    client::ChunkBiomeAccessor biomeAccessor(chunk, biomeNeighbors, chunk.x(), chunk.z());
    const i32 worldX = chunk.x() * world::CHUNK_WIDTH + blockX;
    const i32 worldZ = chunk.z() * world::CHUNK_WIDTH + blockZ;
    const u8 packedLight = static_cast<u8>(((skyLight & 0x0F) << 4) | (blockLight & 0x0F));

    constexpr f64 INV_SQRT2 = static_cast<f64>(mc::math::INV_SQRT2);
    auto emitDoubleSidedQuad = [&](const std::array<std::array<f64, 3>, 4>& positions,
                                   const std::array<f64, 3>& frontNormal,
                                   const FaceLayerRenderData& layer,
                                   u32 color,
                                   f64 layerOffset) {
        const f64 uvs[4][2] = {{layer.texture.u0, layer.texture.v1},
            {layer.texture.u1, layer.texture.v1},
            {layer.texture.u1, layer.texture.v0},
            {layer.texture.u0, layer.texture.v0}};

        auto pushFace = [&](bool reverse, const std::array<f64, 3>& normal) {
            std::array<Vertex, 4> verts;
            for (size_t i = 0; i < 4; ++i) {
                const size_t src = reverse ? (3 - i) : i;
                verts[i] = Vertex(static_cast<f32>(positions[src][0] + normal[0] * layerOffset),
                    static_cast<f32>(positions[src][1] + normal[1] * layerOffset),
                    static_cast<f32>(positions[src][2] + normal[2] * layerOffset),
                    static_cast<f32>(uvs[i][0]),
                    static_cast<f32>(uvs[i][1]),
                    color,
                    packedLight);
            }

            const u16 baseIndex = static_cast<u16>(mesh.vertices.size());
            for (const auto& v : verts) {
                mesh.vertices.push_back(v);
            }

            const auto indices = BlockGeometry::getFaceIndices();
            for (u16 idx : indices) {
                mesh.indices.push_back(static_cast<u16>(baseIndex + idx));
            }
        };

        pushFace(false, frontNormal);
        pushFace(true, {-frontNormal[0], -frontNormal[1], -frontNormal[2]});
    };

    const std::array<std::array<f64, 3>, 4> diagA = {{{x + 0.0f, y + 0.0f, z + 0.0f},
        {x + 1.0f, y + 0.0f, z + 1.0f},
        {x + 1.0f, y + 1.0f, z + 1.0f},
        {x + 0.0f, y + 1.0f, z + 0.0f}}};
    const std::array<std::array<f64, 3>, 4> diagB = {{{x + 1.0f, y + 0.0f, z + 0.0f},
        {x + 0.0f, y + 0.0f, z + 1.0f},
        {x + 0.0f, y + 1.0f, z + 1.0f},
        {x + 1.0f, y + 1.0f, z + 0.0f}}};

    for (size_t i = 0; i < layerA.size(); ++i) {
        const auto& layer = layerA[i];
        const u32 tintColor = _resolveTintColorBlended(biomeAccessor, worldX, blockY, worldZ, block, layer.tintIndex);
        const u32 color = applyBlockAlpha(tintColor, block);
        const f64 layerOffset = static_cast<f64>(i) * 0.001f;
        emitDoubleSidedQuad(diagA, {INV_SQRT2, 0.0f, -INV_SQRT2}, layer, color, layerOffset);
    }

    for (size_t i = 0; i < layerB.size(); ++i) {
        const auto& layer = layerB[i];
        const u32 tintColor = _resolveTintColorBlended(biomeAccessor, worldX, blockY, worldZ, block, layer.tintIndex);
        const u32 color = applyBlockAlpha(tintColor, block);
        const f64 layerOffset = static_cast<f64>(i) * 0.001f;
        emitDoubleSidedQuad(diagB, {-INV_SQRT2, 0.0f, -INV_SQRT2}, layer, color, layerOffset);
    }
}

void ChunkMesher::_addShapeGeometryFromAppearance(MeshData& mesh,
    f64 x,
    f64 y,
    f64 z,
    const ChunkData& chunk,
    i32 blockX,
    i32 blockY,
    i32 blockZ,
    const BlockState* block,
    const BlockAppearance* appearance,
    const CollisionShape& shape,
    const std::array<const BlockState*, 6>& neighborStates,
    const ChunkData* neighborChunks[6])
{
    if (block == nullptr || appearance == nullptr || shape.isEmpty() || shape.boxCount() != 1) {
        return;
    }

    const auto& box = shape.boxes().front();

    std::array<const ChunkData*, 4> biomeNeighbors = {};
    if (neighborChunks) {
        biomeNeighbors[0] = neighborChunks[0];
        biomeNeighbors[1] = neighborChunks[1];
        biomeNeighbors[2] = neighborChunks[2];
        biomeNeighbors[3] = neighborChunks[3];
    }
    client::ChunkBiomeAccessor biomeAccessor(chunk, biomeNeighbors, chunk.x(), chunk.z());
    const i32 worldX = chunk.x() * world::CHUNK_WIDTH + blockX;
    const i32 worldZ = chunk.z() * world::CHUNK_WIDTH + blockZ;

    for (size_t faceIdx = 0; faceIdx < 6; ++faceIdx) {
        const Face face = static_cast<Face>(faceIdx);
        const BlockState* neighbor = neighborStates[faceIdx];
        if (!_shouldRenderFace(block, neighbor, face)) {
            continue;
        }

        const bool neighborOpaque =
            neighbor != nullptr && !neighbor->isAir() && !neighbor->isTransparent() && !neighbor->isLiquid();

        if (neighborOpaque) {
            constexpr f32 EPSILON = 1.0e-4f;
            const bool flushWithNeighbor = (face == Face::Bottom && box.minY <= EPSILON) ||
                (face == Face::Top && box.maxY >= 1.0f - EPSILON) || (face == Face::North && box.minZ <= EPSILON) ||
                (face == Face::South && box.maxZ >= 1.0f - EPSILON) || (face == Face::West && box.minX <= EPSILON) ||
                (face == Face::East && box.maxX >= 1.0f - EPSILON);
            if (flushWithNeighbor) {
                continue;
            }
        }

        std::string faceName;
        switch (face) {
            case Face::Bottom:
                faceName = "down";
                break;
            case Face::Top:
                faceName = "up";
                break;
            case Face::North:
                faceName = "north";
                break;
            case Face::South:
                faceName = "south";
                break;
            case Face::West:
                faceName = "west";
                break;
            case Face::East:
                faceName = "east";
                break;
            default:
                continue;
        }

        auto faceLayers = collectFaceLayers(appearance, faceName);
        if (faceLayers.empty()) {
            continue;
        }

        u8 skyLight = 15;
        u8 blockLight = 0;
        if (s_lightingEnabled) {
            const auto dir = BlockGeometry::getFaceDirection(face);
            const i32 sampleX = blockX + dir[0];
            const i32 sampleY = blockY + dir[1];
            const i32 sampleZ = blockZ + dir[2];

            if (block->isTransparent() && neighbor != nullptr && !neighbor->isAir() && !neighbor->isTransparent()) {
                skyLight = _sampleSkyLight(chunk, blockX, blockY, blockZ, neighborChunks);
                blockLight = _sampleBlockLight(chunk, blockX, blockY, blockZ, neighborChunks);
            } else {
                skyLight = _sampleSkyLight(chunk, sampleX, sampleY, sampleZ, neighborChunks);
                blockLight = _sampleBlockLight(chunk, sampleX, sampleY, sampleZ, neighborChunks);
            }
        }

        const u8 packedLight = static_cast<u8>(((skyLight & 0x0F) << 4) | (blockLight & 0x0F));
        const auto normal = BlockGeometry::getFaceNormal(face);

        const f64 x0 = x + box.minX;
        const f64 y0 = y + box.minY;
        const f64 z0 = z + box.minZ;
        const f64 x1 = x + box.maxX;
        const f64 y1 = y + box.maxY;
        const f64 z1 = z + box.maxZ;

        std::array<std::array<f64, 3>, 4> positions{};
        switch (face) {
            case Face::Bottom:
                positions = {{{x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}}};
                break;
            case Face::Top:
                positions = {{{x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0}}};
                break;
            case Face::North:
                positions = {{{x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}}};
                break;
            case Face::South:
                positions = {{{x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}}};
                break;
            case Face::West:
                positions = {{{x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}}};
                break;
            case Face::East:
                positions = {{{x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}}};
                break;
            default:
                continue;
        }

        for (size_t layerIndex = 0; layerIndex < faceLayers.size(); ++layerIndex) {
            const auto& layer = faceLayers[layerIndex];
            const u32 tintColor =
                _resolveTintColorBlended(biomeAccessor, worldX, blockY, worldZ, block, layer.tintIndex);
            const u32 shadedColor = applyBlockAlpha(applyShadeToPackedColor(tintColor, getFaceShade(face)), block);

            const f64 uvs[4][2] = {{layer.texture.u0, layer.texture.v1},
                {layer.texture.u1, layer.texture.v1},
                {layer.texture.u1, layer.texture.v0},
                {layer.texture.u0, layer.texture.v0}};

            const f64 layerOffset = static_cast<f64>(layerIndex) * 0.001f;

            std::array<Vertex, 4> faceVerts;
            for (size_t i = 0; i < 4; ++i) {
                faceVerts[i] = Vertex(static_cast<f32>(positions[i][0] + normal[0] * layerOffset),
                    static_cast<f32>(positions[i][1] + normal[1] * layerOffset),
                    static_cast<f32>(positions[i][2] + normal[2] * layerOffset),
                    static_cast<f32>(uvs[i][0]),
                    static_cast<f32>(uvs[i][1]),
                    shadedColor,
                    packedLight);
            }

            const u16 baseIndex = static_cast<u16>(mesh.vertices.size());
            for (const auto& vert : faceVerts) {
                mesh.vertices.push_back(vert);
            }

            const auto indices = BlockGeometry::getFaceIndices();
            for (u16 idx : indices) {
                mesh.indices.push_back(static_cast<u16>(baseIndex + idx));
            }
        }
    }
}

void ChunkMesher::_simpleMeshSection(const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6],
    const std::atomic<bool>* abortSignal)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.ChunkMesh, "SimplyGenerateSectionMesh", "phase", "simple");
    // 必须有 BlockModelCache
    if (!s_modelCache) {
        spdlog::error("ChunkMesher: BlockModelCache not initialized, cannot generate mesh");
        return;
    }

    constexpr i32 SIZE = world::CHUNK_SECTION_HEIGHT;
    const i32 baseY = world::sectionToY(sectionIndex);

    // 获取当前段
    const ChunkSection* section = chunk.getSection(sectionIndex);
    if (!section || section->isEmpty()) {
        return;
    }

    if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
        outMesh.clear();
        return;
    }

    const auto getNeighborBlock = [&](i32 x, i32 y, i32 z, Face face) -> const BlockState* {
        const auto dir = BlockGeometry::getFaceDirection(face);
        const i32 nx = x + dir[0];
        const i32 ny = y + dir[1];
        const i32 nz = z + dir[2];

        if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && nz >= 0 && nz < SIZE) {
            return section->getBlockState(nx, ny, nz);
        }

        const i32 worldX = nx;
        const i32 worldY = baseY + ny;
        const i32 worldZ = nz;

        if (worldY < world::MIN_BUILD_HEIGHT || worldY >= world::MAX_BUILD_HEIGHT) {
            return nullptr;
        }

        if (worldX >= 0 && worldX < world::CHUNK_WIDTH && worldZ >= 0 && worldZ < world::CHUNK_WIDTH) {
            return chunk.getBlockState(worldX, worldY, worldZ);
        }

        if (!neighborChunks) {
            return nullptr;
        }

        i32 neighborIdx = -1;
        if (worldX < 0) {
            neighborIdx = 0; // -X
        } else if (worldX >= SIZE) {
            neighborIdx = 1; // +X
        } else if (worldZ < 0) {
            neighborIdx = 2; // -Z
        } else if (worldZ >= SIZE) {
            neighborIdx = 3; // +Z
        }

        if (neighborIdx < 0 || !neighborChunks[neighborIdx]) {
            return nullptr;
        }

        const i32 lx = (worldX + SIZE) % SIZE;
        const i32 lz = (worldZ + SIZE) % SIZE;
        return neighborChunks[neighborIdx]->getBlockState(lx, worldY, lz);
    };

    // 遍历段内所有方块
    for (i32 y = 0; y < SIZE; ++y) {
        if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
            outMesh.clear();
            return;
        }

        for (i32 z = 0; z < SIZE; ++z) {
            for (i32 x = 0; x < SIZE; ++x) {
                if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
                    outMesh.clear();
                    return;
                }

                const BlockState* block = section->getBlockState(x, y, z);
                if (!_shouldRenderBlock(block)) {
                    continue;
                }

                // 获取方块外观
                const BlockAppearance* appearance = s_modelCache->getBlockAppearance(block);
                if (!appearance) {
                    // 使用缺失模型
                    appearance = s_modelCache->getMissingAppearance();
                    if (!appearance) {
                        continue; // 无法渲染
                    }
                }

                // 检查外观是否有效
                if (appearance->elements.empty() && appearance->faceTextures.empty() && !block->isLiquid()) {
                    continue;
                }

                if (_isCrossLikeAppearance(appearance)) {
                    u8 skyLight = 15;
                    u8 blockLight = 0;
                    if (s_lightingEnabled) {
                        skyLight = _sampleSkyLight(chunk, x, baseY + y, z, neighborChunks);
                        blockLight = _sampleBlockLight(chunk, x, baseY + y, z, neighborChunks);
                    }

                    _addCrossedPlantGeometry(outMesh,
                        static_cast<f64>(x),
                        static_cast<f64>(baseY + y),
                        static_cast<f64>(z),
                        chunk,
                        x,
                        baseY + y,
                        z,
                        skyLight,
                        blockLight,
                        block,
                        appearance,
                        neighborChunks);
                    continue;
                }

                const CollisionShape& blockShape = block->getShape();
                const bool useShapeFallback = !block->isLiquid() && !blockShape.isEmpty() &&
                    !blockShape.isFullBlock() && blockShape.boxCount() == 1;

                if (useShapeFallback) {
                    std::array<const BlockState*, 6> neighborStates = {};
                    for (size_t faceIdx = 0; faceIdx < 6; ++faceIdx) {
                        const Face face = static_cast<Face>(faceIdx);
                        neighborStates[faceIdx] = getNeighborBlock(x, y, z, face);
                    }

                    _addShapeGeometryFromAppearance(outMesh,
                        static_cast<f64>(x),
                        static_cast<f64>(baseY + y),
                        static_cast<f64>(z),
                        chunk,
                        x,
                        baseY + y,
                        z,
                        block,
                        appearance,
                        blockShape,
                        neighborStates,
                        neighborChunks);
                    continue;
                }

                // 检查每个面
                for (size_t faceIdx = 0; faceIdx < 6; ++faceIdx) {
                    Face face = static_cast<Face>(faceIdx);
                    auto dir = BlockGeometry::getFaceDirection(face);
                    const BlockState* neighbor = getNeighborBlock(x, y, z, face);

                    // 决定是否渲染该面
                    if (!_shouldRenderFace(block, neighbor, face)) {
                        continue;
                    } else {
                        const f64 fx = static_cast<f64>(x);
                        const f64 fy = static_cast<f64>(baseY + y);
                        const f64 fz = static_cast<f64>(z);

                        if (s_lightingMode == LightingMode::Smooth && s_lightingEnabled) {
                            // 平滑光照模式：使用AO计算
                            _addFaceFromAppearanceSmooth(
                                outMesh, face, fx, fy, fz, chunk, x, baseY + y, z, block, appearance, neighborChunks);
                        } else {
                            // 平面光照模式
                            u8 skyLight = 15;  // 默认天空光
                            u8 blockLight = 0; // 默认方块光
                            if (s_lightingEnabled) {
                                const i32 sampleX = x + dir[0];
                                const i32 sampleY = baseY + y + dir[1];
                                const i32 sampleZ = z + dir[2];

                                // 对于透明方块的边界面，如果邻居是不透明方块，
                                // 使用方块自身位置的光照，避免采样到不透明方块内部的黑色
                                // 这解决了草方块底部渲染为黑色的问题
                                if (block->isTransparent() && neighbor && !neighbor->isAir() &&
                                    !neighbor->isTransparent()) {
                                    // 采样方块自身位置的光照
                                    skyLight = _sampleSkyLight(chunk, x, baseY + y, z, neighborChunks);
                                    blockLight = _sampleBlockLight(chunk, x, baseY + y, z, neighborChunks);
                                } else {
                                    // 正常情况：采样邻居位置的光照
                                    skyLight = _sampleSkyLight(chunk, sampleX, sampleY, sampleZ, neighborChunks);
                                    blockLight = _sampleBlockLight(chunk, sampleX, sampleY, sampleZ, neighborChunks);
                                }
                            }

                            _addFaceFromAppearance(outMesh,
                                face,
                                fx,
                                fy,
                                fz,
                                chunk,
                                x,
                                baseY + y,
                                z,
                                skyLight,
                                blockLight,
                                block,
                                appearance,
                                neighborChunks);
                        }
                    }
                }
            }
        }
    }
}

void ChunkMesher::_greedyMeshSection(const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6],
    const std::atomic<bool>* abortSignal)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.ChunkMesh, "GreedyGenerateSectionMesh", "phase", "greedy");

    // 必须有 BlockModelCache
    if (!s_modelCache) {
        spdlog::error("ChunkMesher: BlockModelCache not initialized, cannot generate greedy mesh");
        return;
    }

    constexpr i32 SIZE = world::CHUNK_SECTION_HEIGHT;
    const i32 baseY = world::sectionToY(sectionIndex);

    const ChunkSection* section = chunk.getSection(sectionIndex);
    if (!section || section->isEmpty()) {
        return;
    }

    // 平滑 AO 依赖逐顶点采样，无法在不引入插值误差的前提下做大面合并，直接回退到逐面路径。
    if (s_lightingMode == LightingMode::Smooth && s_lightingEnabled) {
        _simpleMeshSection(chunk, sectionIndex, outMesh, neighborChunks, abortSignal);
        return;
    }

    // 贪婪合并无法正确表达交叉植物与非完整体素方块，回退到逐方块路径保证几何正确。
    for (i32 y = 0; y < SIZE; ++y) {
        if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
            outMesh.clear();
            return;
        }

        for (i32 z = 0; z < SIZE; ++z) {
            for (i32 x = 0; x < SIZE; ++x) {
                if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
                    outMesh.clear();
                    return;
                }

                const BlockState* block = section->getBlockState(x, y, z);
                if (!_shouldRenderBlock(block)) {
                    continue;
                }

                const BlockAppearance* appearance = s_modelCache->getBlockAppearance(block);
                if (appearance == nullptr) {
                    appearance = s_modelCache->getMissingAppearance();
                }

                if (appearance != nullptr && _isCrossLikeAppearance(appearance)) {
                    _simpleMeshSection(chunk, sectionIndex, outMesh, neighborChunks, abortSignal);
                    return;
                }

                const CollisionShape& shape = block->getShape();
                if (!block->isLiquid() && !shape.isEmpty() && !shape.isFullBlock()) {
                    _simpleMeshSection(chunk, sectionIndex, outMesh, neighborChunks, abortSignal);
                    return;
                }
            }
        }
    }

    // 创建生物群系访问器用于颜色混合
    std::array<const ChunkData*, 4> biomeNeighbors = {};
    if (neighborChunks) {
        biomeNeighbors[0] = neighborChunks[0]; // -X
        biomeNeighbors[1] = neighborChunks[1]; // +X
        biomeNeighbors[2] = neighborChunks[2]; // -Z
        biomeNeighbors[3] = neighborChunks[3]; // +Z
    }
    client::ChunkBiomeAccessor biomeAccessor(chunk, biomeNeighbors, chunk.x(), chunk.z());

    struct FaceCellData {
        bool visible = false;
        const BlockState* block = nullptr;
        const BlockAppearance* appearance = nullptr;
        std::vector<FaceLayerRenderData> layers;
        std::vector<u32> shadedLayerColors;
        u8 skyLight = 15;
        u8 blockLight = 0;
    };

    const auto sameTextureRegion = [](const TextureRegion& a, const TextureRegion& b) {
        constexpr f32 EPSILON = 1e-6f;
        return std::abs(a.u0 - b.u0) <= EPSILON && std::abs(a.v0 - b.v0) <= EPSILON &&
            std::abs(a.u1 - b.u1) <= EPSILON && std::abs(a.v1 - b.v1) <= EPSILON;
    };

    const auto sameCell = [&](const FaceCellData& a, const FaceCellData& b) {
        if (!a.visible || !b.visible) {
            return false;
        }

        if (a.block != b.block || a.appearance != b.appearance || a.skyLight != b.skyLight ||
            a.blockLight != b.blockLight || a.layers.size() != b.layers.size() ||
            a.shadedLayerColors.size() != b.shadedLayerColors.size()) {
            return false;
        }

        for (size_t i = 0; i < a.layers.size(); ++i) {
            if (a.layers[i].tintIndex != b.layers[i].tintIndex ||
                !sameTextureRegion(a.layers[i].texture, b.layers[i].texture)) {
                return false;
            }
        }

        for (size_t i = 0; i < a.shadedLayerColors.size(); ++i) {
            if (a.shadedLayerColors[i] != b.shadedLayerColors[i]) {
                return false;
            }
        }

        return true;
    };

    const auto getNeighborBlock = [&](i32 x, i32 y, i32 z, Face face) -> const BlockState* {
        const auto dir = BlockGeometry::getFaceDirection(face);
        const i32 nx = x + dir[0];
        const i32 ny = y + dir[1];
        const i32 nz = z + dir[2];

        if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && nz >= 0 && nz < SIZE) {
            return section->getBlockState(nx, ny, nz);
        }

        const i32 worldX = nx;
        const i32 worldY = baseY + ny;
        const i32 worldZ = nz;

        if (worldY < world::MIN_BUILD_HEIGHT || worldY >= world::MAX_BUILD_HEIGHT) {
            return nullptr;
        }

        if (worldX >= 0 && worldX < world::CHUNK_WIDTH && worldZ >= 0 && worldZ < world::CHUNK_WIDTH) {
            return chunk.getBlockState(worldX, worldY, worldZ);
        }

        if (!neighborChunks) {
            return nullptr;
        }

        i32 neighborIdx = -1;
        if (worldX < 0) {
            neighborIdx = 0; // -X
        } else if (worldX >= SIZE) {
            neighborIdx = 1; // +X
        } else if (worldZ < 0) {
            neighborIdx = 2; // -Z
        } else if (worldZ >= SIZE) {
            neighborIdx = 3; // +Z
        }

        if (neighborIdx < 0 || !neighborChunks[neighborIdx]) {
            return nullptr;
        }

        const i32 lx = (worldX + SIZE) % SIZE;
        const i32 lz = (worldZ + SIZE) % SIZE;
        return neighborChunks[neighborIdx]->getBlockState(lx, worldY, lz);
    };

    const auto buildCellData = [&](Face face, i32 x, i32 y, i32 z) -> FaceCellData {
        FaceCellData cell;

        const BlockState* block = section->getBlockState(x, y, z);
        if (!_shouldRenderBlock(block)) {
            return cell;
        }

        const BlockState* neighbor = getNeighborBlock(x, y, z, face);
        if (!_shouldRenderFace(block, neighbor, face)) {
            return cell;
        }

        const BlockAppearance* appearance = s_modelCache->getBlockAppearance(block);
        if (!appearance) {
            appearance = s_modelCache->getMissingAppearance();
        }
        if (!appearance) {
            return cell;
        }

        if (appearance->elements.empty() && appearance->faceTextures.empty() && !block->isLiquid()) {
            return cell;
        }

        if (_isCrossLikeAppearance(appearance)) {
            return cell;
        }

        std::string faceName;
        switch (face) {
            case Face::Bottom:
                faceName = "down";
                break;
            case Face::Top:
                faceName = "up";
                break;
            case Face::North:
                faceName = "north";
                break;
            case Face::South:
                faceName = "south";
                break;
            case Face::West:
                faceName = "west";
                break;
            case Face::East:
                faceName = "east";
                break;
            default:
                return cell;
        }

        auto faceLayers = collectFaceLayers(appearance, faceName);
        if (faceLayers.empty()) {
            faceLayers = collectLiquidFaceLayers(block, face);
        }
        if (faceLayers.empty()) {
            return cell;
        }

        u8 skyLight = 15;
        u8 blockLight = 0;
        if (s_lightingEnabled) {
            const auto dir = BlockGeometry::getFaceDirection(face);
            const i32 worldY = baseY + y;
            const i32 sampleX = x + dir[0];
            const i32 sampleY = worldY + dir[1];
            const i32 sampleZ = z + dir[2];

            // 对透明方块接触不透明邻居时，采样自身光照，避免采样到邻居内部导致黑边
            if (block->isTransparent() && neighbor && !neighbor->isAir() && !neighbor->isTransparent()) {
                skyLight = _sampleSkyLight(chunk, x, worldY, z, neighborChunks);
                blockLight = _sampleBlockLight(chunk, x, worldY, z, neighborChunks);
            } else {
                skyLight = _sampleSkyLight(chunk, sampleX, sampleY, sampleZ, neighborChunks);
                blockLight = _sampleBlockLight(chunk, sampleX, sampleY, sampleZ, neighborChunks);
            }
        }

        // 计算世界坐标用于生物群系颜色混合
        const i32 worldX = chunk.x() * world::CHUNK_WIDTH + x;
        const i32 worldZ = chunk.z() * world::CHUNK_WIDTH + z;
        const i32 worldY = baseY + y;

        std::vector<u32> shadedLayerColors;
        shadedLayerColors.reserve(faceLayers.size());
        const f32 faceShade = getFaceShade(face);
        for (const auto& layer : faceLayers) {
            const u32 tintColor =
                _resolveTintColorBlended(biomeAccessor, worldX, worldY, worldZ, block, layer.tintIndex);
            shadedLayerColors.push_back(applyBlockAlpha(applyShadeToPackedColor(tintColor, faceShade), block));
        }

        cell.visible = true;
        cell.block = block;
        cell.appearance = appearance;
        cell.layers = std::move(faceLayers);
        cell.shadedLayerColors = std::move(shadedLayerColors);
        cell.skyLight = skyLight;
        cell.blockLight = blockLight;
        return cell;
    };

    const auto emitMergedFace = [&](Face face, i32 d, i32 u, i32 v, i32 width, i32 height, const FaceCellData& cell) {
        const auto normal = BlockGeometry::getFaceNormal(face);
        const u8 packedLight = static_cast<u8>(((cell.skyLight & 0x0F) << 4) | (cell.blockLight & 0x0F));

        for (size_t layerIndex = 0; layerIndex < cell.layers.size(); ++layerIndex) {
            const auto& layer = cell.layers[layerIndex];
            const u32 color = cell.shadedLayerColors[layerIndex];

            f64 x0 = 0.0f;
            f64 x1 = 0.0f;
            f64 y0 = 0.0f;
            f64 y1 = 0.0f;
            f64 z0 = 0.0f;
            f64 z1 = 0.0f;

            switch (face) {
                case Face::Bottom:
                case Face::Top:
                    x0 = static_cast<f64>(u);
                    x1 = static_cast<f64>(u + width);
                    z0 = static_cast<f64>(v);
                    z1 = static_cast<f64>(v + height);
                    y0 = static_cast<f64>(baseY + d + (face == Face::Top ? 1 : 0));
                    y1 = y0;
                    break;

                case Face::North:
                case Face::South:
                    x0 = static_cast<f64>(u);
                    x1 = static_cast<f64>(u + width);
                    y0 = static_cast<f64>(baseY + v);
                    y1 = static_cast<f64>(baseY + v + height);
                    z0 = static_cast<f64>(d + (face == Face::South ? 1 : 0));
                    z1 = z0;
                    break;

                case Face::West:
                case Face::East:
                    z0 = static_cast<f64>(u);
                    z1 = static_cast<f64>(u + width);
                    y0 = static_cast<f64>(baseY + v);
                    y1 = static_cast<f64>(baseY + v + height);
                    x0 = static_cast<f64>(d + (face == Face::East ? 1 : 0));
                    x1 = x0;
                    break;

                default:
                    return;
            }

            std::array<std::array<f64, 3>, 4> positions{};
            switch (face) {
                case Face::Bottom:
                    positions = {{{x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}}};
                    break;
                case Face::Top:
                    positions = {{{x0, y0, z1}, {x1, y0, z1}, {x1, y0, z0}, {x0, y0, z0}}};
                    break;
                case Face::North:
                    positions = {{{x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}}};
                    break;
                case Face::South:
                    positions = {{{x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z0}}};
                    break;
                case Face::West:
                    positions = {{{x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}}};
                    break;
                case Face::East:
                    positions = {{{x0, y0, z1}, {x0, y0, z0}, {x0, y1, z0}, {x0, y1, z1}}};
                    break;
                default:
                    return;
            }

            // UV 仍按单方块面的标准布局，保持与现有渲染路径一致。
            // 若后续需要平铺纹理，可扩展顶点格式传递 UV 缩放参数。
            const f64 uvs[4][2] = {{layer.texture.u0, layer.texture.v1},
                {layer.texture.u1, layer.texture.v1},
                {layer.texture.u1, layer.texture.v0},
                {layer.texture.u0, layer.texture.v0}};

            const f64 layerOffset = static_cast<f64>(layerIndex) * 0.001f;

            std::array<Vertex, 4> faceVerts;
            for (size_t i = 0; i < 4; ++i) {
                faceVerts[i] = Vertex(static_cast<f32>(positions[i][0] + normal[0] * layerOffset),
                    static_cast<f32>(positions[i][1] + normal[1] * layerOffset),
                    static_cast<f32>(positions[i][2] + normal[2] * layerOffset),
                    static_cast<f32>(uvs[i][0]),
                    static_cast<f32>(uvs[i][1]),
                    color,
                    packedLight);
            }

            const u16 baseIndex = static_cast<u16>(outMesh.vertices.size());
            for (const auto& vert : faceVerts) {
                outMesh.vertices.push_back(vert);
            }

            const auto indices = BlockGeometry::getFaceIndices();
            for (u16 idx : indices) {
                outMesh.indices.push_back(static_cast<u16>(baseIndex + idx));
            }
        }
    };

    constexpr i32 MASK_WIDTH = SIZE;
    constexpr i32 MASK_HEIGHT = SIZE;
    std::vector<FaceCellData> mask(static_cast<size_t>(MASK_WIDTH * MASK_HEIGHT));
    std::vector<bool> visited(static_cast<size_t>(MASK_WIDTH * MASK_HEIGHT), false);

    for (size_t faceIdx = 0; faceIdx < 6; ++faceIdx) {
        if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
            outMesh.clear();
            return;
        }

        const Face face = static_cast<Face>(faceIdx);

        for (i32 d = 0; d < SIZE; ++d) {
            if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
                outMesh.clear();
                return;
            }

            for (i32 v = 0; v < MASK_HEIGHT; ++v) {
                for (i32 u = 0; u < MASK_WIDTH; ++u) {
                    if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
                        outMesh.clear();
                        return;
                    }

                    i32 x = 0;
                    i32 y = 0;
                    i32 z = 0;

                    switch (face) {
                        case Face::Bottom:
                        case Face::Top:
                            x = u;
                            y = d;
                            z = v;
                            break;
                        case Face::North:
                        case Face::South:
                            x = u;
                            y = v;
                            z = d;
                            break;
                        case Face::West:
                        case Face::East:
                            x = d;
                            y = v;
                            z = u;
                            break;
                        default:
                            break;
                    }

                    const size_t idx = static_cast<size_t>(v * MASK_WIDTH + u);
                    mask[idx] = buildCellData(face, x, y, z);
                    visited[idx] = false;
                }
            }

            for (i32 v = 0; v < MASK_HEIGHT; ++v) {
                for (i32 u = 0; u < MASK_WIDTH; ++u) {
                    if (abortSignal && abortSignal->load(std::memory_order::acquire)) {
                        outMesh.clear();
                        return;
                    }

                    const size_t startIdx = static_cast<size_t>(v * MASK_WIDTH + u);
                    if (visited[startIdx] || !mask[startIdx].visible) {
                        continue;
                    }

                    const FaceCellData& seed = mask[startIdx];

                    i32 width = 1;
                    while (u + width < MASK_WIDTH) {
                        const size_t idx = static_cast<size_t>(v * MASK_WIDTH + (u + width));
                        if (visited[idx] || !sameCell(seed, mask[idx])) {
                            break;
                        }
                        ++width;
                    }

                    i32 height = 1;
                    bool canExpand = true;
                    while (v + height < MASK_HEIGHT && canExpand) {
                        for (i32 k = 0; k < width; ++k) {
                            const size_t idx = static_cast<size_t>((v + height) * MASK_WIDTH + (u + k));
                            if (visited[idx] || !sameCell(seed, mask[idx])) {
                                canExpand = false;
                                break;
                            }
                        }

                        if (canExpand) {
                            ++height;
                        }
                    }

                    for (i32 dy = 0; dy < height; ++dy) {
                        for (i32 dx = 0; dx < width; ++dx) {
                            const size_t idx = static_cast<size_t>((v + dy) * MASK_WIDTH + (u + dx));
                            visited[idx] = true;
                        }
                    }

                    emitMergedFace(face, d, u, v, width, height, seed);
                }
            }
        }
    }
}

} // namespace mc
