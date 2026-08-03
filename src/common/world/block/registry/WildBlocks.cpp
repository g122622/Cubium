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

#include "world/block/registry/WildBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/blocks/RotatedPillarBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"

namespace mc {
namespace block_registry {

// 1.19 荒野更新其他方块
Block* WildBlocks::OCHRE_FROGLIGHT = nullptr;
Block* WildBlocks::VERDANT_FROGLIGHT = nullptr;
Block* WildBlocks::PEARLESCENT_FROGLIGHT = nullptr;
Block* WildBlocks::FROGSPAWN = nullptr;

void registerWildBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 蛙明灯系列方块注册（1.19 荒野更新）
    // ============================================================================

    // 蛙明灯属性 - WOOL材质, 硬度0.3, 抗性0.3, 发光等级15
    // 由青蛙吃小型岩浆怪后掉落，颜色取决于青蛙类型
    BlockProperties froglightProps = BlockProperties(Material::WOOL)
                                         .hardness(0.3f)
                                         .resistance(0.3f)
                                         .soundType(BlockSoundTypes::FROGLIGHT)
                                         .lightLevel(15);

    // 赭黄蛙明灯 - 暖蛙（橙色青蛙）吃岩浆怪掉落，有轴属性
    WildBlocks::OCHRE_FROGLIGHT =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:ochre_froglight"), froglightProps);

    // 青翠蛙明灯 - 冷蛙（绿色青蛙）吃岩浆怪掉落，有轴属性
    WildBlocks::VERDANT_FROGLIGHT =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:verdant_froglight"), froglightProps);

    // 珠光蛙明灯 - 温蛙（白色青蛙）吃岩浆怪掉落，有轴属性
    WildBlocks::PEARLESCENT_FROGLIGHT = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:pearlescent_froglight"), froglightProps);

    // ============================================================================
    // 蛙卵方块注册（1.19 荒野更新）
    // ============================================================================

    // 蛙卵属性 - PLANT材质, 无碰撞, 非固体, 硬度0.0
    // 青蛙在水中产卵后生成，可孵化出蝌蚪
    BlockProperties frogspawnProps =
        BlockProperties(Material::PLANT).noCollision().notSolid().soundType(BlockSoundTypes::FROGSPAWN);

    // 蛙卵 - 放置在水面上，孵化后消失
    WildBlocks::FROGSPAWN =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:frogspawn"), frogspawnProps);
}

} // namespace block_registry
} // namespace mc
