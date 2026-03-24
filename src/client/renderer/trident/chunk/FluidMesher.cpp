#include "FluidMesher.hpp"
#include "AmbientOcclusionCalculator.hpp"
#include "../../../resource/BlockModelCache.hpp"
#include "../../../../common/world/WorldConstants.hpp"
#include "../../../../common/world/fluid/Fluid.hpp"
#include "../../../../common/world/block/blocks/LiquidBlock.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cassert>
#include <cmath>

namespace mc::client::renderer {

// ============================================================================
// 静态成员初始化
// ============================================================================

BlockModelCache* FluidMesher::s_modelCache = nullptr;

// ============================================================================
// 常量
// ============================================================================

namespace {

/**
 * @brief 源流体高度 (8/9 ≈ 0.888...)
 *
 * 参考 MC 1.16.5 FluidBlockRenderer
 */
constexpr f32 SOURCE_FLUID_HEIGHT = 8.0f / 9.0f;

/**
 * @brief 将 RGBA 分量打包为顶点颜色
 */
[[nodiscard]] constexpr u32 packVertexColor(u8 r, u8 g, u8 b, u8 a) {
    return static_cast<u32>(r)
        | (static_cast<u32>(g) << 8)
        | (static_cast<u32>(b) << 16)
        | (static_cast<u32>(a) << 24);
}

/**
 * @brief 将 ARGB 颜色转换为 RGBA 顶点颜色
 */
[[nodiscard]] constexpr u32 argbToRgba(u32 argb) {
    u8 a = (argb >> 24) & 0xFF;
    u8 r = (argb >> 16) & 0xFF;
    u8 g = (argb >> 8) & 0xFF;
    u8 b = argb & 0xFF;
    return packVertexColor(r, g, b, a);
}

/**
 * @brief 返回面向明暗系数
 */
[[nodiscard]] constexpr float getFaceShade(Face face) {
    switch (face) {
        case Face::Bottom: return 0.5f;
        case Face::Top:    return 1.0f;
        case Face::North:
        case Face::South:  return 0.8f;
        case Face::West:
        case Face::East:   return 0.6f;
        default:           return 1.0f;
    }
}

/**
 * @brief 将亮度因子转换为灰度顶点色
 */
[[nodiscard]] u32 makeGrayscaleVertexColor(float factor) {
    assert(factor >= -0.01f && factor <= 1.01f);
    const float clamped = std::clamp(factor, 0.0f, 1.0f);
    const u8 channel = static_cast<u8>(std::round(clamped * 255.0f));
    return packVertexColor(channel, channel, channel, 255);
}

} // namespace

// ============================================================================
// 公共方法实现
// ============================================================================

void FluidMesher::setModelCache(BlockModelCache* cache) {
    s_modelCache = cache;
    if (cache) {
        spdlog::info("FluidMesher: Using BlockModelCache for fluid appearances");
    } else {
        spdlog::warn("FluidMesher: BlockModelCache set to null");
    }
}

void FluidMesher::generateFluidMesh(
    const ChunkData& chunk,
    MeshData& outMesh,
    const ChunkData* neighbors[6])
{
    if (!s_modelCache) {
        return;
    }

    outMesh.clear();

    constexpr i32 SIZE = ChunkSection::SIZE;

    // 遍历所有区块段
    for (i32 sectionY = 0; sectionY < ChunkData::SECTIONS; ++sectionY) {
        const ChunkSection* section = chunk.getSection(sectionY);
        if (!section || section->isEmpty()) {
            continue;
        }

        const i32 baseY = sectionY * SIZE;

        // 遍历段内所有方块
        for (i32 y = 0; y < SIZE; ++y) {
            for (i32 z = 0; z < SIZE; ++z) {
                for (i32 x = 0; x < SIZE; ++x) {
                    const BlockState* block = section->getBlock(x, y, z);
                    if (!block || !block->isLiquid()) {
                        continue;
                    }

                    // 获取流体状态
                    const fluid::FluidState* fluidState = block->getFluidState();
                    if (!fluidState || fluidState->isEmpty()) {
                        continue;
                    }

                    // 计算世界坐标
                    const f32 wx = static_cast<f32>(x);
                    const i32 wy = baseY + y;
                    const f32 wz = static_cast<f32>(z);

                    // 获取流体颜色（默认水颜色，TODO: 从群系获取）
                    const u32 color = DEFAULT_WATER_COLOR;
                    const f32 alpha = WATER_ALPHA;

                    // 获取当前流体
                    const fluid::Fluid& fluid = fluidState->getFluid();

                    // 计算流体高度
                    const f32 currentHeight = getFluidHeight(block);

                    // 获取外观信息
                    const BlockAppearance* appearance = s_modelCache->getBlockAppearance(block);
                    if (!appearance) {
                        appearance = s_modelCache->getMissingAppearance();
                    }

                    // 检查每个面
                    for (size_t faceIdx = 0; faceIdx < 6; ++faceIdx) {
                        Face face = static_cast<Face>(faceIdx);
                        auto dir = BlockGeometry::getFaceDirection(face);

                        // 计算邻居坐标
                        const i32 nx = x + dir[0];
                        const i32 ny = wy + dir[1];
                        const i32 nz = z + dir[2];

                        // 获取邻居方块
                        const BlockState* neighbor = nullptr;
                        if (ny >= world::MIN_BUILD_HEIGHT && ny < world::MAX_BUILD_HEIGHT) {
                            if (nx >= 0 && nx < SIZE && nz >= 0 && nz < SIZE) {
                                neighbor = section->getBlock(nx, ny - baseY, nz);
                                // 如果跨区块段，需要从 ChunkData 获取
                                if (!neighbor && (ny < baseY || ny >= baseY + SIZE)) {
                                    neighbor = chunk.getBlock(nx, ny, nz);
                                }
                            } else if (neighbors) {
                                // 边界邻居
                                i32 neighborIdx = -1;
                                i32 lx = nx, lz = nz;
                                if (nx < 0) { lx = nx + SIZE; neighborIdx = 0; }
                                else if (nx >= SIZE) { lx = nx - SIZE; neighborIdx = 1; }
                                else if (nz < 0) { lz = nz + SIZE; neighborIdx = 2; }
                                else if (nz >= SIZE) { lz = nz - SIZE; neighborIdx = 3; }

                                if (neighborIdx >= 0 && neighbors[neighborIdx]) {
                                    neighbor = neighbors[neighborIdx]->getBlock(lx, ny, lz);
                                }
                            }
                        }

                        // 决定是否渲染该面
                        if (!shouldRenderFluidFace(block, neighbor, face)) {
                            continue;
                        }

                        // 采样光照
                        u8 skyLight = 15;
                        u8 blockLight = 0;
                        if (s_modelCache) {
                            skyLight = sampleSkyLight(chunk, nx, ny, nz, neighbors);
                            blockLight = sampleBlockLight(chunk, nx, ny, nz, neighbors);
                        }

                        // 获取纹理区域
                        TextureRegion texture;
                        if (appearance && !appearance->faceTextures.empty()) {
                            String faceName;
                            switch (face) {
                                case Face::Bottom: faceName = "down"; break;
                                case Face::Top:    faceName = "up"; break;
                                default:           faceName = "side"; break;
                            }

                            auto texIt = appearance->faceTextures.find(faceName);
                            if (texIt != appearance->faceTextures.end()) {
                                texture = texIt->second;
                            }
                        }

                        if (face == Face::Top) {
                            // 水面：计算四角高度
                            f32 h00, h10, h01, h11;
                            getCornerHeights(chunk, x, wy, z, neighbors, &fluid, h00, h10, h01, h11);

                            // 如果周围有更低的流体或空气，调整高度
                            // 参考 MC FluidBlockRenderer: 避免渲染内部面
                            if (h00 < 0.001f && h10 < 0.001f && h01 < 0.001f && h11 < 0.001f) {
                                continue; // 完全被流体包围
                            }

                            addWaterSurface(outMesh, wx, static_cast<f32>(wy), wz,
                                          h00, h10, h01, h11,
                                          skyLight, blockLight, texture, color);
                        } else if (face == Face::Bottom) {
                            // 底面：平面渲染
                            addFaceFromAppearance(outMesh, face, wx, static_cast<f32>(wy), wz,
                                                skyLight, blockLight, appearance, color);
                        } else {
                            // 侧面：考虑流体高度
                            f32 neighborHeight = 0.0f;
                            if (neighbor && neighbor->isLiquid()) {
                                neighborHeight = getFluidHeight(neighbor);
                            }

                            const f32 topHeight = currentHeight;
                            const f32 bottomHeight = neighborHeight;

                            if (topHeight > bottomHeight) {
                                addFluidSide(outMesh, face, wx, static_cast<f32>(wy), wz,
                                           topHeight, bottomHeight,
                                           skyLight, blockLight, texture, color);
                            }
                        }
                    }
                }
            }
        }
    }
}

// ============================================================================
// 流体高度计算
// ============================================================================

f32 FluidMesher::getFluidHeight(const BlockState* blockState) {
    if (!blockState || !blockState->isLiquid()) {
        return 0.0f;
    }

    const fluid::FluidState* fluidState = blockState->getFluidState();
    if (!fluidState || fluidState->isEmpty()) {
        return 0.0f;
    }

    // 源方块高度为 8/9，流动水高度为 level/9
    return fluidState->isSource() ? SOURCE_FLUID_HEIGHT
                                  : static_cast<f32>(fluidState->getLevel()) / 9.0f;
}

f32 FluidMesher::getActualFluidHeight(
    const ChunkData& chunk,
    i32 x, i32 y, i32 z,
    const ChunkData* neighbors[6],
    const fluid::Fluid* fluid)
{
    constexpr i32 SIZE = ChunkSection::SIZE;

    // 越界检查
    if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
        return 0.0f;
    }

