#include "LightInitializer.hpp"
#include "../chunk/ChunkData.hpp"
#include "../chunk/IChunk.hpp"
#include "../block/Block.hpp"
#include "../WorldConstants.hpp"
#include <algorithm>

namespace mc {

void LightInitializer::initializeSkyLight(ChunkData& data, const Heightmap& heightmap) {
    // 获取世界表面高度图
    // 对每个 XZ 列，从顶部向下设置天空光照
    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
            // 获取表面高度
            i32 surfaceHeight = heightmap.getHeight(x, z);

            // 从顶部到表面高度+1，天空光照为15
            for (i32 y = world::MAX_BUILD_HEIGHT - 1; y > surfaceHeight; --y) {
                data.setSkyLight(x, y, z, 15);
            }

            // 从表面向下递减
            i32 skyLight = 15;
            for (i32 y = surfaceHeight; y >= world::MIN_BUILD_HEIGHT; --y) {
                const BlockState* state = data.getBlock(x, y, z);
                if (state && !state->isAir()) {
                    // 根据方块透明度衰减
                    i32 opacity = state->getOpacity();
                    if (opacity > 0) {
                        skyLight = std::max(0, skyLight - std::max(1, opacity));
                    }
                }
                data.setSkyLight(x, y, z, static_cast<u8>(skyLight));
            }
        }
    }
}

void LightInitializer::initializeBlockLight(ChunkData& data) {
    // 遍历所有发光方块
    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
            for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; ++y) {
                const BlockState* state = data.getBlock(x, y, z);
                if (state && state->lightLevel() > 0) {
                    data.setBlockLight(x, y, z, static_cast<u8>(state->lightLevel()));
                }
            }
        }
    }
}

void LightInitializer::initializeChunkLight(ChunkData& data, const Heightmap& heightmap) {
    initializeSkyLight(data, heightmap);
    initializeBlockLight(data);
}

} // namespace mc
