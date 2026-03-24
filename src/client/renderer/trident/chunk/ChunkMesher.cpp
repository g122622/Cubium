#include "ChunkMesher.hpp"
#include "AmbientOcclusionCalculator.hpp"
#include "../../../resource/BlockModelCache.hpp"
#include "../../../../common/perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cassert>
#include <cmath>

namespace mc {

// ============================================================================
// 静态成员初始化
// ============================================================================

BlockModelCache* ChunkMesher::s_modelCache = nullptr;
bool ChunkMesher::s_useGreedyMeshing = false;
bool ChunkMesher::s_lightingEnabled = true;
LightingMode ChunkMesher::s_lightingMode = LightingMode::Smooth;

namespace {

/**
 * @brief 将 RGBA 分量打包为顶点颜色（与 VK_FORMAT_R8G8B8A8_UNORM 对齐）
 *
 * @note 低字节为 R，高字节为 A。这样在小端内存布局下可与 Vulkan 直接对应。
 */
[[nodiscard]] constexpr u32 packVertexColor(u8 r, u8 g, u8 b, u8 a) {
    return static_cast<u32>(r)
        | (static_cast<u32>(g) << 8)
        | (static_cast<u32>(b) << 16)
        | (static_cast<u32>(a) << 24);
}

/**
 * @brief 返回与 MC 1.16.5 对齐的面向明暗系数
 *
 * 参考: IBlockDisplayReader#func_230487_a_ 的默认面向亮度
 * DOWN=0.5, UP=1.0, NORTH/SOUTH=0.8, WEST/EAST=0.6
 */
[[nodiscard]] constexpr float getFaceShade(Face face) {
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

/**
 * @brief 将亮度因子转换为灰度顶点色（RGB 同值，A 固定 255）
 */
[[nodiscard]] u32 makeGrayscaleVertexColor(float factor) {
    assert(factor >= -0.01f && factor <= 1.01f);
    const float clamped = std::clamp(factor, 0.0f, 1.0f);
    const u8 channel = static_cast<u8>(std::round(clamped * 255.0f));
    return packVertexColor(channel, channel, channel, 255);
}

} // namespace

// ============================================================================
// ChunkMesher 实现
// ============================================================================

void ChunkMesher::setModelCache(BlockModelCache* cache) {
    s_modelCache = cache;
    if (cache) {
        spdlog::info("ChunkMesher: Using BlockModelCache for block appearances");
    } else {
        spdlog::warn("ChunkMesher: BlockModelCache set to null, no models will be rendered");
    }
}

void ChunkMesher::generateMesh(
    const ChunkData& chunk,
    MeshData& outMesh,
    const ChunkData* neighbors[6]
) {
    MC_TRACE_CHUNK_MESH_EVENT("GenerateMesh");

    outMesh.clear();

    // 基于非空气方块数量进行预估，避免按最坏情况一次性分配过大内存。
    // 经验值：每个非空气方块平均约 2 个可见面（地形场景）。
    constexpr size_t MIN_ESTIMATED_FACES = 1024;
    constexpr size_t MAX_ESTIMATED_FACES = 16384; // 65536 顶点 / 4
    size_t nonAirBlockCount = 0;
    for (i32 sectionY = 0; sectionY < ChunkData::SECTIONS; ++sectionY) {
        const ChunkSection* section = chunk.getSection(sectionY);
        if (section && !section->isEmpty()) {
            nonAirBlockCount += section->getBlockCount();
        }
    }

    const size_t estimatedFaces = std::clamp(
        nonAirBlockCount * 2ULL,
        MIN_ESTIMATED_FACES,
        MAX_ESTIMATED_FACES
    );
    outMesh.reserve(estimatedFaces * BlockGeometry::VERTICES_PER_FACE,
                    estimatedFaces * BlockGeometry::INDICES_PER_FACE);

    // 遍历所有区块段
    for (i32 sectionY = 0; sectionY < ChunkData::SECTIONS; ++sectionY) {
        if (chunk.hasSection(sectionY)) {
            generateSectionMesh(chunk, sectionY, outMesh, neighbors);
        }
    }
}

void ChunkMesher::generateSectionMesh(
    const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6]
) {
    if (s_useGreedyMeshing) {
        greedyMeshSection(chunk, sectionIndex, outMesh, neighborChunks);
    } else {
        simpleMeshSection(chunk, sectionIndex, outMesh, neighborChunks);
    }
}

bool ChunkMesher::shouldRenderBlock(const BlockState* state) {
    // 空气和液体不渲染实体网格
    // 液体由 FluidMesher 单独处理
    if (!state) return false;
    if (state->isAir()) return false;
    if (state->isLiquid()) return false;  // 液体由 FluidMesher 处理
    return true;
}

bool ChunkMesher::shouldRenderFace(const BlockState* block, const BlockState* neighbor) {
    if (!block) {
        return false;
    }

    // 邻居是空气（或越界）时渲染外露面
    if (!neighbor || neighbor->isAir()) {
        return true;
    }

    // 相同状态对象之间不渲染内部面（State 通常为注册表共享单例）
    if (block == neighbor) {
        return false;
    }

    // 液体特殊规则：
    // - 同类型液体之间不渲染内部面（海洋场景显存暴涨的主要来源）
    // - 不同类型液体（如水/岩浆）之间保留边界面
    if (block->isLiquid()) {
        if (neighbor->isLiquid()) {
            return block->blockId() != neighbor->blockId();
        }
        return true;
    }

    // 非液体方块与液体相邻，需要渲染交界面
    if (neighbor->isLiquid()) {
        return true;
    }

    // 透明方块规则：同类型透明方块之间剔除内部面
    if (neighbor->isTransparent()) {
        if (block->isTransparent() && block->blockId() == neighbor->blockId()) {
            return false;
        }
        return true;
    }

    // 与不透明实心邻居相接，不渲染
    return false;
}

u8 ChunkMesher::sampleCombinedLight(
    const ChunkData& chunk,
    i32 x, i32 y, i32 z,
    const ChunkData* neighborChunks[6]
) {
    return std::max(
        sampleSkyLight(chunk, x, y, z, neighborChunks),
        sampleBlockLight(chunk, x, y, z, neighborChunks)
    );
}

u8 ChunkMesher::sampleSkyLight(
    const ChunkData& chunk,
    i32 x, i32 y, i32 z,
    const ChunkData* neighborChunks[6]
) {
    constexpr i32 SIZE = ChunkSection::SIZE;

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

u8 ChunkMesher::sampleBlockLight(
    const ChunkData& chunk,
    i32 x, i32 y, i32 z,
    const ChunkData* neighborChunks[6]
) {
    constexpr i32 SIZE = ChunkSection::SIZE;

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

void ChunkMesher::addFaceFromAppearance(
    MeshData& mesh,
    Face face,
    f32 x, f32 y, f32 z,
    u8 skyLight,
    u8 blockLight,
    const BlockAppearance* appearance
) {
    if (!appearance) {
        return;
    }

    // 检查是否有 elements
    if (appearance->elements.empty()) {
        return;
    }

    // 查找面的纹理
    String faceName;
    switch (face) {
        case Face::Bottom: faceName = "down"; break;
        case Face::Top: faceName = "up"; break;
        case Face::North: faceName = "north"; break;
        case Face::South: faceName = "south"; break;
        case Face::West: faceName = "west"; break;
        case Face::East: faceName = "east"; break;
        default: return;
    }

    auto texIt = appearance->faceTextures.find(faceName);
    if (texIt == appearance->faceTextures.end()) {
        // 尝试使用 "side" 作为后备
        texIt = appearance->faceTextures.find("side");
        if (texIt == appearance->faceTextures.end()) {
            // 尝试使用 "all" 作为后备
            texIt = appearance->faceTextures.find("all");
            if (texIt == appearance->faceTextures.end()) {
                return; // 没有找到纹理，跳过此面
            }
        }
    }

    TextureRegion tex = texIt->second;

    auto normal = BlockGeometry::getFaceNormal(face);
    auto vertices = BlockGeometry::getFaceVertices(face);

    // 创建4个顶点
    std::array<Vertex, 4> faceVerts;
    const u32 shadedWhiteColor = makeGrayscaleVertexColor(getFaceShade(face));

    // UV坐标根据顶点位置设置
    // 顶点顺序: 左下、右下、右上、左上
    f32 uvs[4][2] = {
        { tex.u0, tex.v1 }, // 左下
        { tex.u1, tex.v1 }, // 右下
        { tex.u1, tex.v0 }, // 右上
        { tex.u0, tex.v0 }  // 左上
    };

    // 打包双通道光照：高4位=天空光，低4位=方块光
    const u8 packedLight = static_cast<u8>(((skyLight & 0x0F) << 4) | (blockLight & 0x0F));

    for (size_t i = 0; i < 4; ++i) {
        faceVerts[i] = Vertex(
            x + vertices[i * 3 + 0],
            y + vertices[i * 3 + 1],
            z + vertices[i * 3 + 2],
            normal[0], normal[1], normal[2],
            uvs[i][0], uvs[i][1],
            shadedWhiteColor,
            packedLight
        );
    }

    // 添加顶点和索引
    u32 baseIndex = static_cast<u32>(mesh.vertices.size());
    for (const auto& v : faceVerts) {
        mesh.vertices.push_back(v);
    }

    auto indices = BlockGeometry::getFaceIndices();
    for (u32 idx : indices) {
        mesh.indices.push_back(baseIndex + idx);
    }
}

void ChunkMesher::addFaceFromAppearanceSmooth(
    MeshData& mesh,
    Face face,
    f32 x, f32 y, f32 z,
    const ChunkData& chunk,
    i32 blockX, i32 blockY, i32 blockZ,
    const BlockAppearance* appearance,
    const ChunkData* neighborChunks[6]
) {
    if (!appearance) {
        return;
    }

    // 检查是否有 elements
    if (appearance->elements.empty()) {
        return;
    }

    // 查找面的纹理
    String faceName;
    switch (face) {
        case Face::Bottom: faceName = "down"; break;
        case Face::Top: faceName = "up"; break;
        case Face::North: faceName = "north"; break;
        case Face::South: faceName = "south"; break;
        case Face::West: faceName = "west"; break;
        case Face::East: faceName = "east"; break;
        default: return;
    }

    auto texIt = appearance->faceTextures.find(faceName);
    if (texIt == appearance->faceTextures.end()) {
        // 尝试使用 "side" 作为后备
        texIt = appearance->faceTextures.find("side");
        if (texIt == appearance->faceTextures.end()) {
            // 尝试使用 "all" 作为后备
            texIt = appearance->faceTextures.find("all");
            if (texIt == appearance->faceTextures.end()) {
                return; // 没有找到纹理，跳过此面
            }
        }
    }

    TextureRegion tex = texIt->second;

    auto normal = BlockGeometry::getFaceNormal(face);
    auto vertices = BlockGeometry::getFaceVertices(face);

    // 计算 AO
    client::renderer::AmbientOcclusionCalculator aoCalc;
    auto aoResult = aoCalc.calculate(chunk, blockX, blockY, blockZ, face, neighborChunks);

    // UV坐标根据顶点位置设置
    // 顶点顺序: 左下、右下、右上、左上
    f32 uvs[4][2] = {
        { tex.u0, tex.v1 }, // 左下
        { tex.u1, tex.v1 }, // 右下
        { tex.u1, tex.v0 }, // 右上
        { tex.u0, tex.v0 }  // 左上
    };

    // 创建4个顶点，每个顶点有独立的光照和AO
    std::array<Vertex, 4> faceVerts;
    const float faceShade = getFaceShade(face);
    for (size_t i = 0; i < 4; ++i) {
        // 打包双通道光照：高4位=天空光，低4位=方块光
        const u8 packedLight = static_cast<u8>(
            ((aoResult.vertexSkyLight[i] & 0x0F) << 4) |
            (aoResult.vertexBlockLight[i] & 0x0F)
        );

        // AO 乘数与面向明暗共同作用于顶点颜色。
        // 注意：颜色打包必须遵循 RGBA 字节顺序，避免出现偏色（例如偏红）问题。
        const float ao = aoResult.vertexColorMultiplier[i];
        const float finalShade = std::clamp(ao * faceShade, 0.0f, 1.0f);
        const u32 color = makeGrayscaleVertexColor(finalShade);

        faceVerts[i] = Vertex(
            x + vertices[i * 3 + 0],
            y + vertices[i * 3 + 1],
            z + vertices[i * 3 + 2],
            normal[0], normal[1], normal[2],
            uvs[i][0], uvs[i][1],
            color,
            packedLight
        );
    }

    // 添加顶点和索引
    u32 baseIndex = static_cast<u32>(mesh.vertices.size());
    for (const auto& v : faceVerts) {
        mesh.vertices.push_back(v);
    }

    auto indices = BlockGeometry::getFaceIndices();
    for (u32 idx : indices) {
        mesh.indices.push_back(baseIndex + idx);
    }
}

void ChunkMesher::simpleMeshSection(
    const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6]
) {
    // 必须有 BlockModelCache
    if (!s_modelCache) {
        spdlog::error("ChunkMesher: BlockModelCache not initialized, cannot generate mesh");
        return;
    }

    constexpr i32 SIZE = ChunkSection::SIZE;
    const i32 baseY = sectionIndex * SIZE;

    // 获取当前段
    const ChunkSection* section = chunk.getSection(sectionIndex);
    if (!section || section->isEmpty()) {
        return;
    }

    // 遍历段内所有方块
    for (i32 y = 0; y < SIZE; ++y) {
        for (i32 z = 0; z < SIZE; ++z) {
            for (i32 x = 0; x < SIZE; ++x) {
                const BlockState* block = section->getBlock(x, y, z);
                if (!shouldRenderBlock(block)) {
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
                if (appearance->elements.empty() && appearance->faceTextures.empty()) {
                    continue;
                }

                // 检查每个面
                for (size_t faceIdx = 0; faceIdx < 6; ++faceIdx) {
                    Face face = static_cast<Face>(faceIdx);
                    auto dir = BlockGeometry::getFaceDirection(face);

                    // 计算邻居坐标
                    i32 nx = x + dir[0];
                    i32 ny = y + dir[1];
                    i32 nz = z + dir[2];

                    const BlockState* neighbor = nullptr;

                    // 检查是否在当前区块内
                    if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && nz >= 0 && nz < SIZE) {
                        neighbor = section->getBlock(nx, ny, nz);
                    } else {
                        // 在区块边界，需要查询邻居区块或当前区块的其他段
                        i32 worldX = nx;
                        i32 worldY = baseY + ny;
                        i32 worldZ = nz;

                        // 处理Y方向跨段
                        if (worldY < 0 || worldY >= world::CHUNK_HEIGHT) {
                            neighbor = nullptr; // 空气
                        } else if (worldX >= 0 && worldX < world::CHUNK_WIDTH &&
                                   worldZ >= 0 && worldZ < world::CHUNK_WIDTH) {
                            // 仍在当前区块内
                            neighbor = chunk.getBlock(worldX, worldY, worldZ);
                        } else if (neighborChunks) {
                            // 在邻居区块中
                            i32 neighborIdx = -1;
                            if (worldX < 0) neighborIdx = 0;        // -X
                            else if (worldX >= SIZE) neighborIdx = 1; // +X
                            else if (worldZ < 0) neighborIdx = 2;     // -Z
                            else if (worldZ >= SIZE) neighborIdx = 3; // +Z

                            if (neighborIdx >= 0 && neighborChunks[neighborIdx]) {
                                // 计算邻居区块中的坐标
                                i32 lx = (worldX + SIZE) % SIZE;
                                i32 lz = (worldZ + SIZE) % SIZE;
                                neighbor = neighborChunks[neighborIdx]->getBlock(lx, worldY, lz);
                            } else {
                                neighbor = nullptr;
                            }
                        } else {
                            neighbor = nullptr;
                        }
                    }

                    // 决定是否渲染该面
                    if (shouldRenderFace(block, neighbor)) {
                        const f32 fx = static_cast<f32>(x);
                        const f32 fy = static_cast<f32>(baseY + y);
                        const f32 fz = static_cast<f32>(z);

                        if (s_lightingMode == LightingMode::Smooth && s_lightingEnabled) {
                            // 平滑光照模式：使用AO计算
                            addFaceFromAppearanceSmooth(outMesh, face,
                                    fx, fy, fz,
                                    chunk, x, baseY + y, z,
                                    appearance, neighborChunks);
                        } else {
                            // 平面光照模式
                            u8 skyLight = 15;   // 默认天空光
                            u8 blockLight = 0;  // 默认方块光
                            if (s_lightingEnabled) {
                                const i32 sampleX = x + dir[0];
                                const i32 sampleY = baseY + y + dir[1];
                                const i32 sampleZ = z + dir[2];

                                // 对于透明方块的边界面，如果邻居是不透明方块，
                                // 使用方块自身位置的光照，避免采样到不透明方块内部的黑色
                                // 这解决了草方块底部渲染为黑色的问题
                                if (block->isTransparent() && neighbor && !neighbor->isAir() && !neighbor->isTransparent()) {
                                    // 采样方块自身位置的光照
                                    skyLight = sampleSkyLight(chunk, x, baseY + y, z, neighborChunks);
                                    blockLight = sampleBlockLight(chunk, x, baseY + y, z, neighborChunks);
                                } else {
                                    // 正常情况：采样邻居位置的光照
                                    skyLight = sampleSkyLight(chunk, sampleX, sampleY, sampleZ, neighborChunks);
                                    blockLight = sampleBlockLight(chunk, sampleX, sampleY, sampleZ, neighborChunks);
                                }
                            }

                            addFaceFromAppearance(outMesh, face,
                                    fx, fy, fz,
                                    skyLight,
                                    blockLight,
                                    appearance);
                        }
                    }
                }
            }
        }
    }
}

void ChunkMesher::greedyMeshSection(
    const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6]
) {
    // 贪婪网格合并算法
    // 暂时使用简单网格生成，贪婪网格合并算法较复杂
    // TODO: 实现完整的贪婪网格合并
    simpleMeshSection(chunk, sectionIndex, outMesh, neighborChunks);
}

// ============================================================================
// 透明方块网格生成
// ============================================================================

void ChunkMesher::generateTransparentMesh(
    const ChunkData& chunk,
    MeshData& outMesh,
    const ChunkData* neighbors[6]
) {
    MC_TRACE_CHUNK_MESH_EVENT("GenerateTransparentMesh");

    outMesh.clear();

    // 预分配较小的空间，透明方块通常比实心方块少
    constexpr size_t ESTIMATED_TRANSPARENT_FACES = 256;
    outMesh.reserve(ESTIMATED_TRANSPARENT_FACES * BlockGeometry::VERTICES_PER_FACE,
                    ESTIMATED_TRANSPARENT_FACES * BlockGeometry::INDICES_PER_FACE);

    // 遍历所有区块段
    for (i32 sectionY = 0; sectionY < ChunkData::SECTIONS; ++sectionY) {
        if (chunk.hasSection(sectionY)) {
            transparentMeshSection(chunk, sectionY, outMesh, neighbors);
        }
    }
}

void ChunkMesher::transparentMeshSection(
    const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6]
) {
    if (!s_modelCache) {
        return;
    }

    constexpr i32 SIZE = ChunkSection::SIZE;
    const i32 baseY = sectionIndex * SIZE;

    // 获取当前段
    const ChunkSection* section = chunk.getSection(sectionIndex);
    if (!section || section->isEmpty()) {
        return;
    }

    // 遍历段内所有方块
    for (i32 y = 0; y < SIZE; ++y) {
        for (i32 z = 0; z < SIZE; ++z) {
            for (i32 x = 0; x < SIZE; ++x) {
                const BlockState* block = section->getBlock(x, y, z);
                if (!block || block->isAir()) {
                    continue;
                }

                // 跳过液体方块（由 FluidMesher 单独处理）
                if (block->isLiquid()) {
                    continue;
                }

                // 只处理透明方块（玻璃、冰、染色玻璃等）
                if (!block->isTransparent()) {
                    continue;
                }

                // 获取方块外观
                const BlockAppearance* appearance = s_modelCache->getBlockAppearance(block);
                if (!appearance) {
                    appearance = s_modelCache->getMissingAppearance();
                    if (!appearance) {
                        continue;
                    }
                }

                // 检查外观是否有效
                if (appearance->elements.empty() && appearance->faceTextures.empty()) {
                    continue;
                }

                // 检查每个面
                for (size_t faceIdx = 0; faceIdx < 6; ++faceIdx) {
                    Face face = static_cast<Face>(faceIdx);
                    auto dir = BlockGeometry::getFaceDirection(face);

                    // 计算邻居坐标
                    i32 nx = x + dir[0];
                    i32 ny = y + dir[1];
                    i32 nz = z + dir[2];

                    const BlockState* neighbor = nullptr;

                    // 检查是否在当前区块内
                    if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && nz >= 0 && nz < SIZE) {
                        neighbor = section->getBlock(nx, ny, nz);
                    } else {
                        // 在区块边界，需要查询邻居区块或当前区块的其他段
                        i32 worldX = nx;
                        i32 worldY = baseY + ny;
                        i32 worldZ = nz;

                        // 处理Y方向跨段
                        if (worldY < 0 || worldY >= world::CHUNK_HEIGHT) {
                            neighbor = nullptr; // 空气
                        } else if (worldX >= 0 && worldX < world::CHUNK_WIDTH &&
                                   worldZ >= 0 && worldZ < world::CHUNK_WIDTH) {
                            // 仍在当前区块内
                            neighbor = chunk.getBlock(worldX, worldY, worldZ);
                        } else if (neighborChunks) {
                            // 在邻居区块中
                            i32 neighborIdx = -1;
                            if (worldX < 0) neighborIdx = 0;        // -X
                            else if (worldX >= SIZE) neighborIdx = 1; // +X
                            else if (worldZ < 0) neighborIdx = 2;     // -Z
                            else if (worldZ >= SIZE) neighborIdx = 3; // +Z

                            if (neighborIdx >= 0 && neighborChunks[neighborIdx]) {
                                // 计算邻居区块中的坐标
                                i32 lx = (worldX + SIZE) % SIZE;
                                i32 lz = (worldZ + SIZE) % SIZE;
                                neighbor = neighborChunks[neighborIdx]->getBlock(lx, worldY, lz);
                            } else {
                                neighbor = nullptr;
                            }
                        } else {
                            neighbor = nullptr;
                        }
                    }

                    // 决定是否渲染该面
                    if (shouldRenderFace(block, neighbor)) {
                        const f32 fx = static_cast<f32>(x);
                        const f32 fy = static_cast<f32>(baseY + y);
                        const f32 fz = static_cast<f32>(z);

                        if (s_lightingMode == LightingMode::Smooth && s_lightingEnabled) {
                            // 平滑光照模式：使用AO计算
                            addFaceFromAppearanceSmooth(outMesh, face,
                                    fx, fy, fz,
                                    chunk, x, baseY + y, z,
                                    appearance, neighborChunks);
                        } else {
                            // 平面光照模式
                            u8 skyLight = 15;
                            u8 blockLight = 0;
                            if (s_lightingEnabled) {
                                const i32 sampleX = x + dir[0];
                                const i32 sampleY = baseY + y + dir[1];
                                const i32 sampleZ = z + dir[2];

                                // 对于透明方块的边界面，如果邻居是不透明方块，
                                // 使用方块自身位置的光照，避免采样到不透明方块内部的黑色
                                if (block->isTransparent() && neighbor && !neighbor->isAir() && !neighbor->isTransparent()) {
                                    skyLight = sampleSkyLight(chunk, x, baseY + y, z, neighborChunks);
                                    blockLight = sampleBlockLight(chunk, x, baseY + y, z, neighborChunks);
                                } else {
                                    skyLight = sampleSkyLight(chunk, sampleX, sampleY, sampleZ, neighborChunks);
                                    blockLight = sampleBlockLight(chunk, sampleX, sampleY, sampleZ, neighborChunks);
                                }
                            }

                            addFaceFromAppearance(outMesh, face,
                                    fx, fy, fz,
                                    skyLight,
                                    blockLight,
                                    appearance);
                        }
                    }
                }
            }
        }
    }
}

