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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ServerTreeGenerators.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/cave/AzaleaBlock.hpp"
#include "common/world/block/blocks/vegetation/SaplingBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/tree/TreeFeature.hpp"
#include <memory>

namespace mc {
namespace server::gen {

// SaplingBlock / AzaleaBlock 定义在 mc::blocks 中，在此引入以便
// 使用 SaplingBlock::TreeGenerator 等符号时无需 blocks:: 前缀。
using namespace blocks;

namespace {

/**
 * @brief 创建树木生成器的通用工厂函数
 *
 * TreeGenerator 签名为 void(IWorld&, const BlockPos&, math::IRandom&)，
 * 由 SaplingBlock 在 grow() 中通过 createFeatureRegion() 获取的
 * WorldGenRegion（以 IWorld 接口暴露）回调。lambda 内部将 IWorld&
 * static_cast 为 WorldGenRegion& 后调用 TreeFeature::place()。
 *
 * @param configCreator 返回 TreeFeatureConfig 的工厂函数
 * @return SaplingBlock::TreeGenerator 回调
 */
SaplingBlock::TreeGenerator createTreeGenerator(std::function<TreeFeatureConfig()> configCreator)
{
    auto config = std::make_shared<TreeFeatureConfig>(configCreator());
    auto feature = std::make_shared<TreeFeature>();

    return [config, feature](IWorld& world, const BlockPos& pos, math::IRandom& random) {
        auto& region = static_cast<WorldGenRegion&>(world);
        feature->place(region, random, pos, *config);
    };
}

} // namespace

SaplingBlock::TreeGenerator ServerTreeGenerators::oakTree()
{
    return createTreeGenerator([]() { return TreeFeatures::oakConfig(); });
}

SaplingBlock::TreeGenerator ServerTreeGenerators::birchTree()
{
    return createTreeGenerator([]() { return TreeFeatures::birchConfig(); });
}

SaplingBlock::TreeGenerator ServerTreeGenerators::spruceTree()
{
    return createTreeGenerator([]() { return TreeFeatures::spruceConfig(); });
}

SaplingBlock::TreeGenerator ServerTreeGenerators::jungleTree()
{
    return createTreeGenerator([]() { return TreeFeatures::jungleConfig(); });
}

SaplingBlock::TreeGenerator ServerTreeGenerators::acaciaTree()
{
    return createTreeGenerator([]() { return TreeFeatures::acaciaConfig(); });
}

SaplingBlock::TreeGenerator ServerTreeGenerators::darkOakTree()
{
    return createTreeGenerator([]() { return TreeFeatures::darkOakConfig(); });
}

SaplingBlock::TreeGenerator ServerTreeGenerators::cherryTree()
{
    return createTreeGenerator([]() { return TreeFeatures::cherryConfig(); });
}

SaplingBlock::TreeGenerator ServerTreeGenerators::paleOakTree()
{
    return createTreeGenerator([]() { return TreeFeatures::paleOakConfig(); });
}

SaplingBlock::TreeGenerator ServerTreeGenerators::azaleaTree()
{
    return createTreeGenerator([]() { return TreeFeatures::azaleaConfig(); });
}

void ServerTreeGenerators::injectAll()
{
    // 在 VanillaBlocks::initialize() 完成所有方块注册后调用。
    // 通过 VanillaBlocks 中的静态 Block* 指针访问已注册的 SaplingBlock，
    // 调用 setTreeGenerator() 注入真实回调。
    if (VanillaBlocks::OAK_SAPLING != nullptr) {
        static_cast<blocks::SaplingBlock*>(VanillaBlocks::OAK_SAPLING)
            ->setTreeGenerator(ServerTreeGenerators::oakTree());
    }
    if (VanillaBlocks::SPRUCE_SAPLING != nullptr) {
        static_cast<blocks::SaplingBlock*>(VanillaBlocks::SPRUCE_SAPLING)
            ->setTreeGenerator(ServerTreeGenerators::spruceTree());
    }
    if (VanillaBlocks::BIRCH_SAPLING != nullptr) {
        static_cast<blocks::SaplingBlock*>(VanillaBlocks::BIRCH_SAPLING)
            ->setTreeGenerator(ServerTreeGenerators::birchTree());
    }
    if (VanillaBlocks::JUNGLE_SAPLING != nullptr) {
        static_cast<blocks::SaplingBlock*>(VanillaBlocks::JUNGLE_SAPLING)
            ->setTreeGenerator(ServerTreeGenerators::jungleTree());
    }
    if (VanillaBlocks::ACACIA_SAPLING != nullptr) {
        static_cast<blocks::SaplingBlock*>(VanillaBlocks::ACACIA_SAPLING)
            ->setTreeGenerator(ServerTreeGenerators::acaciaTree());
    }
    if (VanillaBlocks::DARK_OAK_SAPLING != nullptr) {
        static_cast<blocks::SaplingBlock*>(VanillaBlocks::DARK_OAK_SAPLING)
            ->setTreeGenerator(ServerTreeGenerators::darkOakTree());
    }
    if (VanillaBlocks::CHERRY_SAPLING != nullptr) {
        static_cast<blocks::SaplingBlock*>(VanillaBlocks::CHERRY_SAPLING)
            ->setTreeGenerator(ServerTreeGenerators::cherryTree());
    }
    if (VanillaBlocks::PALE_OAK_SAPLING != nullptr) {
        static_cast<blocks::SaplingBlock*>(VanillaBlocks::PALE_OAK_SAPLING)
            ->setTreeGenerator(ServerTreeGenerators::paleOakTree());
    }

    // 杜鹃花/开花杜鹃花方块也持有 TreeGenerator（AzaleaBlock 内部成员）
    if (VanillaBlocks::AZALEA != nullptr) {
        static_cast<blocks::AzaleaBlock*>(VanillaBlocks::AZALEA)->setTreeGenerator(ServerTreeGenerators::azaleaTree());
    }
    if (VanillaBlocks::FLOWERING_AZALEA != nullptr) {
        static_cast<blocks::AzaleaBlock*>(VanillaBlocks::FLOWERING_AZALEA)
            ->setTreeGenerator(ServerTreeGenerators::azaleaTree());
    }
}

} // namespace server::gen
} // namespace mc