    // 获取方块
    const BlockState* block = nullptr;
    if (x >= 0 && x < SIZE && z >= 0 && z < SIZE) {
        block = chunk.getBlock(x, y, z);
    } else if (neighbors) {
        // 边界处理
        i32 neighborIdx = -1;
        i32 lx = x, lz = z;

        if (x < 0) { lx = x + SIZE; neighborIdx = 0; }
        else if (x >= SIZE) { lx = x - SIZE; neighborIdx = 1; }

        if (z < 0) { lz = z + SIZE; neighborIdx = 2; }
        else if (z >= SIZE) { lz = z - SIZE; neighborIdx = 3; }

        if (neighborIdx >= 0 && neighbors[neighborIdx]) {
            block = neighbors[neighborIdx]->getBlock(lx, y, lz);
        }
    }

    if (!block) {
        return 0.0f;
    }

    // 检查是否为同类型流体
    if (!block->isLiquid()) {
        return 0.0f;
    }

    const fluid::FluidState* state = block->getFluidState();
    if (!state || state->isEmpty()) {
        return 0.0f;
    }

    // 检查是否为相同流体
    if (!fluid->isEquivalentTo(state->getFluid())) {
        return 0.0f;
    }

    return getFluidHeight(block);
}

void FluidMesher::getCornerHeights(
    const ChunkData& chunk,
    i32 x, i32 y, i32 z,
    const ChunkData* neighbors[6],
    const fluid::Fluid* fluid,
    f32& h00, f32& h10, f32& h01, f32& h11)
{
    // 四个角的高度采样
    // 参考 MC 1.16.5 FluidBlockRenderer.getFluidHeight()

    // h00 = 西北角 (x, z)
    // h10 = 东北角 (x+1, z)
    // h01 = 西南角 (x, z+1)
    // h11 = 东南角 (x+1, z+1)

    // 每个角的高度是周围 2x2 方块的平均值
    auto sampleCorner = [&](i32 cx, i32 cz) -> f32 {
        f32 totalHeight = 0.0f;
        i32 count = 0;

        for (i32 dx = 0; dx <= 1; ++dx) {
            for (i32 dz = 0; dz <= 1; ++dz) {
                f32 h = getActualFluidHeight(chunk, cx + dx, y, cz + dz, neighbors, fluid);
                if (h >= 0.8f) {
                    // 高流体，增加权重
                    totalHeight += h * 10.0f;
                    count += 10;
                } else if (h > 0.0f) {
                    totalHeight += h;
                    ++count;
                }
            }
        }

        // 如果周围有空气或非固体方块，也计入
        if (count == 0) {
            // 检查上方是否有流体
            f32 h = getActualFluidHeight(chunk, cx, y + 1, cz, neighbors, fluid);
            if (h > 0.0f) {
                return 1.0f; // 上方有流体，返回满高度
            }
            return 0.0f;
        }

        return totalHeight / static_cast<f32>(count);
    };

    h00 = sampleCorner(x - 1, z - 1);
    h10 = sampleCorner(x, z - 1);
    h01 = sampleCorner(x - 1, z);
    h11 = sampleCorner(x, z);
}

