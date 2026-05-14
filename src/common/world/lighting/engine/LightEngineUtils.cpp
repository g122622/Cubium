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

#include "LightEngineUtils.hpp"
#include "../../../physics/collision/CollisionShape.hpp"
#include "../../IWorld.hpp"
#include "../../block/Block.hpp"
#include "../../chunk/ChunkPos.hpp"
#include "../../chunk/IChunk.hpp"
#include <climits>

namespace mc {

// 静态常量定义
constexpr Direction LightEngineUtils::ALL_DIRECTIONS[6];
constexpr Direction LightEngineUtils::HORIZONTAL_DIRECTIONS[4];

i64 LightEngineUtils::worldToSectionPos(i64 worldPos)
{
    // 使用unpackPos解码坐标，然后转换为SectionPos
    i32 x, y, z;
    unpackPos(worldPos, x, y, z);
    return SectionPos(x >> 4, y >> 4, z >> 4).toLong();
}

const BlockState* LightEngineUtils::getBlockAndOpacity(const IChunk* chunk, i64 worldPos, i32* opacityOut)
{

    if (worldPos == ROOT_POS) {
        if (opacityOut != nullptr) {
            *opacityOut = 0;
        }
        return nullptr; // 空气
    }

    if (chunk == nullptr) {
        if (opacityOut != nullptr) {
            *opacityOut = 15; // 视为不透明
        }
        return nullptr; // 基岩
    }

    i32 x, y, z;
    unpackPos(worldPos, x, y, z);
    const BlockState* state = chunk->getBlockState(x & 0xF, y, z & 0xF);

    if (state == nullptr) {
        if (opacityOut != nullptr) {
            *opacityOut = 0;
        }
        return nullptr;
    }

    // 空气方块按“无方块”处理，避免误参与光照遮挡计算。
    if (state->isAir()) {
        if (opacityOut != nullptr) {
            *opacityOut = 0;
        }
        return nullptr;
    }

    if (opacityOut != nullptr) {
        *opacityOut = state->getOpacity();
    }

    return state;
}

const CollisionShape& LightEngineUtils::getVoxelShape(const BlockState& state)
{
    // 对于光照，我们只关心方块是否是固体
    if (state.isSolid()) {
        return state.getOcclusionShape();
    }
    return VoxelShapes::empty();
}

bool LightEngineUtils::facesHaveOcclusion(IWorld* world,
    const BlockState& stateA,
    const BlockPos& posA,
    const BlockState& stateB,
    const BlockPos& posB,
    Direction dir,
    i32 opacityA)
{
    // 如果任一方块是空气，则无遮挡
    if (stateA.isAir() || stateB.isAir()) {
        return false;
    }

    // 如果透明度为0，光线可以通过
    if (opacityA <= 0 && stateB.getOpacity() <= 0) {
        return false;
    }

    // 如果透明度为15（完全不透明），检查是否有完整遮挡面
    if (opacityA >= 15 && stateB.getOpacity() >= 15) {
        // 两个完全不透明的方块
        // 检查是否有完整的遮挡形状
        const CollisionShape& shapeA = stateA.getOcclusionShape();
        const CollisionShape& shapeB = stateB.getOcclusionShape();

        // 如果两个都是完整方块，则完全遮挡
        if (shapeA.isFullBlock() && shapeB.isFullBlock()) {
            return true;
        }

        // 对于非完整方块，检查面遮挡
        Direction oppositeDir = Directions::opposite(dir);

        if (shapeFullyOccludesFace(shapeA, dir) && shapeFullyOccludesFace(shapeB, oppositeDir)) {
            return true;
        }
    }

    // 对于部分透明的方块，不进行面遮挡检测
    // 光线会根据透明度衰减
    return false;
}

bool LightEngineUtils::blocksLightInDirection(const BlockState& state, Direction dir)
{
    if (state.isAir()) {
        return false;
    }

    i32 opacity = state.getOpacity();
    if (opacity <= 0) {
        return false;
    }

    if (opacity >= 15) {
        return true;
    }

    // 部分透明方块，检查遮挡形状
    const CollisionShape& shape = state.getOcclusionShape();
    return shapeFullyOccludesFace(shape, dir);
}

bool LightEngineUtils::shapeFullyOccludesFace(const CollisionShape& shape, Direction face)
{
    if (shape.isEmpty()) {
        return false;
    }

    // 如果是完整方块，所有面都被完全遮挡
    if (shape.isFullBlock()) {
        return true;
    }

    // 对于简单盒，检查是否完全覆盖面
    // 一个面被完全覆盖的条件是：在该方向上投影覆盖整个面 (0-1范围)
    const auto& boxes = shape.boxes();

    // 使用简化的判断：检查所有盒的并集是否覆盖整个面
    // 这不是完全准确的，但对于大多数情况足够
    switch (face) {
        case Direction::Down: // Y = 0 面
            // 检查是否有盒子的 minY == 0 且在该面上完全覆盖
            for (const auto& box : boxes) {
                if (box.minY <= 0.0f && box.minX <= 0.0f && box.maxX >= 1.0f && box.minZ <= 0.0f && box.maxZ >= 1.0f) {
                    return true;
                }
            }
            break;

        case Direction::Up: // Y = 1 面
            for (const auto& box : boxes) {
                if (box.maxY >= 1.0f && box.minX <= 0.0f && box.maxX >= 1.0f && box.minZ <= 0.0f && box.maxZ >= 1.0f) {
                    return true;
                }
            }
            break;

        case Direction::North: // Z = 0 面
            for (const auto& box : boxes) {
                if (box.minZ <= 0.0f && box.minX <= 0.0f && box.maxX >= 1.0f && box.minY <= 0.0f && box.maxY >= 1.0f) {
                    return true;
                }
            }
            break;

        case Direction::South: // Z = 1 面
            for (const auto& box : boxes) {
                if (box.maxZ >= 1.0f && box.minX <= 0.0f && box.maxX >= 1.0f && box.minY <= 0.0f && box.maxY >= 1.0f) {
                    return true;
                }
            }
            break;

        case Direction::West: // X = 0 面
            for (const auto& box : boxes) {
                if (box.minX <= 0.0f && box.minY <= 0.0f && box.maxY >= 1.0f && box.minZ <= 0.0f && box.maxZ >= 1.0f) {
                    return true;
                }
            }
            break;

        case Direction::East: // X = 1 面
            for (const auto& box : boxes) {
                if (box.maxX >= 1.0f && box.minY <= 0.0f && box.maxY >= 1.0f && box.minZ <= 0.0f && box.maxZ >= 1.0f) {
                    return true;
                }
            }
            break;

        default:
            break;
    }

    return false;
}

} // namespace mc
