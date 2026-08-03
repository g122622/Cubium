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

#include "StructureCheck.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

// 前向声明
class IWorldWriter;

namespace resource {
class DataPackRepository;
} // namespace resource

namespace world::gen::jigsaw {
class TemplatePoolRegistry;
}

namespace world::gen::structure {

/**
 * @brief 结构注册表
 *
 * 管理所有已注册的结构类型。按 ResourceLocation 索引结构。
 */
class StructureRegistry {
public:
    static void initialize();
    static void registerStructure(std::unique_ptr<Structure> structure);

    /**
     * @brief 清空结构注册表
     *
     * 清空已注册的结构映射与列表并重置初始化标志，供数据驱动重新加载前调用
     * （MinecraftServer::initializeRegistries 重载世界时）。不清理模板池
     * （模板池有独立生命周期，由 Pools::initialize 自带守卫）。保留该入口后，
     * 硬编码 initialize() 仍可作为测试/旧入口的兜底。
     */
    static void clear();

    /**
     * @brief 标记注册表为已初始化（数据驱动加载完成后调用）
     *
     * 数据驱动路径通过 StructureDefinitionLoader 注册结构，但不会像硬编码
     * initialize() 那样置 s_initialized。加载完成后调用本方法置位，使区块
     * 生成器的兜底守卫（if (!isInitialized()) initialize()）不再触发硬编码注册。
     */
    static void markInitialized();

    /**
     * @brief 按资源位置获取结构
     *
     * @param id 结构资源位置
     * @return 结构指针，未找到返回 nullptr
     */
    [[nodiscard]] static const Structure* get(const ResourceLocation& id);

    /**
     * @brief 按名称字符串获取结构（兼容旧接口）
     *
     * 内部将字符串转换为 ResourceLocation 进行查找。
     *
     * @param name 结构名称（如 "minecraft:village_plains" 或 "village_plains"）
     * @return 结构指针，未找到返回 nullptr
     */
    [[nodiscard]] static const Structure* get(const std::string& name);

    [[nodiscard]] static const std::vector<const Structure*>& getAll();
    [[nodiscard]] static bool isInitialized() { return s_initialized; }

    /**
     * @brief 从数据包加载模板池
     *
     * 加载数据包中的模板池 JSON 文件并注册到 TemplatePoolRegistry。
     * 应在 initialize() 之后调用，或在加载世界数据包时调用。
     *
     * @param dataPackList 数据包列表
     * @return 加载的模板池数量
     */
    static size_t loadTemplatePoolsFromDataPacks(const resource::DataPackRepository& dataPackList);

private:
    static std::unordered_map<ResourceLocation, std::unique_ptr<Structure>>& getStructures();
    static std::vector<const Structure*>& getStructureList();
    static bool s_initialized;
};

/**
 * @brief 结构管理器
 *
 * 协调结构生成，管理结构引用和起始点。
 * 持有 StructureCheck 缓存，缓存结构存在性检查结果以避免重复计算。
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
     * @brief 清理所有结构检查缓存
     *
     * 清空 StructureCheck 的精确缓存（m_loadedChunks）和近似缓存（m_featureChecks）。
     * 在维度卸载时由 IChunkGenerator::clearStructureCache() 显式调用，
     * 通过 ServerDimension::shutdown() -> ServerWorld -> ServerChunkManager -> IChunkGenerator
     * 的调用链触发，对齐 MC 1.21.11 中 ServerLevel 卸载时立即清理 StructureCheck 的行为。
     */
    void clearCache();

    /**
     * @brief 获取结构检查缓存
     *
     * 允许外部代码访问 StructureCheck 以进行结构存在性查询和缓存通知。
     * 对齐 MC 1.21.11 中 StructureManager 持有 StructureCheck 的架构。
     *
     * 当前调用方：
     * - NoiseChunkGenerator::generateStructureStarts() 通过此方法调用 onStructureLoad()
     * - NoiseChunkGenerator::generateStructureReferences() 通过此方法调用 incrementReference()
     * - ServerWorld::findNearestStructure() 通过 IChunkGenerator::structureCheck() 调用 checkStart()
     */
    [[nodiscard]] StructureCheck& structureCheck() { return m_structureCheck; }
    [[nodiscard]] const StructureCheck& structureCheck() const { return m_structureCheck; }

private:
    i64 m_seed;
    i32 m_referenceDistance = 8;

    /// 结构存在性检查缓存，避免重复执行昂贵的生物群系检查和频率计算
    StructureCheck m_structureCheck;

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
