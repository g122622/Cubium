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

#include "BlockSoundType.hpp"

namespace mc {

BlockSoundType::BlockSoundType(const ResourceLocation& breakSound,
    const ResourceLocation& stepSound,
    const ResourceLocation& placeSound,
    const ResourceLocation& hitSound,
    const ResourceLocation& fallSound,
    f32 volume,
    f32 pitch)
    : m_breakSound(breakSound)
    , m_stepSound(stepSound)
    , m_placeSound(placeSound)
    , m_hitSound(hitSound)
    , m_fallSound(fallSound)
    , m_volume(volume)
    , m_pitch(pitch)
{}

// ============================================================================
// 预定义声音类型
// ============================================================================

namespace BlockSoundTypes {

// 木头 - 木头碰撞声
const BlockSoundType WOOD(ResourceLocation("minecraft:block.wood.break"),
    ResourceLocation("minecraft:block.wood.step"),
    ResourceLocation("minecraft:block.wood.place"),
    ResourceLocation("minecraft:block.wood.hit"),
    ResourceLocation("minecraft:block.wood.fall"),
    1.0f,
    1.0f);

// 石头 - 石头碰撞声
const BlockSoundType STONE(ResourceLocation("minecraft:block.stone.break"),
    ResourceLocation("minecraft:block.stone.step"),
    ResourceLocation("minecraft:block.stone.place"),
    ResourceLocation("minecraft:block.stone.hit"),
    ResourceLocation("minecraft:block.stone.fall"),
    1.0f,
    1.0f);

// 泥土 - 柔软的泥土声
const BlockSoundType DIRT(ResourceLocation("minecraft:block.gravel.break"),
    ResourceLocation("minecraft:block.gravel.step"),
    ResourceLocation("minecraft:block.gravel.place"),
    ResourceLocation("minecraft:block.gravel.hit"),
    ResourceLocation("minecraft:block.gravel.fall"),
    1.0f,
    1.0f);

// 草方块 - 草地声
const BlockSoundType GRASS(ResourceLocation("minecraft:block.grass.break"),
    ResourceLocation("minecraft:block.grass.step"),
    ResourceLocation("minecraft:block.grass.place"),
    ResourceLocation("minecraft:block.grass.hit"),
    ResourceLocation("minecraft:block.grass.fall"),
    1.0f,
    1.0f);

// 沙子 - 沙沙声
const BlockSoundType SAND(ResourceLocation("minecraft:block.sand.break"),
    ResourceLocation("minecraft:block.sand.step"),
    ResourceLocation("minecraft:block.sand.place"),
    ResourceLocation("minecraft:block.sand.hit"),
    ResourceLocation("minecraft:block.sand.fall"),
    1.0f,
    1.0f);

// 砾石 - 碎石声
const BlockSoundType GRAVEL(ResourceLocation("minecraft:block.gravel.break"),
    ResourceLocation("minecraft:block.gravel.step"),
    ResourceLocation("minecraft:block.gravel.place"),
    ResourceLocation("minecraft:block.gravel.hit"),
    ResourceLocation("minecraft:block.gravel.fall"),
    1.0f,
    1.0f);

// 玻璃 - 清脆的玻璃声
const BlockSoundType GLASS(ResourceLocation("minecraft:block.glass.break"),
    ResourceLocation("minecraft:block.glass.step"),
    ResourceLocation("minecraft:block.glass.place"),
    ResourceLocation("minecraft:block.glass.hit"),
    ResourceLocation("minecraft:block.glass.fall"),
    1.0f,
    1.0f);

// 金属 - 金属碰撞声
const BlockSoundType METAL(ResourceLocation("minecraft:block.metal.break"),
    ResourceLocation("minecraft:block.metal.step"),
    ResourceLocation("minecraft:block.metal.place"),
    ResourceLocation("minecraft:block.metal.hit"),
    ResourceLocation("minecraft:block.metal.fall"),
    1.0f,
    1.0f);

// 水 - 水声
const BlockSoundType WATER(ResourceLocation("minecraft:block.water.ambient"),
    ResourceLocation("minecraft:block.water.step"),
    ResourceLocation("minecraft:block.water.place"),
    ResourceLocation("minecraft:block.water.hit"),
    ResourceLocation("minecraft:block.water.fall"),
    1.0f,
    1.0f);

// 岩浆 - 岩浆声
const BlockSoundType LAVA(ResourceLocation("minecraft:block.lava.ambient"),
    ResourceLocation("minecraft:block.lava.step"),
    ResourceLocation("minecraft:block.lava.place"),
    ResourceLocation("minecraft:block.lava.hit"),
    ResourceLocation("minecraft:block.lava.fall"),
    1.0f,
    1.0f);

// 雪 - 柔软的雪声
const BlockSoundType SNOW(ResourceLocation("minecraft:block.snow.break"),
    ResourceLocation("minecraft:block.snow.step"),
    ResourceLocation("minecraft:block.snow.place"),
    ResourceLocation("minecraft:block.snow.hit"),
    ResourceLocation("minecraft:block.snow.fall"),
    1.0f,
    1.0f);

// 叶子 - 叶子沙沙声
const BlockSoundType LEAVES(ResourceLocation("minecraft:block.grass.break"),
    ResourceLocation("minecraft:block.grass.step"),
    ResourceLocation("minecraft:block.grass.place"),
    ResourceLocation("minecraft:block.grass.hit"),
    ResourceLocation("minecraft:block.grass.fall"),
    1.0f,
    1.0f);

// 羊毛 - 柔软的羊毛声
const BlockSoundType WOOL(ResourceLocation("minecraft:block.wool.break"),
    ResourceLocation("minecraft:block.wool.step"),
    ResourceLocation("minecraft:block.wool.place"),
    ResourceLocation("minecraft:block.wool.hit"),
    ResourceLocation("minecraft:block.wool.fall"),
    1.0f,
    1.0f);

// 地狱岩 - 地狱岩声
const BlockSoundType NETHERRACK(ResourceLocation("minecraft:block.netherrack.break"),
    ResourceLocation("minecraft:block.netherrack.step"),
    ResourceLocation("minecraft:block.netherrack.place"),
    ResourceLocation("minecraft:block.netherrack.hit"),
    ResourceLocation("minecraft:block.netherrack.fall"),
    1.0f,
    1.0f);

// 灵魂沙 - 灵魂沙声
const BlockSoundType SOUL_SAND(ResourceLocation("minecraft:block.soul_sand.break"),
    ResourceLocation("minecraft:block.soul_sand.step"),
    ResourceLocation("minecraft:block.soul_sand.place"),
    ResourceLocation("minecraft:block.soul_sand.hit"),
    ResourceLocation("minecraft:block.soul_sand.fall"),
    1.0f,
    1.0f);

// 灵魂土 - 灵魂土声
const BlockSoundType SOUL_SOIL(ResourceLocation("minecraft:block.soul_soil.break"),
    ResourceLocation("minecraft:block.soul_soil.step"),
    ResourceLocation("minecraft:block.soul_soil.place"),
    ResourceLocation("minecraft:block.soul_soil.hit"),
    ResourceLocation("minecraft:block.soul_soil.fall"),
    1.0f,
    1.0f);

// 基岩 - 玄武岩声
const BlockSoundType BASALT(ResourceLocation("minecraft:block.basalt.break"),
    ResourceLocation("minecraft:block.basalt.step"),
    ResourceLocation("minecraft:block.basalt.place"),
    ResourceLocation("minecraft:block.basalt.hit"),
    ResourceLocation("minecraft:block.basalt.fall"),
    1.0f,
    1.0f);

// 骨头 - 骨块声
const BlockSoundType BONE(ResourceLocation("minecraft:block.bone_block.break"),
    ResourceLocation("minecraft:block.bone_block.step"),
    ResourceLocation("minecraft:block.bone_block.place"),
    ResourceLocation("minecraft:block.bone_block.hit"),
    ResourceLocation("minecraft:block.bone_block.fall"),
    1.0f,
    1.0f);

// 下界金矿 - 下界金矿声
const BlockSoundType NETHER_GOLD_ORE(ResourceLocation("minecraft:block.nether_gold_ore.break"),
    ResourceLocation("minecraft:block.nether_gold_ore.step"),
    ResourceLocation("minecraft:block.nether_gold_ore.place"),
    ResourceLocation("minecraft:block.nether_gold_ore.hit"),
    ResourceLocation("minecraft:block.nether_gold_ore.fall"),
    1.0f,
    1.0f);

// 下界合金块 - 下界合金声
const BlockSoundType NETHERITE(ResourceLocation("minecraft:block.netherite_block.break"),
    ResourceLocation("minecraft:block.netherite_block.step"),
    ResourceLocation("minecraft:block.netherite_block.place"),
    ResourceLocation("minecraft:block.netherite_block.hit"),
    ResourceLocation("minecraft:block.netherite_block.fall"),
    1.0f,
    1.0f);

// 古代遗迹 - 古代遗迹声
const BlockSoundType ANCIENT_DEBRIS(ResourceLocation("minecraft:block.ancient_debris.break"),
    ResourceLocation("minecraft:block.ancient_debris.step"),
    ResourceLocation("minecraft:block.ancient_debris.place"),
    ResourceLocation("minecraft:block.ancient_debris.hit"),
    ResourceLocation("minecraft:block.ancient_debris.fall"),
    1.0f,
    1.0f);

// 锚 - 重生锚声
const BlockSoundType RESPAWN_ANCHOR(ResourceLocation("minecraft:block.respawn_anchor.break"),
    ResourceLocation("minecraft:block.respawn_anchor.step"),
    ResourceLocation("minecraft:block.respawn_anchor.place"),
    ResourceLocation("minecraft:block.respawn_anchor.hit"),
    ResourceLocation("minecraft:block.respawn_anchor.fall"),
    1.0f,
    1.0f);

// 紫水晶 - 紫水晶声
const BlockSoundType AMETHYST(ResourceLocation("minecraft:block.amethyst_block.break"),
    ResourceLocation("minecraft:block.amethyst_block.step"),
    ResourceLocation("minecraft:block.amethyst_block.place"),
    ResourceLocation("minecraft:block.amethyst_block.hit"),
    ResourceLocation("minecraft:block.amethyst_block.fall"),
    1.0f,
    1.0f);

// 铜块 - 铜块声
const BlockSoundType COPPER(ResourceLocation("minecraft:block.copper.break"),
    ResourceLocation("minecraft:block.copper.step"),
    ResourceLocation("minecraft:block.copper.place"),
    ResourceLocation("minecraft:block.copper.hit"),
    ResourceLocation("minecraft:block.copper.fall"),
    1.0f,
    1.0f);

// 深板岩 - 深板岩声
const BlockSoundType DEEPSLATE(ResourceLocation("minecraft:block.deepslate.break"),
    ResourceLocation("minecraft:block.deepslate.step"),
    ResourceLocation("minecraft:block.deepslate.place"),
    ResourceLocation("minecraft:block.deepslate.hit"),
    ResourceLocation("minecraft:block.deepslate.fall"),
    1.0f,
    1.0f);

// 凝灰岩 - 凝灰岩声
const BlockSoundType TUFF(ResourceLocation("minecraft:block.tuff.break"),
    ResourceLocation("minecraft:block.tuff.step"),
    ResourceLocation("minecraft:block.tuff.place"),
    ResourceLocation("minecraft:block.tuff.hit"),
    ResourceLocation("minecraft:block.tuff.fall"),
    1.0f,
    1.0f);

// 浮冰 - 冰声
const BlockSoundType PACKED_ICE(ResourceLocation("minecraft:block.packed_ice.break"),
    ResourceLocation("minecraft:block.packed_ice.step"),
    ResourceLocation("minecraft:block.packed_ice.place"),
    ResourceLocation("minecraft:block.packed_ice.hit"),
    ResourceLocation("minecraft:block.packed_ice.fall"),
    1.0f,
    1.0f);

// 冰 - 冰声
const BlockSoundType ICE(ResourceLocation("minecraft:block.ice.break"),
    ResourceLocation("minecraft:block.ice.step"),
    ResourceLocation("minecraft:block.ice.place"),
    ResourceLocation("minecraft:block.ice.hit"),
    ResourceLocation("minecraft:block.ice.fall"),
    1.0f,
    1.0f);

// 萤石 - 萤石声
const BlockSoundType GLOWSTONE(ResourceLocation("minecraft:block.glowstone.break"),
    ResourceLocation("minecraft:block.glowstone.step"),
    ResourceLocation("minecraft:block.glowstone.place"),
    ResourceLocation("minecraft:block.glowstone.hit"),
    ResourceLocation("minecraft:block.glowstone.fall"),
    1.0f,
    1.0f);

// 海晶石 - 海晶石声
const BlockSoundType PRISMARINE(ResourceLocation("minecraft:block.prismarine.break"),
    ResourceLocation("minecraft:block.prismarine.step"),
    ResourceLocation("minecraft:block.prismarine.place"),
    ResourceLocation("minecraft:block.prismarine.hit"),
    ResourceLocation("minecraft:block.prismarine.fall"),
    1.0f,
    1.0f);

// 海绵 - 海绵声
const BlockSoundType SPONGE(ResourceLocation("minecraft:block.sponge.break"),
    ResourceLocation("minecraft:block.sponge.step"),
    ResourceLocation("minecraft:block.sponge.place"),
    ResourceLocation("minecraft:block.sponge.hit"),
    ResourceLocation("minecraft:block.sponge.fall"),
    1.0f,
    1.0f);

// 湿海绵 - 湿海绵声
const BlockSoundType WET_SPONGE(ResourceLocation("minecraft:block.wet_sponge.break"),
    ResourceLocation("minecraft:block.wet_sponge.step"),
    ResourceLocation("minecraft:block.wet_sponge.place"),
    ResourceLocation("minecraft:block.wet_sponge.hit"),
    ResourceLocation("minecraft:block.wet_sponge.fall"),
    1.0f,
    1.0f);

// 干草块 - 干草声
const BlockSoundType HAY(ResourceLocation("minecraft:block.hay_block.break"),
    ResourceLocation("minecraft:block.hay_block.step"),
    ResourceLocation("minecraft:block.hay_block.place"),
    ResourceLocation("minecraft:block.hay_block.hit"),
    ResourceLocation("minecraft:block.hay_block.fall"),
    1.0f,
    1.0f);

// 地毯 - 布料声
const BlockSoundType CLOTH(ResourceLocation("minecraft:block.wool.break"),
    ResourceLocation("minecraft:block.wool.step"),
    ResourceLocation("minecraft:block.wool.place"),
    ResourceLocation("minecraft:block.wool.hit"),
    ResourceLocation("minecraft:block.wool.fall"),
    1.0f,
    1.0f);

// 空气 - 静音
const BlockSoundType AIR(ResourceLocation("minecraft:block.air.ambient"), // 不存在的声音，用于静音
    ResourceLocation("minecraft:block.air.ambient"),
    ResourceLocation("minecraft:block.air.ambient"),
    ResourceLocation("minecraft:block.air.ambient"),
    ResourceLocation("minecraft:block.air.ambient"),
    0.0f,
    1.0f // 音量为0，静音
);

// ============================================================================
// MC 1.16.5 缺失的声音类型
// ============================================================================

// 地面 - 泥土/砾石变体
const BlockSoundType GROUND(ResourceLocation("minecraft:block.gravel.break"),
    ResourceLocation("minecraft:block.gravel.step"),
    ResourceLocation("minecraft:block.gravel.place"),
    ResourceLocation("minecraft:block.gravel.hit"),
    ResourceLocation("minecraft:block.gravel.fall"),
    1.0f,
    1.0f);

// 植物 - 植物声音
const BlockSoundType PLANT(ResourceLocation("minecraft:block.grass.break"),
    ResourceLocation("minecraft:block.grass.step"),
    ResourceLocation("minecraft:block.grass.place"),
    ResourceLocation("minecraft:block.grass.hit"),
    ResourceLocation("minecraft:block.grass.fall"),
    1.0f,
    1.0f);

// 梯子 - 梯子攀爬声
const BlockSoundType LADDER(ResourceLocation("minecraft:block.ladder.break"),
    ResourceLocation("minecraft:block.ladder.step"),
    ResourceLocation("minecraft:block.ladder.place"),
    ResourceLocation("minecraft:block.ladder.hit"),
    ResourceLocation("minecraft:block.ladder.fall"),
    1.0f,
    1.0f);

// 铁砧 - 金属碰撞声（音量较小）
const BlockSoundType ANVIL(ResourceLocation("minecraft:block.anvil.break"),
    ResourceLocation("minecraft:block.anvil.step"),
    ResourceLocation("minecraft:block.anvil.place"),
    ResourceLocation("minecraft:block.anvil.hit"),
    ResourceLocation("minecraft:block.anvil.fall"),
    0.3f,
    1.0f);

// 黏液块 - 弹性声音
const BlockSoundType SLIME(ResourceLocation("minecraft:block.slime_block.break"),
    ResourceLocation("minecraft:block.slime_block.step"),
    ResourceLocation("minecraft:block.slime_block.place"),
    ResourceLocation("minecraft:block.slime_block.hit"),
    ResourceLocation("minecraft:block.slime_block.fall"),
    1.0f,
    1.0f);

// 蜂蜜块 - 黏稠声音
const BlockSoundType HONEY(ResourceLocation("minecraft:block.honey_block.break"),
    ResourceLocation("minecraft:block.honey_block.step"),
    ResourceLocation("minecraft:block.honey_block.place"),
    ResourceLocation("minecraft:block.honey_block.hit"),
    ResourceLocation("minecraft:block.honey_block.fall"),
    1.0f,
    1.0f);

// 湿草 - 潮湿草声
const BlockSoundType WET_GRASS(ResourceLocation("minecraft:block.wet_grass.break"),
    ResourceLocation("minecraft:block.wet_grass.step"),
    ResourceLocation("minecraft:block.wet_grass.place"),
    ResourceLocation("minecraft:block.wet_grass.hit"),
    ResourceLocation("minecraft:block.wet_grass.fall"),
    1.0f,
    1.0f);

// 珊瑚 - 珊瑚声
const BlockSoundType CORAL(ResourceLocation("minecraft:block.coral_block.break"),
    ResourceLocation("minecraft:block.coral_block.step"),
    ResourceLocation("minecraft:block.coral_block.place"),
    ResourceLocation("minecraft:block.coral_block.hit"),
    ResourceLocation("minecraft:block.coral_block.fall"),
    1.0f,
    1.0f);

// 竹子 - 竹子声音
const BlockSoundType BAMBOO(ResourceLocation("minecraft:block.bamboo.break"),
    ResourceLocation("minecraft:block.bamboo.step"),
    ResourceLocation("minecraft:block.bamboo.place"),
    ResourceLocation("minecraft:block.bamboo.hit"),
    ResourceLocation("minecraft:block.bamboo.fall"),
    1.0f,
    1.0f);

// 竹笋 - 竹笋声音
const BlockSoundType BAMBOO_SAPLING(ResourceLocation("minecraft:block.bamboo_sapling.break"),
    ResourceLocation("minecraft:block.bamboo_sapling.step"),
    ResourceLocation("minecraft:block.bamboo_sapling.place"),
    ResourceLocation("minecraft:block.bamboo_sapling.hit"),
    ResourceLocation("minecraft:block.bamboo_sapling.fall"),
    1.0f,
    1.0f);

// 脚手架 - 脚手架声音
const BlockSoundType SCAFFOLDING(ResourceLocation("minecraft:block.scaffolding.break"),
    ResourceLocation("minecraft:block.scaffolding.step"),
    ResourceLocation("minecraft:block.scaffolding.place"),
    ResourceLocation("minecraft:block.scaffolding.hit"),
    ResourceLocation("minecraft:block.scaffolding.fall"),
    1.0f,
    1.0f);

// 甜浆果丛 - 甜浆果丛声音
const BlockSoundType SWEET_BERRY_BUSH(ResourceLocation("minecraft:block.sweet_berry_bush.break"),
    ResourceLocation("minecraft:block.sweet_berry_bush.step"),
    ResourceLocation("minecraft:block.sweet_berry_bush.place"),
    ResourceLocation("minecraft:block.sweet_berry_bush.hit"),
    ResourceLocation("minecraft:block.sweet_berry_bush.fall"),
    1.0f,
    1.0f);

// 农作物 - 农作物声音
const BlockSoundType CROP(ResourceLocation("minecraft:block.crop.break"),
    ResourceLocation("minecraft:block.crop.step"),
    ResourceLocation("minecraft:block.crop.place"),
    ResourceLocation("minecraft:block.crop.hit"),
    ResourceLocation("minecraft:block.crop.fall"),
    1.0f,
    1.0f);

// 菌柄 - 下界木质茎
const BlockSoundType STEM(ResourceLocation("minecraft:block.stem.break"),
    ResourceLocation("minecraft:block.stem.step"),
    ResourceLocation("minecraft:block.stem.place"),
    ResourceLocation("minecraft:block.stem.hit"),
    ResourceLocation("minecraft:block.stem.fall"),
    1.0f,
    1.0f);

// 藤蔓 - 藤蔓声音
const BlockSoundType VINE(ResourceLocation("minecraft:block.vine.break"),
    ResourceLocation("minecraft:block.vine.step"),
    ResourceLocation("minecraft:block.vine.place"),
    ResourceLocation("minecraft:block.vine.hit"),
    ResourceLocation("minecraft:block.vine.fall"),
    1.0f,
    1.0f);

// 地狱疣 - 地狱疣声音
const BlockSoundType NETHER_WART(ResourceLocation("minecraft:block.nether_wart.break"),
    ResourceLocation("minecraft:block.nether_wart.step"),
    ResourceLocation("minecraft:block.nether_wart.place"),
    ResourceLocation("minecraft:block.nether_wart.hit"),
    ResourceLocation("minecraft:block.nether_wart.fall"),
    1.0f,
    1.0f);

// 灯笼 - 灯笼声音
const BlockSoundType LANTERN(ResourceLocation("minecraft:block.lantern.break"),
    ResourceLocation("minecraft:block.lantern.step"),
    ResourceLocation("minecraft:block.lantern.place"),
    ResourceLocation("minecraft:block.lantern.hit"),
    ResourceLocation("minecraft:block.lantern.fall"),
    1.0f,
    1.0f);

// 菌核 - 下界木质菌核
const BlockSoundType HYPHAE(ResourceLocation("minecraft:block.hyphae.break"),
    ResourceLocation("minecraft:block.hyphae.step"),
    ResourceLocation("minecraft:block.hyphae.place"),
    ResourceLocation("minecraft:block.hyphae.hit"),
    ResourceLocation("minecraft:block.hyphae.fall"),
    1.0f,
    1.0f);

// 菌岩 - 菌岩声音
const BlockSoundType NYLIUM(ResourceLocation("minecraft:block.nylium.break"),
    ResourceLocation("minecraft:block.nylium.step"),
    ResourceLocation("minecraft:block.nylium.place"),
    ResourceLocation("minecraft:block.nylium.hit"),
    ResourceLocation("minecraft:block.nylium.fall"),
    1.0f,
    1.0f);

// 真菌 - 真菌声音
const BlockSoundType FUNGUS(ResourceLocation("minecraft:block.fungus.break"),
    ResourceLocation("minecraft:block.fungus.step"),
    ResourceLocation("minecraft:block.fungus.place"),
    ResourceLocation("minecraft:block.fungus.hit"),
    ResourceLocation("minecraft:block.fungus.fall"),
    1.0f,
    1.0f);

// 菌索 - 菌索声音
const BlockSoundType ROOT(ResourceLocation("minecraft:block.root.break"),
    ResourceLocation("minecraft:block.root.step"),
    ResourceLocation("minecraft:block.root.place"),
    ResourceLocation("minecraft:block.root.hit"),
    ResourceLocation("minecraft:block.root.fall"),
    1.0f,
    1.0f);

// 菌光体 - 菌光体声音
const BlockSoundType SHROOMLIGHT(ResourceLocation("minecraft:block.shroomlight.break"),
    ResourceLocation("minecraft:block.shroomlight.step"),
    ResourceLocation("minecraft:block.shroomlight.place"),
    ResourceLocation("minecraft:block.shroomlight.hit"),
    ResourceLocation("minecraft:block.shroomlight.fall"),
    1.0f,
    1.0f);

// 下界藤蔓 - 下界藤蔓声音
const BlockSoundType NETHER_VINE(ResourceLocation("minecraft:block.nether_vine.break"),
    ResourceLocation("minecraft:block.nether_vine.step"),
    ResourceLocation("minecraft:block.nether_vine.place"),
    ResourceLocation("minecraft:block.nether_vine.hit"),
    ResourceLocation("minecraft:block.nether_vine.fall"),
    1.0f,
    1.0f);

// 下界藤蔓（低音调）
const BlockSoundType NETHER_VINE_LOWER_PITCH(ResourceLocation("minecraft:block.nether_vine.break"),
    ResourceLocation("minecraft:block.nether_vine.step"),
    ResourceLocation("minecraft:block.nether_vine.place"),
    ResourceLocation("minecraft:block.nether_vine.hit"),
    ResourceLocation("minecraft:block.nether_vine.fall"),
    1.0f,
    0.9f // 音调稍低
);

// 疣块 - 疣块声音
const BlockSoundType WART(ResourceLocation("minecraft:block.wart_block.break"),
    ResourceLocation("minecraft:block.wart_block.step"),
    ResourceLocation("minecraft:block.wart_block.place"),
    ResourceLocation("minecraft:block.wart_block.hit"),
    ResourceLocation("minecraft:block.wart_block.fall"),
    1.0f,
    1.0f);

// 下界砖 - 下界砖声音
const BlockSoundType NETHER_BRICK(ResourceLocation("minecraft:block.nether_bricks.break"),
    ResourceLocation("minecraft:block.nether_bricks.step"),
    ResourceLocation("minecraft:block.nether_bricks.place"),
    ResourceLocation("minecraft:block.nether_bricks.hit"),
    ResourceLocation("minecraft:block.nether_bricks.fall"),
    1.0f,
    1.0f);

// 下界苗 - 下界苗声音
const BlockSoundType NETHER_SPROUT(ResourceLocation("minecraft:block.nether_sprouts.break"),
    ResourceLocation("minecraft:block.nether_sprouts.step"),
    ResourceLocation("minecraft:block.nether_sprouts.place"),
    ResourceLocation("minecraft:block.nether_sprouts.hit"),
    ResourceLocation("minecraft:block.nether_sprouts.fall"),
    1.0f,
    1.0f);

// 下界矿石 - 下界矿石声音
const BlockSoundType NETHER_ORE(ResourceLocation("minecraft:block.nether_ore.break"),
    ResourceLocation("minecraft:block.nether_ore.step"),
    ResourceLocation("minecraft:block.nether_ore.place"),
    ResourceLocation("minecraft:block.nether_ore.hit"),
    ResourceLocation("minecraft:block.nether_ore.fall"),
    1.0f,
    1.0f);

// 磁石 - 磁石声音
const BlockSoundType LODESTONE(ResourceLocation("minecraft:block.lodestone.break"),
    ResourceLocation("minecraft:block.lodestone.step"),
    ResourceLocation("minecraft:block.lodestone.place"),
    ResourceLocation("minecraft:block.lodestone.hit"),
    ResourceLocation("minecraft:block.lodestone.fall"),
    1.0f,
    1.0f);

// 锁链 - 锁链声音
const BlockSoundType CHAIN(ResourceLocation("minecraft:block.chain.break"),
    ResourceLocation("minecraft:block.chain.step"),
    ResourceLocation("minecraft:block.chain.place"),
    ResourceLocation("minecraft:block.chain.hit"),
    ResourceLocation("minecraft:block.chain.fall"),
    1.0f,
    1.0f);

// 镶金黑石 - 镶金黑石声音
const BlockSoundType GILDED_BLACKSTONE(ResourceLocation("minecraft:block.gilded_blackstone.break"),
    ResourceLocation("minecraft:block.gilded_blackstone.step"),
    ResourceLocation("minecraft:block.gilded_blackstone.place"),
    ResourceLocation("minecraft:block.gilded_blackstone.hit"),
    ResourceLocation("minecraft:block.gilded_blackstone.fall"),
    1.0f,
    1.0f);

void initialize()
{
    // 静态初始化已在全局对象构造时完成
    // 此函数保留用于未来可能的动态加载
}

} // namespace BlockSoundTypes

} // namespace mc