// ============================================================================
// 面渲染判断
// ============================================================================

bool FluidMesher::shouldRenderFluidFace(
    const BlockState* fluidBlock,
    const BlockState* neighborBlock,
    Face face)
{
    if (!fluidBlock || !neighborBlock) {
        return true; // 边界外，渲染面
    }

    // 邻居是空气
    if (neighborBlock->isAir()) {
        return true;
    }

    // 邻居是同类型流体，不渲染内部面
    if (neighborBlock->isLiquid()) {
        const fluid::FluidState* myFluid = fluidBlock->getFluidState();
        const fluid::FluidState* neighborFluid = neighborBlock->getFluidState();

        if (myFluid && neighborFluid &&
            myFluid->getFluid().isEquivalentTo(neighborFluid->getFluid())) {
            return false;
        }
        return true; // 不同类型流体，渲染边界面
    }

    // 邻居是固体方块
    // 检查是否应该显示流体覆盖层（如玻璃、冰等）
    if (shouldDisplayFluidOverlay(neighborBlock)) {
        return true;
    }

    // 邻居是完全不透明的固体方块，不渲染
    if (neighborBlock->isOpaque()) {
        return false;
    }

    // 其他情况（如透明方块），渲染面
    return true;
}

bool FluidMesher::shouldDisplayFluidOverlay(const BlockState* blockState) {
    if (!blockState) {
        return false;
    }

    // 检查方块是否应该显示流体覆盖层
    // 参考 MC 1.16.5 Block.shouldDisplayFluidOverlay()
    // 包括：玻璃、染色玻璃、冰等透明方块

    const Material& material = blockState->getMaterial();

    // 玻璃类材料
    if (material == Material::GLASS) {
        return true;
    }

    // 冰类材料
    if (material == Material::ICE) {
        return true;
    }

    // 荷叶等特殊方块
    // TODO: 添加更精确的检测

    return false;
}

