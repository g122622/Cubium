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

#include "common/util/math/random/Random.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

// 前向声明
class IWorldWriter;

namespace resource {
class DataPackList;
}

namespace world::gen::jigsaw {
class JigsawPatternRegistry;
}

namespace world::gen::structure {

/**
 * @brief 结构注册表
 *
 * 管理所有已注册的结构类型。
 */
class StructureRegistry {
public:
    static void initialize();
    static void registerStructure(std::unique_ptr<Structure> structure);
    [[nodiscard]] static const Structure* get(const std::string& name);
    [[nodiscard]] static const std::vector<const Structure*>& getAll();
    [[nodiscard]] static bool isInitialized() { return s_initialized; }

    /**
     * @brief 从数据包加载模板池
     *
     * 加载数据包中的模板池 JSON 文件并注册到 JigsawPatternRegistry。
     * 应在 initialize() 之后调用，或在加载世界数据包时调用。
     *
     * @param dataPackList 数据包列表
     * @return 加载的模板池数量
     */
    static size_t loadTemplatePoolsFromDataPacks(const resource::DataPackList& dataPackList);

private:
    static std::unordered_map<std::string, std::unique_ptr<Structure>>& getStructures();
    static std::vector<const Structure*>& getStructureList();
    static bool s_initialized;
};

/**
 * @brief 结构管理器
 *
 * 协调结构生成，管理结构引用和起始点。
 */
class StructureManager {
public:
    explicit StructureManager(i64 seed);

    /**
     * @brief 获取世界种子
     */
    [[nodiscard]] i64 seed() const { return m_seed; }

    /**
     * @brief 设置引用距离
     */
    void setReferenceDistance(i32 distance) { m_referenceDistance = distance; }

    /**
     * @brief 检查是否应该在指定区块生成结构起点
     * @param structure 结构类型
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 是否应该生成
     */
    [[nodiscard]] bool shouldGenerateStructureStart(const Structure& structure, i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 在指定区块生成结构起点
     * @param structure 结构类型
     * @param world 世界写入器
     * @param generator 区块生成器
     * @param rng 随机数生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 生成的结构起点，如果无法生成则返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generateStructureStart(const Structure& structure,
        IWorldWriter& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ);

    /**
     * @brief 在区块中放置结构
     * @param structure 结构类型
     * @param world 世界写入器
     * @param chunk 区块
     * @param start 结构起点
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     */
    void placeStructureInChunk(const Structure& structure,
        IWorldWriter& world,
        ChunkPrimer& chunk,
        StructureStart& start,
        i32 chunkX,
        i32 chunkZ);

    /**
     * @brief 清理缓存
     */
    void clearCache();

private:
    i64 m_seed;
    i32 m_referenceDistance = 8;

    /**
     * @brief 创建结构随机数生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param salt 盐值
     * @return 随机数生成器
     */
    [[nodiscard]] math::Random _createRandom(i32 chunkX, i32 chunkZ, i32 salt) const;
};

} // namespace world::gen::structure
} // namespace mc
