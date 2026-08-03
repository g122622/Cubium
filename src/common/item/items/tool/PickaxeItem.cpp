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

#include "PickaxeItem.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/tool/ToolItem.hpp"
#include "common/item/items/tool/ToolType.hpp"
#include "common/item/tier/IItemTier.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <unordered_set>

namespace mc {
namespace item {
namespace tool {

PickaxeItem::PickaxeItem(const tier::IItemTier& tier, i32 attackDamage, f32 attackSpeed, ItemProperties properties)
    : ToolItem(static_cast<f32>(attackDamage),
          attackSpeed,
          tier,
          _initializeEffectiveBlocks(),
          ToolType::Pickaxe,
          properties)
{}

bool PickaxeItem::canHarvestBlock(const BlockState& state) const
{
    // 1. 如果方块需要镐，检查挖掘等级
    if (state.getHarvestTool() == TOOL_TYPE_PICKAXE) {
        return m_tier.getHarvestLevel() >= state.getHarvestLevel();
    }

    // 2. 镐对 ROCK, IRON, ANVIL 材质总是可以采集
    const Material& mat = state.getMaterial();
    if (mat == Material::ROCK || mat == Material::IRON || mat == Material::ANVIL) {
        return true;
    }

    // 其他材质无法采集
    return false;
}

f32 PickaxeItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const
{
    // 镐对 ROCK, IRON, ANVIL 材质有高效率
    const Material& mat = state.getMaterial();
    if (mat == Material::ROCK || mat == Material::IRON || mat == Material::ANVIL) {
        return m_efficiency;
    }

    // 检查特定方块
    if (isEffectiveBlock(state.owner())) {
        return m_efficiency;
    }

    return 1.0f;
}

bool PickaxeItem::isEffectiveMaterial(const Material& material) const
{
    return material == Material::ROCK || material == Material::IRON || material == Material::ANVIL;
}

std::unordered_set<const Block*> PickaxeItem::_initializeEffectiveBlocks()
{
    std::unordered_set<const Block*> blocks;

    // 基础石头类
    if (VanillaBlocks::STONE) blocks.insert(VanillaBlocks::STONE);
    if (VanillaBlocks::COBBLESTONE) blocks.insert(VanillaBlocks::COBBLESTONE);
    if (VanillaBlocks::MOSSY_COBBLESTONE) blocks.insert(VanillaBlocks::MOSSY_COBBLESTONE);

    // 矿石类
    if (VanillaBlocks::COAL_ORE) blocks.insert(VanillaBlocks::COAL_ORE);
    if (VanillaBlocks::IRON_ORE) blocks.insert(VanillaBlocks::IRON_ORE);
    if (VanillaBlocks::GOLD_ORE) blocks.insert(VanillaBlocks::GOLD_ORE);
    if (VanillaBlocks::DIAMOND_ORE) blocks.insert(VanillaBlocks::DIAMOND_ORE);
    if (VanillaBlocks::EMERALD_ORE) blocks.insert(VanillaBlocks::EMERALD_ORE);
    if (VanillaBlocks::LAPIS_ORE) blocks.insert(VanillaBlocks::LAPIS_ORE);
    if (VanillaBlocks::REDSTONE_ORE) blocks.insert(VanillaBlocks::REDSTONE_ORE);
    if (VanillaBlocks::COPPER_ORE) blocks.insert(VanillaBlocks::COPPER_ORE);

    // 石头变种
    if (VanillaBlocks::GRANITE) blocks.insert(VanillaBlocks::GRANITE);
    if (VanillaBlocks::DIORITE) blocks.insert(VanillaBlocks::DIORITE);
    if (VanillaBlocks::ANDESITE) blocks.insert(VanillaBlocks::ANDESITE);
    if (VanillaBlocks::POLISHED_GRANITE) blocks.insert(VanillaBlocks::POLISHED_GRANITE);
    if (VanillaBlocks::POLISHED_DIORITE) blocks.insert(VanillaBlocks::POLISHED_DIORITE);
    if (VanillaBlocks::POLISHED_ANDESITE) blocks.insert(VanillaBlocks::POLISHED_ANDESITE);

    // 矿物方块
    if (VanillaBlocks::IRON_BLOCK) blocks.insert(VanillaBlocks::IRON_BLOCK);
    if (VanillaBlocks::GOLD_BLOCK) blocks.insert(VanillaBlocks::GOLD_BLOCK);
    if (VanillaBlocks::DIAMOND_BLOCK) blocks.insert(VanillaBlocks::DIAMOND_BLOCK);
    if (VanillaBlocks::EMERALD_BLOCK) blocks.insert(VanillaBlocks::EMERALD_BLOCK);
    if (VanillaBlocks::LAPIS_BLOCK) blocks.insert(VanillaBlocks::LAPIS_BLOCK);
    if (VanillaBlocks::REDSTONE_BLOCK) blocks.insert(VanillaBlocks::REDSTONE_BLOCK);

    // 下界矿石
    if (VanillaBlocks::NETHER_QUARTZ_ORE) blocks.insert(VanillaBlocks::NETHER_QUARTZ_ORE);
    if (VanillaBlocks::NETHER_GOLD_ORE) blocks.insert(VanillaBlocks::NETHER_GOLD_ORE);
    if (VanillaBlocks::ANCIENT_DEBRIS) blocks.insert(VanillaBlocks::ANCIENT_DEBRIS);

    // 石砖系列
    if (VanillaBlocks::STONE_BRICKS) blocks.insert(VanillaBlocks::STONE_BRICKS);
    if (VanillaBlocks::MOSSY_STONE_BRICKS) blocks.insert(VanillaBlocks::MOSSY_STONE_BRICKS);
    if (VanillaBlocks::CRACKED_STONE_BRICKS) blocks.insert(VanillaBlocks::CRACKED_STONE_BRICKS);
    if (VanillaBlocks::CHISELED_STONE_BRICKS) blocks.insert(VanillaBlocks::CHISELED_STONE_BRICKS);

    // 砂岩系列
    if (VanillaBlocks::SANDSTONE) blocks.insert(VanillaBlocks::SANDSTONE);
    if (VanillaBlocks::RED_SANDSTONE) blocks.insert(VanillaBlocks::RED_SANDSTONE);
    if (VanillaBlocks::CHISELED_SANDSTONE) blocks.insert(VanillaBlocks::CHISELED_SANDSTONE);
    if (VanillaBlocks::CUT_SANDSTONE) blocks.insert(VanillaBlocks::CUT_SANDSTONE);

    // 石英方块
    if (VanillaBlocks::QUARTZ_BLOCK) blocks.insert(VanillaBlocks::QUARTZ_BLOCK);
    if (VanillaBlocks::CHISELED_QUARTZ_BLOCK) blocks.insert(VanillaBlocks::CHISELED_QUARTZ_BLOCK);
    if (VanillaBlocks::QUARTZ_PILLAR) blocks.insert(VanillaBlocks::QUARTZ_PILLAR);

    // 紫珀方块
    if (VanillaBlocks::PURPUR_BLOCK) blocks.insert(VanillaBlocks::PURPUR_BLOCK);
    if (VanillaBlocks::PURPUR_PILLAR) blocks.insert(VanillaBlocks::PURPUR_PILLAR);

    // 海晶系列
    if (VanillaBlocks::PRISMARINE) blocks.insert(VanillaBlocks::PRISMARINE);
    if (VanillaBlocks::PRISMARINE_BRICKS) blocks.insert(VanillaBlocks::PRISMARINE_BRICKS);
    if (VanillaBlocks::DARK_PRISMARINE) blocks.insert(VanillaBlocks::DARK_PRISMARINE);
    if (VanillaBlocks::SEA_LANTERN) blocks.insert(VanillaBlocks::SEA_LANTERN);

    // 黑石系列
    if (VanillaBlocks::BLACKSTONE) blocks.insert(VanillaBlocks::BLACKSTONE);
    if (VanillaBlocks::POLISHED_BLACKSTONE) blocks.insert(VanillaBlocks::POLISHED_BLACKSTONE);

    // 末地石系列
    if (VanillaBlocks::END_STONE) blocks.insert(VanillaBlocks::END_STONE);
    if (VanillaBlocks::END_STONE_BRICKS) blocks.insert(VanillaBlocks::END_STONE_BRICKS);

    // 其他石头类
    if (VanillaBlocks::BRICKS) blocks.insert(VanillaBlocks::BRICKS);
    if (VanillaBlocks::OBSIDIAN) blocks.insert(VanillaBlocks::OBSIDIAN);
    if (VanillaBlocks::CRYING_OBSIDIAN) blocks.insert(VanillaBlocks::CRYING_OBSIDIAN);
    if (VanillaBlocks::NETHERRACK) blocks.insert(VanillaBlocks::NETHERRACK);
    if (VanillaBlocks::BASALT) blocks.insert(VanillaBlocks::BASALT);
    if (VanillaBlocks::POLISHED_BASALT) blocks.insert(VanillaBlocks::POLISHED_BASALT);
    if (VanillaBlocks::GLOWSTONE) blocks.insert(VanillaBlocks::GLOWSTONE);
    if (VanillaBlocks::MAGMA) blocks.insert(VanillaBlocks::MAGMA);

    // 冰类
    if (VanillaBlocks::ICE) blocks.insert(VanillaBlocks::ICE);
    if (VanillaBlocks::PACKED_ICE) blocks.insert(VanillaBlocks::PACKED_ICE);
    if (VanillaBlocks::BLUE_ICE) blocks.insert(VanillaBlocks::BLUE_ICE);

    // 骨块
    if (VanillaBlocks::BONE_BLOCK) blocks.insert(VanillaBlocks::BONE_BLOCK);

    // 信标和潮涌核心
    if (VanillaBlocks::BEACON) blocks.insert(VanillaBlocks::BEACON);
    if (VanillaBlocks::CONDUIT) blocks.insert(VanillaBlocks::CONDUIT);

    // 铁轨
    if (VanillaBlocks::RAIL) blocks.insert(VanillaBlocks::RAIL);
    if (VanillaBlocks::POWERED_RAIL) blocks.insert(VanillaBlocks::POWERED_RAIL);
    if (VanillaBlocks::DETECTOR_RAIL) blocks.insert(VanillaBlocks::DETECTOR_RAIL);
    if (VanillaBlocks::ACTIVATOR_RAIL) blocks.insert(VanillaBlocks::ACTIVATOR_RAIL);

    // 铁制品
    if (VanillaBlocks::IRON_DOOR) blocks.insert(VanillaBlocks::IRON_DOOR);
    if (VanillaBlocks::IRON_TRAPDOOR) blocks.insert(VanillaBlocks::IRON_TRAPDOOR);
    if (VanillaBlocks::IRON_BARS) blocks.insert(VanillaBlocks::IRON_BARS);

    // 光照方块（灯笼、锁链）
    if (VanillaBlocks::LANTERN) blocks.insert(VanillaBlocks::LANTERN);
    if (VanillaBlocks::SOUL_LANTERN) blocks.insert(VanillaBlocks::SOUL_LANTERN);
    if (VanillaBlocks::CHAIN) blocks.insert(VanillaBlocks::CHAIN);

    // 按钮/压力板
    if (VanillaBlocks::STONE_BUTTON) blocks.insert(VanillaBlocks::STONE_BUTTON);
    if (VanillaBlocks::STONE_PRESSURE_PLATE) blocks.insert(VanillaBlocks::STONE_PRESSURE_PLATE);

    // 功能方块
    if (VanillaBlocks::DISPENSER) blocks.insert(VanillaBlocks::DISPENSER);
    if (VanillaBlocks::DROPPER) blocks.insert(VanillaBlocks::DROPPER);
    if (VanillaBlocks::PISTON) blocks.insert(VanillaBlocks::PISTON);
    if (VanillaBlocks::STICKY_PISTON) blocks.insert(VanillaBlocks::STICKY_PISTON);
    if (VanillaBlocks::ENDER_CHEST) blocks.insert(VanillaBlocks::ENDER_CHEST);
    if (VanillaBlocks::BREWING_STAND) blocks.insert(VanillaBlocks::BREWING_STAND);

    // 台阶（石制）
    if (VanillaBlocks::STONE_SLAB) blocks.insert(VanillaBlocks::STONE_SLAB);
    if (VanillaBlocks::COBBLESTONE_SLAB) blocks.insert(VanillaBlocks::COBBLESTONE_SLAB);
    if (VanillaBlocks::STONE_BRICK_SLAB) blocks.insert(VanillaBlocks::STONE_BRICK_SLAB);
    if (VanillaBlocks::MOSSY_STONE_BRICK_SLAB) blocks.insert(VanillaBlocks::MOSSY_STONE_BRICK_SLAB);
    if (VanillaBlocks::PRISMARINE_SLAB) blocks.insert(VanillaBlocks::PRISMARINE_SLAB);
    if (VanillaBlocks::PRISMARINE_BRICK_SLAB) blocks.insert(VanillaBlocks::PRISMARINE_BRICK_SLAB);
    if (VanillaBlocks::DARK_PRISMARINE_SLAB) blocks.insert(VanillaBlocks::DARK_PRISMARINE_SLAB);

    // 脚手架和梯子
    if (VanillaBlocks::LADDER) blocks.insert(VanillaBlocks::LADDER);
    if (VanillaBlocks::SCAFFOLDING) blocks.insert(VanillaBlocks::SCAFFOLDING);

    // 活塞头
    if (VanillaBlocks::PISTON_HEAD) blocks.insert(VanillaBlocks::PISTON_HEAD);

    return blocks;
}

} // namespace tool
} // namespace item
} // namespace mc