// ============================================================================
// 顶点生成
// ============================================================================

void FluidMesher::addWaterSurface(
    MeshData& mesh,
    f32 x, f32 y, f32 z,
    f32 h00, f32 h10, f32 h01, f32 h11,
    u8 skyLight, u8 blockLight,
    const TextureRegion& texture,
    u32 color)
{
    // 顶点位置（带高度偏移）
    // 四个角的高度可能不同，形成倾斜的水面
    // 顺序：左下、右下、右上、左上

    const f32 y00 = y + h00; // 西北
    const f32 y10 = y + h10; // 东北
    const f32 y01 = y + h01; // 西南
    const f32 y11 = y + h11; // 东南

    // UV 坐标
    const f32 u0 = texture.u0;
    const f32 u1 = texture.u1;
    const f32 v0 = texture.v0;
    const f32 v1 = texture.v1;

    // 打包双通道光照
    const u8 packedLight = static_cast<u8>(((skyLight & 0x0F) << 4) | (blockLight & 0x0F));

    // 面向明暗（顶部面最亮）
    constexpr f32 faceShade = 1.0f;
    const u32 vertexColor = makeGrayscaleVertexColor(faceShade);

    // 提取颜色分量
    const u8 r = (color >> 16) & 0xFF;
    const u8 g = (color >> 8) & 0xFF;
    const u8 b = color & 0xFF;
    const u8 a = (color >> 24) & 0xFF;

    // 临时：使用标准顶点颜色，实际应该调制水颜色
    // TODO: 在着色器中处理水颜色调制

    // 顶点数据：西北、东北、东南、西南
    std::array<Vertex, 4> vertices = {
        Vertex(x,     y00, z,     0.0f, 1.0f, 0.0f, u0, v0, vertexColor, packedLight), // 西北
        Vertex(x + 1, y10, z,     0.0f, 1.0f, 0.0f, u1, v0, vertexColor, packedLight), // 东北
        Vertex(x + 1, y11, z + 1, 0.0f, 1.0f, 0.0f, u1, v1, vertexColor, packedLight), // 东南
        Vertex(x,     y01, z + 1, 0.0f, 1.0f, 0.0f, u0, v1, vertexColor, packedLight), // 西南
    };

    // 添加顶点
    const u32 baseIndex = static_cast<u32>(mesh.vertices.size());
    for (const auto& v : vertices) {
        mesh.vertices.push_back(v);
    }

    // 添加索引（两个三角形）
    // 正面（从上往下看）：西北 -> 东北 -> 东南 -> 西南
    mesh.indices.push_back(baseIndex + 0);
    mesh.indices.push_back(baseIndex + 1);
    mesh.indices.push_back(baseIndex + 2);

    mesh.indices.push_back(baseIndex + 0);
    mesh.indices.push_back(baseIndex + 2);
    mesh.indices.push_back(baseIndex + 3);
}

