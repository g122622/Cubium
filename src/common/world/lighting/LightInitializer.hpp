#pragma once

#include "../../core/Types.hpp"
#include "../chunk/IChunk.hpp"
#include "../chunk/ChunkPos.hpp"
#include <memory>

namespace mc {

// 前向声明
class ChunkData;
class Heightmap;

/**
 * @brief 光照初始化器
 *
 * 负责区块生成时的初始光照计算。
 * 将逻辑从 ChunkPrimer 中分离出来，属于光照系统的职责。
 *
 * 参考: net.minecraft.world.lighting.WorldLightManager 和区块生成流程
 */
class LightInitializer {
public:
    /**
     * @brief 初始化天空光照
     *
     * 根据 WORLD_SURFACE_WG 高度图初始化天空光照。
     * 高度图以上的方块天空光照为15，以下根据方块透明度递减。
     *
     * @param data 区块数据
     * @param heightmap 世界表面高度图
     */
    static void initializeSkyLight(ChunkData& data, const Heightmap& heightmap);

    /**
     * @brief 初始化方块光照
     *
     * 扫描区块中所有发光方块，将其光照值写入区块数据。
     *
     * @param data 区块数据
     */
    static void initializeBlockLight(ChunkData& data);

    /**
     * @brief 初始化区块光照（天空+方块）
     *
     * 一次性初始化天空光照和方块光照。
     * 这是区块生成 LIGHT 阶段的完整初始化流程。
     *
     * @param data 区块数据
     * @param heightmap 世界表面高度图
     */
    static void initializeChunkLight(ChunkData& data, const Heightmap& heightmap);
};

} // namespace mc
