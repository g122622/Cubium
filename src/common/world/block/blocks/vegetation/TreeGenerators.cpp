/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "TreeGenerators.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/gen/feature/tree/TreeFeature.hpp"

namespace mc {
namespace blocks {

namespace {

/**
 * @brief 创建树木生成器的通用工厂函数
 *
 * @param configCreator 返回 TreeFeatureConfig 的工厂函数
 * @return TreeGenerator 回调
 */
SaplingBlock::TreeGenerator createTreeGenerator(std::function<TreeFeatureConfig()> configCreator)
{
    // 每个 TreeGenerator 持有自己的 TreeFeatureConfig 和 TreeFeature 实例
    // 使用 shared_ptr 保证 lambda 捕获的对象生命周期正确
    auto config = std::make_shared<TreeFeatureConfig>(configCreator());
    auto feature = std::make_shared<TreeFeature>();

    return [config, feature](WorldGenRegion& world, const BlockPos& pos, math::Random& random) {
        feature->place(world, random, pos, *config);
    };
}

} // namespace

SaplingBlock::TreeGenerator TreeGenerators::oakTree()
{
    return createTreeGenerator([]() { return TreeFeatures::oakConfig(); });
}

SaplingBlock::TreeGenerator TreeGenerators::birchTree()
{
    return createTreeGenerator([]() { return TreeFeatures::birchConfig(); });
}

SaplingBlock::TreeGenerator TreeGenerators::spruceTree()
{
    return createTreeGenerator([]() { return TreeFeatures::spruceConfig(); });
}

SaplingBlock::TreeGenerator TreeGenerators::jungleTree()
{
    return createTreeGenerator([]() { return TreeFeatures::jungleConfig(); });
}

SaplingBlock::TreeGenerator TreeGenerators::acaciaTree()
{
    return createTreeGenerator([]() { return TreeFeatures::acaciaConfig(); });
}

SaplingBlock::TreeGenerator TreeGenerators::darkOakTree()
{
    return createTreeGenerator([]() { return TreeFeatures::darkOakConfig(); });
}

SaplingBlock::TreeGenerator TreeGenerators::cherryTree()
{
    return createTreeGenerator([]() { return TreeFeatures::cherryConfig(); });
}

SaplingBlock::TreeGenerator TreeGenerators::paleOakTree()
{
    return createTreeGenerator([]() { return TreeFeatures::paleOakConfig(); });
}

SaplingBlock::TreeGenerator TreeGenerators::azaleaTree()
{
    return createTreeGenerator([]() { return TreeFeatures::azaleaConfig(); });
}

} // namespace blocks
} // namespace mc