void FluidMesher::addFluidSide(
    MeshData& mesh,
    Face face,
    f32 x, f32 y, f32 z,
    f32 heightTop, f32 heightBottom,
    u8 skyLight, u8 blockLight,
    const TextureRegion& texture,
    u32 color)
{
    auto normal = BlockGeometry::getFaceNormal(face);
    auto baseVertices = BlockGeometry::getFaceVertices(face);

    const f32 u0 = texture.u0;
    const f32 u1 = texture.u1;
    const f32 v0 = texture.v0; // 顶部 V
    const f32 v1 = texture.v1; // 底部 V

    // 打包双通道光照
    const u8 packedLight = static_cast<u8>(((skyLight & 0x0F) << 4) | (blockLight & 0x0F));

    // 面向明暗
    const f32 faceShade = getFaceShade(face);
    const u32 vertexColor = makeGrayscaleVertexColor(faceShade);

    // 计算顶点位置（顶部高度可变）
    std::array<Vertex, 4> vertices;

    switch (face) {
        case Face::North: // -Z
            vertices = {
                Vertex(x + 1, y + heightTop,     z,     normal[0], normal[1], normal[2], u1, v0, vertexColor, packedLight),
                Vertex(x,     y + heightTop,     z,     normal[0], normal[1], normal[2], u0, v0, vertexColor, packedLight),
                Vertex(x,     y + heightBottom,  z,     normal[0], normal[1], normal[2], u0, v1, vertexColor, packedLight),
                Vertex(x + 1, y + heightBottom,  z,     normal[0], normal[1], normal[2], u1, v1, vertexColor, packedLight),
            };
            break;

        case Face::South: // +Z
            vertices = {
                Vertex(x,     y + heightTop,     z + 1, normal[0], normal[1], normal[2], u0, v0, vertexColor, packedLight),
                Vertex(x + 1, y + heightTop,     z + 1, normal[0], normal[1], normal[2], u1, v0, vertexColor, packedLight),
                Vertex(x + 1, y + heightBottom,  z + 1, normal[0], normal[1], normal[2], u1, v1, vertexColor, packedLight),
                Vertex(x,     y + heightBottom,  z + 1, normal[0], normal[1], normal[2], u0, v1, vertexColor, packedLight),
            };
            break;

        case Face::West: // -X
            vertices = {
                Vertex(x, y + heightTop,     z,     normal[0], normal[1], normal[2], u0, v0, vertexColor, packedLight),
                Vertex(x, y + heightTop,     z + 1, normal[0], normal[1], normal[2], u1, v0, vertexColor, packedLight),
                Vertex(x, y + heightBottom,  z + 1, normal[0], normal[1], normal[2], u1, v1, vertexColor, packedLight),
                Vertex(x, y + heightBottom,  z,     normal[0], normal[1], normal[2], u0, v1, vertexColor, packedLight),
            };
            break;

        case Face::East: // +X
            vertices = {
                Vertex(x, y + heightTop,     z + 1, normal[0], normal[1], normal[2], u1, v0, vertexColor, packedLight),
                Vertex(x, y + heightTop,     z,     normal[0], normal[1], normal[2], u0, v0, vertexColor, packedLight),
                Vertex(x, y + heightBottom,  z,     normal[0], normal[1], normal[2], u0, v1, vertexColor, packedLight),
                Vertex(x, y + heightBottom,  z + 1, normal[0], normal[1], normal[2], u1, v1, vertexColor, packedLight),
            };
            break;

        default:
            return; // 不处理顶面和底面
    }

    // 添加顶点
    const u32 baseIndex = static_cast<u32>(mesh.vertices.size());
    for (const auto& v : vertices) {
        mesh.vertices.push_back(v);
    }

    // 添加索引
    mesh.indices.push_back(baseIndex + 0);
    mesh.indices.push_back(baseIndex + 1);
    mesh.indices.push_back(baseIndex + 2);

    mesh.indices.push_back(baseIndex + 0);
    mesh.indices.push_back(baseIndex + 2);
    mesh.indices.push_back(baseIndex + 3);
}

