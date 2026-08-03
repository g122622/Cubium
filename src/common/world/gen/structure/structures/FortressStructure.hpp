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

#include "../Structure.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 下界要塞回退片段
 *
 * 在 Jigsaw 系统不可用时生成简单的要塞结构。
 * 在 FEATURES 阶段由 placeInChunk() 调用 generate() 写入方块。
 */
class FortressFallbackPiece final : public StructurePiece {
public:
    explicit FortressFallbackPiece(const BlockPos& startPos);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

private:
    BlockPos m_startPos;
    void _generateFallbackFortress(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds);
};

/**
 * @brief 下界要塞结构
 *
 * 下界要塞是生成在下界的大型结构，包含烈焰人刷怪笼和地狱疣房间。
 *
 * 特点：
 * - 生成于下界荒地(Nether Wastes)和灵魂沙谷(Soul Sand Valley)群系
 * - 由桥和走廊组成，桥连接不同部分
 * - 包含烈焰人刷怪笼（Throne房间）
 * - 包含地狱疣房间
 * - 有箱子战利品
 */
class FortressStructure : public Structure {
public:
    /**
     * @brief 下界要塞配置
     */
    struct Config {
        i32 spacing = 27;    ///< 区块间距
        i32 separation = 4;  ///< 最小分离区块
        i32 salt = 30084232; ///< 随机种子盐
        i32 minY = 64;       ///< 最低 Y 坐标
        i32 maxY = 128;      ///< 最高 Y 坐标
        i32 maxRange = 112;  ///< 最大扩展范围
    };

    explicit FortressStructure(ResourceLocation id);
    FortressStructure(ResourceLocation id, const Config& config);

    [[nodiscard]] const std::string& name() const override { return m_name; }
    [[nodiscard]] DecorationStage defaultDecorationStage() const override
    {
        return DecorationStage::UndergroundDecoration;
    }
    [[nodiscard]] const SpawnOverrides* spawnOverrides() const override { return &s_spawnOverrides; }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 minecraft:has_structure/nether_fortress 标签，
     * 用于判断下界要塞可生成的生物群系。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    /**
     * @brief 检查是否可以生成
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成下界要塞起点（仅创建 StructurePiece，禁止写方块）
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    Config m_config;
    static const std::string m_name;
    static const SpawnOverrides s_spawnOverrides;
};

// ============================================================================
// 下界要塞片段类型
// ============================================================================

namespace FortressPieceTypes {
// 主要片段（桥）
constexpr i32 STRAIGHT = 100;  ///< 直桥段 (5x10x19)
constexpr i32 CROSSING3 = 101; ///< 大型十字交叉 (19x10x19)
constexpr i32 CROSSING = 102;  ///< 普通十字交叉 (7x9x7)
constexpr i32 STAIRS = 103;    ///< 楼梯 (7x11x7)
constexpr i32 THRONE = 104;    ///< 王座房间（烈焰人刷怪笼）(7x8x9)
constexpr i32 ENTRANCE = 105;  ///< 入口 (13x14x13)

// 次要片段（走廊）
constexpr i32 CORRIDOR5 = 106;         ///< 基础走廊 (5x7x5)
constexpr i32 CROSSING2 = 107;         ///< 小型十字交叉 (5x7x5)
constexpr i32 CORRIDOR2 = 108;         ///< 带箱子走廊 (5x7x5)
constexpr i32 CORRIDOR = 109;          ///< 带箱子走廊 (5x7x5)
constexpr i32 CORRIDOR3 = 110;         ///< 下沉走廊（带楼梯）(5x14x10)
constexpr i32 CORRIDOR4 = 111;         ///< 带桥走廊 (9x7x9)
constexpr i32 NETHER_STALK_ROOM = 112; ///< 地狱疣房间 (13x14x13)

// 特殊片段
constexpr i32 START = 113; ///< 起始片段
constexpr i32 END = 114;   ///< 终止片段 (5x10x8)
} // namespace FortressPieceTypes

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
