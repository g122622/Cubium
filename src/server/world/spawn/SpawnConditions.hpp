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

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc {

// 前向声明
class IWorld;

namespace world::spawn {

/**
 * @brief 生成条件检查
 *
 * 提供生成位置的条件检查工具函数。
 *
 * 参考 MC 1.16.5 EntitySpawnPlacementRegistry
 */
namespace SpawnConditions {

/**
 * @brief 检查光照等级是否满足生成条件
 * @param skyLight 天空光照 (0-15)
 * @param blockLight 方块光照 (0-15)
 * @param isMonster 是否是怪物
 * @return 是否可以生成
 */
bool checkLightLevel(i32 skyLight, i32 blockLight, bool isMonster);

/**
 * @brief 检查位置是否可以生成实体
 * @param world 世界接口
 * @param x, y, z 位置
 * @param entityWidth 实体宽度
 * @param entityHeight 实体高度
 * @return 是否可以生成
 */
bool canSpawnAtPosition(IWorld& world, i32 x, i32 y, i32 z, f32 entityWidth, f32 entityHeight);

/**
 * @brief 检查位置是否有足够的碰撞空间
 * @param world 世界接口
 * @param x, y, z 位置
 * @param width 实体宽度
 * @param height 实体高度
 * @return 是否有足够空间
 */
bool hasCollisionSpace(IWorld& world, i32 x, i32 y, i32 z, f32 width, f32 height);

/**
 * @brief 检查方块是否阻止生成
 * @param isLiquid 是否为液体方块
 * @param isAir 是否为空气方块
 * @return 是否阻止生成
 */
bool blockPreventsSpawn(bool isLiquid, bool isAir);

/**
 * @brief 获取地面高度（最高可站立方块）
 * @param world 世界接口
 * @param x, z 水平坐标
 * @return 地面高度，如果没有返回 -1
 */
i32 getGroundHeight(IWorld& world, i32 x, i32 z);

/**
 * @brief 检查位置是否在水中
 * @param world 世界接口
 * @param x, y, z 位置
 * @return 是否在水中
 */
bool isInWater(IWorld& world, i32 x, i32 y, i32 z);

/**
 * @brief 检查位置是否在岩浆中
 * @param world 世界接口
 * @param x, y, z 位置
 * @return 是否在岩浆中
 */
bool isInLava(IWorld& world, i32 x, i32 y, i32 z);

} // namespace SpawnConditions

} // namespace world::spawn
} // namespace mc
