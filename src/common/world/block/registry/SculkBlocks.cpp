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

#include "world/block/registry/SculkBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/blocks/sculk/SculkBlocks.hpp"

namespace mc {
namespace block_registry {

// 幽匿系列方块
Block* SculkBlocks::SCULK = nullptr;
Block* SculkBlocks::SCULK_VEIN = nullptr;
Block* SculkBlocks::SCULK_CATALYST = nullptr;
Block* SculkBlocks::SCULK_SENSOR = nullptr;
Block* SculkBlocks::CALIBRATED_SCULK_SENSOR = nullptr;
Block* SculkBlocks::SCULK_SHRIEKER = nullptr;

void registerSculkBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 幽匿系列方块注册（1.19 荒野更新）
    // ============================================================================

    // 幽匿块 - Material::SCULK, 锄有效, 硬度0.2, 抗性0.2, 随机刻, 无掉落表(精确采集掉落经验)
    // 深暗之域的主要构成方块，会被幽匿催化体蔓延
    SculkBlocks::SCULK = &registry.registerBlock<blocks::SculkBlock>(ResourceLocation("minecraft:sculk"),
        BlockProperties(Material::SCULK)
            .hardness(0.2f)
            .resistance(0.2f)
            .harvestTool(HarvestTool::Hoe)
            .requiresTool()
            .soundType(BlockSoundTypes::SCULK)
            .tickRandomly()
            .noLootTable());

    // 幽匿脉络 - Material::SCULK, 无碰撞, 非固体, 锄有效, 硬度0.2, 抗性0.2
    // 可放置在方块表面，类似藤蔓的多方向附着
    SculkBlocks::SCULK_VEIN = &registry.registerBlock<blocks::SculkVeinBlock>(ResourceLocation("minecraft:sculk_vein"),
        BlockProperties(Material::SCULK)
            .noCollision()
            .notSolid()
            .hardness(0.2f)
            .resistance(0.2f)
            .harvestTool(HarvestTool::Hoe)
            .requiresTool()
            .soundType(BlockSoundTypes::SCULK_VEIN));

    // 幽匿催化体 - Material::SCULK, 锄有效, 硬度3.0, 抗性3.0, 发光等级6
    // 生物在此附近死亡时会生成幽匿块，BLOOM属性标记脉冲状态
    SculkBlocks::SCULK_CATALYST =
        &registry.registerBlock<blocks::SculkCatalystBlock>(ResourceLocation("minecraft:sculk_catalyst"),
            BlockProperties(Material::SCULK)
                .hardness(3.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Hoe)
                .requiresTool()
                .soundType(BlockSoundTypes::SCULK_CATALYST)
                .lightLevel(6));

    // 幽匿感测体 - Material::SCULK, 非固体, 锄有效, 硬度1.5, 抗性1.5
    // 检测振动并发出红石信号，SCULK_SENSOR_PHASE + POWER + WATERLOGGED
    SculkBlocks::SCULK_SENSOR =
        &registry.registerBlock<blocks::SculkSensorBlock>(ResourceLocation("minecraft:sculk_sensor"),
            BlockProperties(Material::SCULK)
                .notSolid()
                .hardness(1.5f)
                .resistance(1.5f)
                .harvestTool(HarvestTool::Hoe)
                .requiresTool()
                .soundType(BlockSoundTypes::SCULK_SENSOR));

    // 校准幽匿感测体 - Material::SCULK, 非固体, 锄有效, 硬度1.5, 抗性1.5
    // 可通过红石信号过滤振动频率的高级感测体，FACING + SCULK_SENSOR_PHASE + POWER + WATERLOGGED
    SculkBlocks::CALIBRATED_SCULK_SENSOR = &registry.registerBlock<blocks::CalibratedSculkSensorBlock>(
        ResourceLocation("minecraft:calibrated_sculk_sensor"),
        BlockProperties(Material::SCULK)
            .notSolid()
            .hardness(1.5f)
            .resistance(1.5f)
            .harvestTool(HarvestTool::Hoe)
            .requiresTool()
            .soundType(BlockSoundTypes::SCULK_SENSOR));

    // 幽匿尖啸体 - Material::SCULK, 非固体, 锄有效, 硬度3.0, 抗性3.0
    // 被激活多次后会召唤监守者，SHRIEKING + CAN_SUMMON + WATERLOGGED
    SculkBlocks::SCULK_SHRIEKER =
        &registry.registerBlock<blocks::SculkShriekerBlock>(ResourceLocation("minecraft:sculk_shrieker"),
            BlockProperties(Material::SCULK)
                .notSolid()
                .hardness(3.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Hoe)
                .requiresTool()
                .soundType(BlockSoundTypes::SCULK_SHRIEKER));
}

} // namespace block_registry
} // namespace mc
