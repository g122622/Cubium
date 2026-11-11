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

#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/AgriculturalBlocks.hpp"
#include "common/world/block/registry/vanilla/BambooBlocks.hpp"
#include "common/world/block/registry/vanilla/BaseBlocks.hpp"
#include "common/world/block/registry/vanilla/BuildingBlocks.hpp"
#include "common/world/block/registry/vanilla/BuildingVariantBlocks.hpp"
#include "common/world/block/registry/vanilla/CandleBlocks.hpp"
#include "common/world/block/registry/vanilla/CaveBlocks.hpp"
#include "common/world/block/registry/vanilla/CherryBlocks.hpp"
#include "common/world/block/registry/vanilla/ColoredBlocks.hpp"
#include "common/world/block/registry/vanilla/CopperBlocks.hpp"
#include "common/world/block/registry/vanilla/DeepslateBlocks.hpp"
#include "common/world/block/registry/vanilla/FlowerPotBlocks.hpp"
#include "common/world/block/registry/vanilla/GardenBlocks.hpp"
#include "common/world/block/registry/vanilla/MangroveBlocks.hpp"
#include "common/world/block/registry/vanilla/MudBlocks.hpp"
#include "common/world/block/registry/vanilla/NaturalBlocks.hpp"
#include "common/world/block/registry/vanilla/NetherBlocks.hpp"
#include "common/world/block/registry/vanilla/PaleGardenBlocks.hpp"
#include "common/world/block/registry/vanilla/RedstoneBlocks.hpp"
#include "common/world/block/registry/vanilla/SculkBlocks.hpp"
#include "common/world/block/registry/vanilla/ShelfBlocks.hpp"
#include "common/world/block/registry/vanilla/SignBannerBlocks.hpp"
#include "common/world/block/registry/vanilla/TrailsBlocks.hpp"
#include "common/world/block/registry/vanilla/TrialBlocks.hpp"
#include "common/world/block/registry/vanilla/TuffBlocks.hpp"
#include "common/world/block/registry/vanilla/VegetationBlocks.hpp"
#include "common/world/block/registry/vanilla/WildBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"

namespace mc {

/**
 * @brief 原版方块静态引用
 *
 * 提供所有原版方块的静态指针，便于快速访问。
 * 在游戏初始化时调用 VanillaBlocks::initialize() 进行注册。
 *
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
