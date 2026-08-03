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

#include "../../chunk/IChunkGenerator.hpp"
#include "../Structure.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc::world::gen::structure {

class MineshaftPiece;

/**
 * @brief 废弃矿井类型
 */
enum class MineshaftType : u8 {
    Normal, ///< 普通废弃矿井
    Mesa    ///< 恶地废弃矿井（恶地生物群系）
};

/**
 * @brief 废弃矿井配置
 */
struct MineshaftConfig {
    f32 probability = 0.004f; ///< 生成概率
    MineshaftType type = MineshaftType::Normal;
};

// ============================================================================
// 辅助函数声明
// ============================================================================

/**
 * @brief 随机创建矿井片段
 */
[[nodiscard]] std::unique_ptr<MineshaftPiece> createMineshaftPiece(std::vector<std::unique_ptr<MineshaftPiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    i32 direction,
    i32 depth,
    MineshaftType type);

/**
 * @brief 生成并添加矿井片段
 */
[[nodiscard]] std::unique_ptr<MineshaftPiece> addMineshaftPiece(MineshaftPiece* parent,
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    i32 direction,
    i32 depth,
    MineshaftType type);

// ============================================================================
// 废弃矿井片段基类
// ============================================================================

/**
 * @brief 废弃矿井片段基类
 */
class MineshaftPiece : public StructurePiece {
public:
    using StructurePiece::buildComponent;

    MineshaftPiece(i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, MineshaftType mineshaftType);

    [[nodiscard]] MineshaftType mineshaftType() const { return m_mineshaftType; }

    /**
     * @brief 构建连接片段
     * @param pieces 已有片段列表
     * @param rng 随机数生成器
     * @param maxDepth 最大深度
     */
    virtual void buildComponent(
        std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::Random& rng, i32 maxDepth) = 0;

protected:
    /**
     * @brief 检查位置是否可以放置片段
     */
    [[nodiscard]] static bool _canPlaceAt(i32 x, i32 y, i32 z);

    /**
     * @brief 生成木板支撑
     */
    void _generateSupport(IWorldWriter& world, i32 x, i32 y, i32 z, i32 height, math::Random& rng);

    MineshaftType m_mineshaftType;
};

// ============================================================================
// 废弃矿井房间
// ============================================================================

/**
 * @brief 废弃矿井房间
 *
 * 废弃矿井的中央起点房间。
 */
class MineshaftRoom : public MineshaftPiece {
public:
    MineshaftRoom(i32 componentType, math::Random& rng, i32 x, i32 y, i32 z, MineshaftType type);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::Random& rng, i32 maxDepth) override;

private:
    /// 出口方向列表
    std::vector<i32> m_exits;
};

// ============================================================================
// 废弃矿井走廊
// ============================================================================

/**
 * @brief 废弃矿井走廊
 *
 * 水平的矿道走廊，带有支撑柱和铁轨。
 */
class MineshaftCorridor : public MineshaftPiece {
public:
    MineshaftCorridor(i32 componentType,
        math::Random& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 direction,
        MineshaftType type);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::Random& rng, i32 maxDepth) override;

    [[nodiscard]] i32 direction() const { return m_direction; }

private:
    /**
     * @brief 生成走廊地板
     */
    void _generateFloor(IWorldWriter& world,
        i32 x1,
        i32 z1,
        i32 x2,
        i32 z2,
        math::Random& rng,
        const StructureBoundingBox& chunkBounds);

    /**
     * @brief 生成走廊天花板
     */
    void _generateCeiling(IWorldWriter& world,
        i32 x1,
        i32 z1,
        i32 x2,
        i32 z2,
        math::Random& rng,
        const StructureBoundingBox& chunkBounds);

    /**
     * @brief 生成支撑柱
     */
    void _generatePillars(
        IWorldWriter& world, i32 sectionIndex, math::Random& rng, const StructureBoundingBox& chunkBounds);

    /**
     * @brief 生成铁轨
     */
    void _generateRails(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds);

    /**
     * @brief 生成蜘蛛刷怪笼
     */
    void _generateSpawner(IWorldWriter& world, i32 x, i32 y, i32 z, const StructureBoundingBox& chunkBounds);

    /**
     * @brief 生成宝箱矿车
     */
    void _generateChestMinecart(
        IWorldWriter& world, i32 x, i32 y, i32 z, math::Random& rng, const StructureBoundingBox& chunkBounds);

    bool m_hasRails;
    bool m_hasSpiders;
    bool m_spawnerPlaced = false;
    i32 m_sectionCount;
    i32 m_direction; ///< 0=北, 1=南, 2=西, 3=东
};

// ============================================================================
// 废弃矿井交叉点
// ============================================================================

/**
 * @brief 废弃矿井交叉点
 *
 * 两条走廊的交叉点。
 */
class MineshaftCross : public MineshaftPiece {
public:
    MineshaftCross(i32 componentType,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 direction,
        MineshaftType type);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::Random& rng, i32 maxDepth) override;

    [[nodiscard]] i32 direction() const { return m_direction; }

private:
    i32 m_direction;
};

// ============================================================================
// 废弃矿井楼梯
// ============================================================================

/**
 * @brief 废弃矿井楼梯
 *
 * 连接不同高度层的楼梯。
 */
class MineshaftStairs : public MineshaftPiece {
public:
    MineshaftStairs(i32 componentType,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 direction,
        MineshaftType type);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::Random& rng, i32 maxDepth) override;

    [[nodiscard]] i32 direction() const { return m_direction; }

private:
    i32 m_direction;
};

// ============================================================================
// 废弃矿井结构
// ============================================================================

/**
 * @brief 废弃矿井结构
 *
 * 地下生成的复杂矿井结构，包含走廊、交叉点和楼梯。
 */
class MineshaftStructure : public Structure {
public:
    explicit MineshaftStructure(ResourceLocation id, MineshaftType type = MineshaftType::Normal);

    [[nodiscard]] const std::string& name() const override { return m_name; }

    [[nodiscard]] DecorationStage defaultDecorationStage() const override
    {
        return DecorationStage::UndergroundStructures;
    }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 minecraft:has_structure/mineshaft 标签，用于 O(1) 生物群系查找。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    /**
     * @brief 检查是否可以生成
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成废弃矿井
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    static const std::string m_name;
    static const std::vector<BiomeId> m_mesaBiomes;

    MineshaftConfig m_config;
};

// 片段类型常量
namespace MineshaftPieceTypes {
constexpr i32 ROOM = 60;
constexpr i32 CORRIDOR = 61;
constexpr i32 CROSS = 62;
constexpr i32 STAIRS = 63;
} // namespace MineshaftPieceTypes

} // namespace mc::world::gen::structure