void FluidMesher::addFaceFromAppearance(
    MeshData& mesh,
    Face face,
    f32 x, f32 y, f32 z,
    u8 skyLight, u8 blockLight,
    const BlockAppearance* appearance,
    u32 color)
{
    if (!appearance || appearance->faceTextures.empty()) {
        return;
    }

    // 查找面的纹理
    String faceName;
    switch (face) {
        case Face::Bottom: faceName = "down"; break;
        case Face::Top:    faceName = "up"; break;
        case Face::North:  faceName = "north"; break;
        case Face::South:  faceName = "south"; break;
        case Face::West:   faceName = "west"; break;
        case Face::East:   faceName = "east"; break;
    }

    auto texIt = appearance->faceTextures.find(faceName);
    if (texIt == appearance->faceTextures.end()) {
        texIt = appearance->faceTextures.find("side");
        if (texIt == appearance->faceTextures.end()) {
            texIt = appearance->faceTextures.find("all");
            if (texIt == appearance->faceTextures.end()) {
                return;
            }
        }
    }

    const TextureRegion& tex = texIt->second;
    auto normal = BlockGeometry::getFaceNormal(face);
    auto vertices = BlockGeometry::getFaceVertices(face);

    const u8 packedLight = static_cast<u8>(((skyLight & 0x0F) << 4) | (blockLight & 0x0F));
    const f32 faceShade = getFaceShade(face);
    const u32 vertexColor = makeGrayscaleVertexColor(faceShade);

    f32 uvs[4][2] = {
        { tex.u0, tex.v1 }, // 左下
        { tex.u1, tex.v1 }, // 右下
        { tex.u1, tex.v0 }, // 右上
        { tex.u0, tex.v0 }, // 左上
    };

    std::array<Vertex, 4> faceVerts;
    for (size_t i = 0; i < 4; ++i) {
        faceVerts[i] = Vertex(
            x + vertices[i * 3 + 0],
            y + vertices[i * 3 + 1],
            z + vertices[i * 3 + 2],
            normal[0], normal[1], normal[2],
            uvs[i][0], uvs[i][1],
            vertexColor,
            packedLight
        );
    }

    const u32 baseIndex = static_cast<u32>(mesh.vertices.size());
    for (const auto& v : faceVerts) {
        mesh.vertices.push_back(v);
    }

    auto indices = BlockGeometry::getFaceIndices();
    for (u32 idx : indices) {
        mesh.indices.push_back(baseIndex + idx);
    }
}

