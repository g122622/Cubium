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

#include "world/block/registry/MudBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/blocks/dirt/MudBlock.hpp"

namespace mc {
namespace block_registry {

// 泥巴系列方块
Block* MudBlocks::MUD = nullptr;
Block* MudBlocks::PACKED_MUD = nullptr;
Block* MudBlocks::MUD_BRICKS = nullptr;
Block* MudBlocks::MUD_BRICK_STAIRS = nullptr;
Block* MudBlocks::MUD_BRICK_SLAB = nullptr;
Block* MudBlocks::MUD_BRICK_WALL = nullptr;

void registerMudBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 泥巴系列方块注册（1.19 荒野更新）
    // ============================================================================

    // 泥巴属性 - EARTH材质, 锹有效, 硬度0.5, 抗性0.5
    // 泥巴碰撞箱略矮（14/16格高），实体走在上面会略微下沉，且不可被路径寻找通过
    MudBlocks::MUD = &registry.registerBlock<blocks::MudBlock>(ResourceLocation("minecraft:mud"),
        BlockProperties(Material::EARTH)
            .hardness(0.5f)
            .resistance(0.5f)
            .harvestTool(HarvestTool::Shovel)
            .soundType(BlockSoundTypes::MUD));

    // 泥坯属性 - EARTH材质, 镐有效, 硬度1.0, 抗性3.0
    // 泥巴压实后的方块，可用于合成泥砖
    MudBlocks::PACKED_MUD = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:packed_mud"),
        BlockProperties(Material::EARTH)
            .hardness(1.0f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .soundType(BlockSoundTypes::PACKED_MUD));

    // 泥砖属性 - ROCK材质, 镐有效, 硬度1.5, 抗性3.0
    BlockProperties mudBricksProps = BlockProperties(Material::ROCK)
                                         .hardness(1.5f)
                                         .resistance(3.0f)
                                         .harvestTool(HarvestTool::Pickaxe)
                                         .soundType(BlockSoundTypes::MUD_BRICKS);

    // 泥砖 - 建筑材料
    MudBlocks::MUD_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:mud_bricks"), mudBricksProps);

    // 泥砖楼梯属性 - ROCK材质, 镐有效, 硬度1.5, 抗性3.0
    BlockProperties mudBrickStairsProps =
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f).harvestTool(HarvestTool::Pickaxe);

    // 泥砖楼梯 - 使用泥砖作为源方块
    MudBlocks::MUD_BRICK_STAIRS = &registry.registerBlock<blocks::StairsBlock>(
        ResourceLocation("minecraft:mud_brick_stairs"), MudBlocks::MUD_BRICKS->defaultState(), mudBrickStairsProps);

    // 泥砖台阶属性 - ROCK材质, 镐有效, 硬度1.5, 抗性3.0
    BlockProperties mudBrickSlabProps =
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f).harvestTool(HarvestTool::Pickaxe);

    // 泥砖台阶
    MudBlocks::MUD_BRICK_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:mud_brick_slab"), mudBrickSlabProps);

    // 泥砖墙属性 - ROCK材质, 镐有效, 硬度1.5, 抗性3.0
    BlockProperties mudBrickWallProps =
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f).harvestTool(HarvestTool::Pickaxe);

    // 泥砖墙
    MudBlocks::MUD_BRICK_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:mud_brick_wall"), mudBrickWallProps);
}

} // namespace block_registry
} // namespace mc
