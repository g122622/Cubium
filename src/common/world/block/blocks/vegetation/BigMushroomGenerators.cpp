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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "BigMushroomGenerators.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VegetationBlocks.hpp"
#include "common/world/gen/feature/vegetation/BigMushroomFeature.hpp"
#include <functional>
#include <memory>

namespace mc {
namespace blocks {

namespace {

/**
 * @brief 创建巨型蘑菇生成器的通用工厂函数
 *
 * @param configCreator 返回 BigMushroomFeatureConfig 的工厂函数（延迟到调用时执行，
 *                      以避免在方块注册阶段访问尚未注册的巨型蘑菇方块状态）
 * @param feature 巨型蘑菇 feature 实例（BigBrownMushroomFeature / BigRedMushroomFeature）
 * @return BigMushroomGenerator 回调
 */
MushroomBlock::BigMushroomGenerator createBigMushroomGenerator(
    std::function<BigMushroomFeatureConfig()> configCreator, std::shared_ptr<BigMushroomFeature> feature)
{
    return [configCreator, feature](WorldGenRegion& world, const BlockPos& pos, math::Random& random) {
        BigMushroomFeatureConfig config = configCreator();
        feature->place(world, random, pos, config);
    };
}

} // namespace

MushroomBlock::BigMushroomGenerator BigMushroomGenerators::brownMushroom()
{
    // 棕色巨型蘑菇：盖为 brown_mushroom_block，柄为 mushroom_stem
    auto feature = std::make_shared<BigBrownMushroomFeature>();
    return createBigMushroomGenerator(
        []() {
            return BigMushroomFeatureConfig(&block_registry::VegetationBlocks::BROWN_MUSHROOM_BLOCK->defaultState(),
                &block_registry::VegetationBlocks::MUSHROOM_STEM->defaultState(),
                2);
        },
        feature);
}

MushroomBlock::BigMushroomGenerator BigMushroomGenerators::redMushroom()
{
    // 红色巨型蘑菇：盖为 red_mushroom_block，柄为 mushroom_stem
    auto feature = std::make_shared<BigRedMushroomFeature>();
    return createBigMushroomGenerator(
        []() {
            return BigMushroomFeatureConfig(&block_registry::VegetationBlocks::RED_MUSHROOM_BLOCK->defaultState(),
                &block_registry::VegetationBlocks::MUSHROOM_STEM->defaultState(),
                2);
        },
        feature);
}

} // namespace blocks
} // namespace mc