// ============================================================================
// 光照采样
// ============================================================================

u8 FluidMesher::sampleSkyLight(
    const ChunkData& chunk,
    i32 x, i32 y, i32 z,
    const ChunkData* neighbors[6])
{
    constexpr i32 SIZE = ChunkSection::SIZE;

    if (y >= world::MAX_BUILD_HEIGHT) return 15;
    if (y < world::MIN_BUILD_HEIGHT) return 0;

    const ChunkData* sampleChunk = &chunk;
    i32 localX = x;
    i32 localZ = z;

    if (localX < 0) { localX += SIZE; sampleChunk = neighbors ? neighbors[0] : nullptr; }
    else if (localX >= SIZE) { localX -= SIZE; sampleChunk = neighbors ? neighbors[1] : nullptr; }

    if (localZ < 0) {
        localZ += SIZE;
        if (sampleChunk == &chunk || sampleChunk == nullptr) {
            sampleChunk = neighbors ? neighbors[2] : nullptr;
        }
    } else if (localZ >= SIZE) {
        localZ -= SIZE;
        if (sampleChunk == &chunk || sampleChunk == nullptr) {
            sampleChunk = neighbors ? neighbors[3] : nullptr;
        }
    }

    if (!sampleChunk) return 15;

    return sampleChunk->getSkyLight(localX, y, localZ);
}

u8 FluidMesher::sampleBlockLight(
    const ChunkData& chunk,
    i32 x, i32 y, i32 z,
    const ChunkData* neighbors[6])
{
    constexpr i32 SIZE = ChunkSection::SIZE;

    if (y >= world::MAX_BUILD_HEIGHT || y < world::MIN_BUILD_HEIGHT) return 0;

    const ChunkData* sampleChunk = &chunk;
    i32 localX = x;
    i32 localZ = z;

    if (localX < 0) { localX += SIZE; sampleChunk = neighbors ? neighbors[0] : nullptr; }
    else if (localX >= SIZE) { localX -= SIZE; sampleChunk = neighbors ? neighbors[1] : nullptr; }

    if (localZ < 0) {
        localZ += SIZE;
        if (sampleChunk == &chunk || sampleChunk == nullptr) {
            sampleChunk = neighbors ? neighbors[2] : nullptr;
        }
    } else if (localZ >= SIZE) {
        localZ -= SIZE;
        if (sampleChunk == &chunk || sampleChunk == nullptr) {
            sampleChunk = neighbors ? neighbors[3] : nullptr;
        }
    }

    if (!sampleChunk) return 0;

    return sampleChunk->getBlockLight(localX, y, localZ);
}

u8 FluidMesher::sampleCombinedLight(
    const ChunkData& chunk,
    i32 x, i32 y, i32 z,
    const ChunkData* neighbors[6])
{
    return std::max(
        sampleSkyLight(chunk, x, y, z, neighbors),
        sampleBlockLight(chunk, x, y, z, neighbors)
    );
}

} // namespace mc::client::renderer
