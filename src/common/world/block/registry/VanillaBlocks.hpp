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
 */

#pragma once

#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "world/block/registry/AgriculturalBlocks.hpp"
#include "world/block/registry/BambooBlocks.hpp"
#include "world/block/registry/BaseBlocks.hpp"
#include "world/block/registry/BuildingBlocks.hpp"
#include "world/block/registry/BuildingVariantBlocks.hpp"
#include "world/block/registry/CandleBlocks.hpp"
#include "world/block/registry/CaveBlocks.hpp"
#include "world/block/registry/CherryBlocks.hpp"
#include "world/block/registry/ColoredBlocks.hpp"
#include "world/block/registry/CopperBlocks.hpp"
#include "world/block/registry/DeepslateBlocks.hpp"
#include "world/block/registry/FlowerPotBlocks.hpp"
#include "world/block/registry/GardenBlocks.hpp"
#include "world/block/registry/MangroveBlocks.hpp"
#include "world/block/registry/MudBlocks.hpp"
#include "world/block/registry/NaturalBlocks.hpp"
#include "world/block/registry/NetherBlocks.hpp"
#include "world/block/registry/PaleGardenBlocks.hpp"
#include "world/block/registry/RedstoneBlocks.hpp"
#include "world/block/registry/SculkBlocks.hpp"
#include "world/block/registry/ShelfBlocks.hpp"
#include "world/block/registry/SignBannerBlocks.hpp"
#include "world/block/registry/TrailsBlocks.hpp"
#include "world/block/registry/TrialBlocks.hpp"
#include "world/block/registry/TuffBlocks.hpp"
#include "world/block/registry/VegetationBlocks.hpp"
#include "world/block/registry/WildBlocks.hpp"

namespace mc {

/**
 * @brief 原版方块静态引用
 *
 * 提供所有原版方块的静态指针，便于快速访问。
 * 在游戏初始化时调用 VanillaBlocks::initialize() 进行注册。
 *
 * 参考: net.minecraft.block.Blocks
 */
class VanillaBlocks : public block_registry::AgriculturalBlocks,
                      public block_registry::BambooBlocks,
                      public block_registry::BaseBlocks,
                      public block_registry::BuildingBlocks,
                      public block_registry::BuildingVariantBlocks,
                      public block_registry::CandleBlocks,
                      public block_registry::CaveBlocks,
                      public block_registry::CherryBlocks,
                      public block_registry::ColoredBlocks,
                      public block_registry::CopperBlocks,
                      public block_registry::DeepslateBlocks,
                      public block_registry::FlowerPotBlocks,
                      public block_registry::GardenBlocks,
                      public block_registry::MangroveBlocks,
                      public block_registry::MudBlocks,
                      public block_registry::NaturalBlocks,
                      public block_registry::NetherBlocks,
                      public block_registry::PaleGardenBlocks,
                      public block_registry::RedstoneBlocks,
                      public block_registry::SculkBlocks,
                      public block_registry::ShelfBlocks,
                      public block_registry::SignBannerBlocks,
                      public block_registry::TrailsBlocks,
                      public block_registry::TrialBlocks,
                      public block_registry::TuffBlocks,
                      public block_registry::VegetationBlocks,
                      public block_registry::WildBlocks {
public:
    /**
     * @brief 初始化所有原版方块
     *
     * 必须在使用任何方块前调用。
     */
    static void initialize();

    /**
     * @brief 安全获取方块默认状态
     *
     * 用于在初始化阶段可能尚未注册方块时安全获取默认状态。
     * 如果方块为空指针，返回 nullptr。
     *
     * @param block 方块指针（可能为 nullptr）
     * @return 方块默认状态指针，如果方块为空则返回 nullptr
     */
    [[nodiscard]] static const BlockState* getState(Block* block) noexcept
    {
        return block ? &block->defaultState() : nullptr;
    }

private:
    static bool s_initialized;
};

} // namespace mc
