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
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"

namespace mc {
class IWorld;

namespace world::spawn {

/** @brief 生成检查使用的实时世界视图；生成区域补充高度图、群系与生成器海平面。 */
class IWorldSpawnAdapter : public ISpawnWorldReader {
public:
    explicit IWorldSpawnAdapter(IWorld& world);
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] bool isWithinWorldBorder(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] i32 getHeight(HeightmapType type, i32 x, i32 z) const override;
    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] u64 seed() const override;
    [[nodiscard]] Difficulty difficulty() const override;
    [[nodiscard]] SpawnEnvironment environment(const Vector3i& pos) const override;
    [[nodiscard]] bool hasNearbyPlayer(const Vector3d& center, f64 distance) const override;

private:
    IWorld& m_world;
};
} // namespace world::spawn
} // namespace mc