// ============================================================================
// ChunkMeshCache 实现
// ============================================================================

ChunkMeshCache::ChunkMeshCache(size_t maxCachedChunks)
    : m_maxCachedChunks(maxCachedChunks)
{
}

ChunkRenderData* ChunkMeshCache::getOrCreate(const ChunkId& id) {
    auto it = m_cache.find(id);
    if (it != m_cache.end()) {
        return &it->second;
    }

    // 检查是否需要清理旧缓存
    if (m_cache.size() >= m_maxCachedChunks) {
        // 简单的LRU策略：移除标记为脏的旧数据
        for (auto it2 = m_cache.begin(); it2 != m_cache.end(); ) {
            if (!it2->second.isDirty && !it2->second.needsUpdate) {
                it2 = m_cache.erase(it2);
                if (m_cache.size() < m_maxCachedChunks) break;
            } else {
                ++it2;
            }
        }
    }

    auto& data = m_cache[id];
    data.chunkId = id;
    return &data;
}

ChunkRenderData* ChunkMeshCache::get(const ChunkId& id) {
    auto it = m_cache.find(id);
    if (it != m_cache.end()) {
        return &it->second;
    }
    return nullptr;
}

const ChunkRenderData* ChunkMeshCache::get(const ChunkId& id) const {
    auto it = m_cache.find(id);
    if (it != m_cache.end()) {
        return &it->second;
    }
    return nullptr;
}

void ChunkMeshCache::markDirty(const ChunkId& id) {
    auto it = m_cache.find(id);
    if (it != m_cache.end()) {
        if (!it->second.isDirty) {
            m_dirtyCount++;
        }
        it->second.markDirty();
    }
}

void ChunkMeshCache::remove(const ChunkId& id) {
    auto it = m_cache.find(id);
    if (it != m_cache.end()) {
        if (it->second.isDirty) {
            m_dirtyCount--;
        }
        m_cache.erase(it);
    }
}

void ChunkMeshCache::clear() {
    m_cache.clear();
    m_dirtyCount = 0;
}

} // namespace mc
