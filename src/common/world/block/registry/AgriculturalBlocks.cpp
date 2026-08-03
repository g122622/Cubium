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

#include "world/block/registry/AgriculturalBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockSoundType.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/agricultural/BeetrootBlock.hpp"
#include "world/block/blocks/agricultural/CarrotBlock.hpp"
#include "world/block/blocks/agricultural/CocoaBlock.hpp"
#include "world/block/blocks/agricultural/PotatoBlock.hpp"
#include "world/block/blocks/agricultural/WheatBlock.hpp"

namespace mc {
namespace block_registry {

Block* AgriculturalBlocks::WHEAT = nullptr;
Block* AgriculturalBlocks::CARROTS = nullptr;
Block* AgriculturalBlocks::POTATOES = nullptr;
Block* AgriculturalBlocks::BEETROOTS = nullptr;
Block* AgriculturalBlocks::COCOA = nullptr;

void registerAgriculturalBlocks()
{
    auto& registry = BlockRegistry::instance();

    // 农作物方块通用属性：植物材质、无碰撞、非固体、作物音效、0硬度
    const BlockProperties cropProps =
        BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).soundType(BlockSoundTypes::CROP);

    // 小麦作物 - 8个生长阶段（AGE_0_7），小麦种子种植
    AgriculturalBlocks::WHEAT =
        &registry.registerBlock<blocks::WheatBlock>(ResourceLocation("minecraft:wheat"), cropProps);

    // 胡萝卜作物 - 8个生长阶段（AGE_0_7），胡萝卜种植
    AgriculturalBlocks::CARROTS =
        &registry.registerBlock<blocks::CarrotBlock>(ResourceLocation("minecraft:carrots"), cropProps);

    // 马铃薯作物 - 8个生长阶段（AGE_0_7），马铃薯种植
    AgriculturalBlocks::POTATOES =
        &registry.registerBlock<blocks::PotatoBlock>(ResourceLocation("minecraft:potatoes"), cropProps);

    // 甜菜根作物 - 4个生长阶段（AGE_0_3），甜菜种子种植
    AgriculturalBlocks::BEETROOTS =
        &registry.registerBlock<blocks::BeetrootBlock>(ResourceLocation("minecraft:beetroots"), cropProps);

    // 可可豆 - 3个生长阶段（AGE_0_2），附着在丛林原木侧面
    // MC原版属性：Plant材质、无碰撞、非固体、硬度0.2/爆炸抗性3.0、随机tick、WOOD音效
    AgriculturalBlocks::COCOA = &registry.registerBlock<blocks::CocoaBlock>(ResourceLocation("minecraft:cocoa"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.2f)
            .resistance(3.0f)
            .soundType(BlockSoundTypes::WOOD));
}

} // namespace block_registry
} // namespace mc
