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
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

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
    1.5f);

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

// 空气 - 原版 SoundType.EMPTY：五个事件都是 SoundEvents.EMPTY（不会发声）
const BlockSoundType AIR(ResourceLocation("minecraft:intentionally_empty"),
    ResourceLocation("minecraft:intentionally_empty"),
    ResourceLocation("minecraft:intentionally_empty"),
    ResourceLocation("minecraft:intentionally_empty"),
    ResourceLocation("minecraft:intentionally_empty"),
    1.0f,
    1.0f);

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
    ResourceLocation("minecraft:block.bamboo.step"),
    ResourceLocation("minecraft:block.bamboo_sapling.place"),
    ResourceLocation("minecraft:block.bamboo_sapling.hit"),
    ResourceLocation("minecraft:block.bamboo.fall"),
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
    ResourceLocation("minecraft:block.grass.step"),
    ResourceLocation("minecraft:block.sweet_berry_bush.place"),
    ResourceLocation("minecraft:block.grass.hit"),
    ResourceLocation("minecraft:block.grass.fall"),
    1.0f,
    1.0f);

// 农作物 - 农作物声音
const BlockSoundType CROP(ResourceLocation("minecraft:block.crop.break"),
    ResourceLocation("minecraft:block.grass.step"),
    ResourceLocation("minecraft:item.crop.plant"),
    ResourceLocation("minecraft:block.grass.hit"),
    ResourceLocation("minecraft:block.grass.fall"),
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

// 下界木 - 绯红/诡异木板、楼梯、台阶、栅栏等使用
const BlockSoundType NETHER_WOOD(ResourceLocation("minecraft:block.nether_wood.break"),
    ResourceLocation("minecraft:block.nether_wood.step"),
    ResourceLocation("minecraft:block.nether_wood.place"),
    ResourceLocation("minecraft:block.nether_wood.hit"),
    ResourceLocation("minecraft:block.nether_wood.fall"),
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
    ResourceLocation("minecraft:block.stone.step"),
    ResourceLocation("minecraft:item.nether_wart.plant"),
    ResourceLocation("minecraft:block.stone.hit"),
    ResourceLocation("minecraft:block.stone.fall"),
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
// 菌根（原版 SoundType.ROOTS）：事件名是 roots.*，不是 root.*
const BlockSoundType ROOTS(ResourceLocation("minecraft:block.roots.break"),
    ResourceLocation("minecraft:block.roots.step"),
    ResourceLocation("minecraft:block.roots.place"),
    ResourceLocation("minecraft:block.roots.hit"),
    ResourceLocation("minecraft:block.roots.fall"),
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
const BlockSoundType NETHER_VINE(ResourceLocation("minecraft:block.weeping_vines.break"),
    ResourceLocation("minecraft:block.weeping_vines.step"),
    ResourceLocation("minecraft:block.weeping_vines.place"),
    ResourceLocation("minecraft:block.weeping_vines.hit"),
    ResourceLocation("minecraft:block.weeping_vines.fall"),
    1.0f,
    1.0f);

// 下界藤蔓（低音调）
const BlockSoundType NETHER_VINE_LOWER_PITCH(ResourceLocation("minecraft:block.weeping_vines.break"),
    ResourceLocation("minecraft:block.weeping_vines.step"),
    ResourceLocation("minecraft:block.weeping_vines.place"),
    ResourceLocation("minecraft:block.weeping_vines.hit"),
    ResourceLocation("minecraft:block.weeping_vines.fall"),
    1.0f,
    0.5f // 音调减半（原版 TWISTING_VINES）
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

// ============================================================================
// 1.17 洞穴与山崖 Part 1
// ============================================================================

// 深板岩系列 - DEEPSLATE 已在前面定义
const BlockSoundType COBBLED_DEEPSLATE(ResourceLocation("minecraft:block.cobbled_deepslate.break"),
    ResourceLocation("minecraft:block.cobbled_deepslate.step"),
    ResourceLocation("minecraft:block.cobbled_deepslate.place"),
    ResourceLocation("minecraft:block.cobbled_deepslate.hit"),
    ResourceLocation("minecraft:block.cobbled_deepslate.fall"),
    1.0f,
    1.0f);

const BlockSoundType POLISHED_DEEPSLATE(ResourceLocation("minecraft:block.polished_deepslate.break"),
    ResourceLocation("minecraft:block.polished_deepslate.step"),
    ResourceLocation("minecraft:block.polished_deepslate.place"),
    ResourceLocation("minecraft:block.polished_deepslate.hit"),
    ResourceLocation("minecraft:block.polished_deepslate.fall"),
    1.0f,
    1.0f);

const BlockSoundType DEEPSLATE_BRICKS(ResourceLocation("minecraft:block.deepslate_bricks.break"),
    ResourceLocation("minecraft:block.deepslate_bricks.step"),
    ResourceLocation("minecraft:block.deepslate_bricks.place"),
    ResourceLocation("minecraft:block.deepslate_bricks.hit"),
    ResourceLocation("minecraft:block.deepslate_bricks.fall"),
    1.0f,
    1.0f);

const BlockSoundType DEEPSLATE_TILES(ResourceLocation("minecraft:block.deepslate_tiles.break"),
    ResourceLocation("minecraft:block.deepslate_tiles.step"),
    ResourceLocation("minecraft:block.deepslate_tiles.place"),
    ResourceLocation("minecraft:block.deepslate_tiles.hit"),
    ResourceLocation("minecraft:block.deepslate_tiles.fall"),
    1.0f,
    1.0f);

// 铜系列 - COPPER 已在前面定义
const BlockSoundType COPPER_BULB(ResourceLocation("minecraft:block.copper_bulb.break"),
    ResourceLocation("minecraft:block.copper_bulb.step"),
    ResourceLocation("minecraft:block.copper_bulb.place"),
    ResourceLocation("minecraft:block.copper_bulb.hit"),
    ResourceLocation("minecraft:block.copper_bulb.fall"),
    1.0f,
    1.0f);

const BlockSoundType COPPER_GRATE(ResourceLocation("minecraft:block.copper_grate.break"),
    ResourceLocation("minecraft:block.copper_grate.step"),
    ResourceLocation("minecraft:block.copper_grate.place"),
    ResourceLocation("minecraft:block.copper_grate.hit"),
    ResourceLocation("minecraft:block.copper_grate.fall"),
    1.0f,
    1.0f);

// 紫水晶系列 - AMETHYST 已在前面定义
const BlockSoundType AMETHYST_CLUSTER(ResourceLocation("minecraft:block.amethyst_cluster.break"),
    ResourceLocation("minecraft:block.amethyst_cluster.step"),
    ResourceLocation("minecraft:block.amethyst_cluster.place"),
    ResourceLocation("minecraft:block.amethyst_cluster.hit"),
    ResourceLocation("minecraft:block.amethyst_cluster.fall"),
    1.0f,
    1.0f);

const BlockSoundType SMALL_AMETHYST_BUD(ResourceLocation("minecraft:block.small_amethyst_bud.break"),
    ResourceLocation("minecraft:block.amethyst_cluster.step"),
    ResourceLocation("minecraft:block.small_amethyst_bud.place"),
    ResourceLocation("minecraft:block.amethyst_cluster.hit"),
    ResourceLocation("minecraft:block.amethyst_cluster.fall"),
    1.0f,
    1.0f);

const BlockSoundType MEDIUM_AMETHYST_BUD(ResourceLocation("minecraft:block.medium_amethyst_bud.break"),
    ResourceLocation("minecraft:block.amethyst_cluster.step"),
    ResourceLocation("minecraft:block.medium_amethyst_bud.place"),
    ResourceLocation("minecraft:block.amethyst_cluster.hit"),
    ResourceLocation("minecraft:block.amethyst_cluster.fall"),
    1.0f,
    1.0f);

const BlockSoundType LARGE_AMETHYST_BUD(ResourceLocation("minecraft:block.large_amethyst_bud.break"),
    ResourceLocation("minecraft:block.amethyst_cluster.step"),
    ResourceLocation("minecraft:block.large_amethyst_bud.place"),
    ResourceLocation("minecraft:block.amethyst_cluster.hit"),
    ResourceLocation("minecraft:block.amethyst_cluster.fall"),
    1.0f,
    1.0f);

// 洞穴装饰
const BlockSoundType MOSS(ResourceLocation("minecraft:block.moss.break"),
    ResourceLocation("minecraft:block.moss.step"),
    ResourceLocation("minecraft:block.moss.place"),
    ResourceLocation("minecraft:block.moss.hit"),
    ResourceLocation("minecraft:block.moss.fall"),
    1.0f,
    1.0f);

const BlockSoundType MOSS_CARPET(ResourceLocation("minecraft:block.moss_carpet.break"),
    ResourceLocation("minecraft:block.moss_carpet.step"),
    ResourceLocation("minecraft:block.moss_carpet.place"),
    ResourceLocation("minecraft:block.moss_carpet.hit"),
    ResourceLocation("minecraft:block.moss_carpet.fall"),
    1.0f,
    1.0f);

const BlockSoundType AZALEA(ResourceLocation("minecraft:block.azalea.break"),
    ResourceLocation("minecraft:block.azalea.step"),
    ResourceLocation("minecraft:block.azalea.place"),
    ResourceLocation("minecraft:block.azalea.hit"),
    ResourceLocation("minecraft:block.azalea.fall"),
    1.0f,
    1.0f);

const BlockSoundType AZALEA_LEAVES(ResourceLocation("minecraft:block.azalea_leaves.break"),
    ResourceLocation("minecraft:block.azalea_leaves.step"),
    ResourceLocation("minecraft:block.azalea_leaves.place"),
    ResourceLocation("minecraft:block.azalea_leaves.hit"),
    ResourceLocation("minecraft:block.azalea_leaves.fall"),
    1.0f,
    1.0f);

const BlockSoundType CAVE_VINES(ResourceLocation("minecraft:block.cave_vines.break"),
    ResourceLocation("minecraft:block.cave_vines.step"),
    ResourceLocation("minecraft:block.cave_vines.place"),
    ResourceLocation("minecraft:block.cave_vines.hit"),
    ResourceLocation("minecraft:block.cave_vines.fall"),
    1.0f,
    1.0f);

const BlockSoundType SPORE_BLOSSOM(ResourceLocation("minecraft:block.spore_blossom.break"),
    ResourceLocation("minecraft:block.spore_blossom.step"),
    ResourceLocation("minecraft:block.spore_blossom.place"),
    ResourceLocation("minecraft:block.spore_blossom.hit"),
    ResourceLocation("minecraft:block.spore_blossom.fall"),
    1.0f,
    1.0f);

const BlockSoundType DRIPSTONE_BLOCK(ResourceLocation("minecraft:block.dripstone_block.break"),
    ResourceLocation("minecraft:block.dripstone_block.step"),
    ResourceLocation("minecraft:block.dripstone_block.place"),
    ResourceLocation("minecraft:block.dripstone_block.hit"),
    ResourceLocation("minecraft:block.dripstone_block.fall"),
    1.0f,
    1.0f);

const BlockSoundType POINTED_DRIPSTONE(ResourceLocation("minecraft:block.pointed_dripstone.break"),
    ResourceLocation("minecraft:block.pointed_dripstone.step"),
    ResourceLocation("minecraft:block.pointed_dripstone.place"),
    ResourceLocation("minecraft:block.pointed_dripstone.hit"),
    ResourceLocation("minecraft:block.pointed_dripstone.fall"),
    1.0f,
    1.0f);

const BlockSoundType CALCITE(ResourceLocation("minecraft:block.calcite.break"),
    ResourceLocation("minecraft:block.calcite.step"),
    ResourceLocation("minecraft:block.calcite.place"),
    ResourceLocation("minecraft:block.calcite.hit"),
    ResourceLocation("minecraft:block.calcite.fall"),
    1.0f,
    1.0f);

const BlockSoundType POWDER_SNOW(ResourceLocation("minecraft:block.powder_snow.break"),
    ResourceLocation("minecraft:block.powder_snow.step"),
    ResourceLocation("minecraft:block.powder_snow.place"),
    ResourceLocation("minecraft:block.powder_snow.hit"),
    ResourceLocation("minecraft:block.powder_snow.fall"),
    1.0f,
    1.0f);

const BlockSoundType HANGING_ROOTS(ResourceLocation("minecraft:block.hanging_roots.break"),
    ResourceLocation("minecraft:block.hanging_roots.step"),
    ResourceLocation("minecraft:block.hanging_roots.place"),
    ResourceLocation("minecraft:block.hanging_roots.hit"),
    ResourceLocation("minecraft:block.hanging_roots.fall"),
    1.0f,
    1.0f);

const BlockSoundType ROOTED_DIRT(ResourceLocation("minecraft:block.rooted_dirt.break"),
    ResourceLocation("minecraft:block.rooted_dirt.step"),
    ResourceLocation("minecraft:block.rooted_dirt.place"),
    ResourceLocation("minecraft:block.rooted_dirt.hit"),
    ResourceLocation("minecraft:block.rooted_dirt.fall"),
    1.0f,
    1.0f);

const BlockSoundType BIG_DRIPLEAF(ResourceLocation("minecraft:block.big_dripleaf.break"),
    ResourceLocation("minecraft:block.big_dripleaf.step"),
    ResourceLocation("minecraft:block.big_dripleaf.place"),
    ResourceLocation("minecraft:block.big_dripleaf.hit"),
    ResourceLocation("minecraft:block.big_dripleaf.fall"),
    1.0f,
    1.0f);

const BlockSoundType SMALL_DRIPLEAF(ResourceLocation("minecraft:block.small_dripleaf.break"),
    ResourceLocation("minecraft:block.small_dripleaf.step"),
    ResourceLocation("minecraft:block.small_dripleaf.place"),
    ResourceLocation("minecraft:block.small_dripleaf.hit"),
    ResourceLocation("minecraft:block.small_dripleaf.fall"),
    1.0f,
    1.0f);

const BlockSoundType GLOW_LICHEN(ResourceLocation("minecraft:block.grass.break"),
    ResourceLocation("minecraft:block.vine.step"),
    ResourceLocation("minecraft:block.grass.place"),
    ResourceLocation("minecraft:block.grass.hit"),
    ResourceLocation("minecraft:block.grass.fall"),
    1.0f,
    1.0f);

const BlockSoundType FLOWERING_AZALEA(ResourceLocation("minecraft:block.flowering_azalea.break"),
    ResourceLocation("minecraft:block.flowering_azalea.step"),
    ResourceLocation("minecraft:block.flowering_azalea.place"),
    ResourceLocation("minecraft:block.flowering_azalea.hit"),
    ResourceLocation("minecraft:block.flowering_azalea.fall"),
    1.0f,
    1.0f);

// ============================================================================
// 1.19 荒野更新
// ============================================================================

const BlockSoundType SCULK(ResourceLocation("minecraft:block.sculk.break"),
    ResourceLocation("minecraft:block.sculk.step"),
    ResourceLocation("minecraft:block.sculk.place"),
    ResourceLocation("minecraft:block.sculk.hit"),
    ResourceLocation("minecraft:block.sculk.fall"),
    1.0f,
    1.0f);

const BlockSoundType SCULK_CATALYST(ResourceLocation("minecraft:block.sculk_catalyst.break"),
    ResourceLocation("minecraft:block.sculk_catalyst.step"),
    ResourceLocation("minecraft:block.sculk_catalyst.place"),
    ResourceLocation("minecraft:block.sculk_catalyst.hit"),
    ResourceLocation("minecraft:block.sculk_catalyst.fall"),
    1.0f,
    1.0f);

const BlockSoundType SCULK_SENSOR(ResourceLocation("minecraft:block.sculk_sensor.break"),
    ResourceLocation("minecraft:block.sculk_sensor.step"),
    ResourceLocation("minecraft:block.sculk_sensor.place"),
    ResourceLocation("minecraft:block.sculk_sensor.hit"),
    ResourceLocation("minecraft:block.sculk_sensor.fall"),
    1.0f,
    1.0f);

const BlockSoundType SCULK_SHRIEKER(ResourceLocation("minecraft:block.sculk_shrieker.break"),
    ResourceLocation("minecraft:block.sculk_shrieker.step"),
    ResourceLocation("minecraft:block.sculk_shrieker.place"),
    ResourceLocation("minecraft:block.sculk_shrieker.hit"),
    ResourceLocation("minecraft:block.sculk_shrieker.fall"),
    1.0f,
    1.0f);

const BlockSoundType SCULK_VEIN(ResourceLocation("minecraft:block.sculk_vein.break"),
    ResourceLocation("minecraft:block.sculk_vein.step"),
    ResourceLocation("minecraft:block.sculk_vein.place"),
    ResourceLocation("minecraft:block.sculk_vein.hit"),
    ResourceLocation("minecraft:block.sculk_vein.fall"),
    1.0f,
    1.0f);

const BlockSoundType MUD(ResourceLocation("minecraft:block.mud.break"),
    ResourceLocation("minecraft:block.mud.step"),
    ResourceLocation("minecraft:block.mud.place"),
    ResourceLocation("minecraft:block.mud.hit"),
    ResourceLocation("minecraft:block.mud.fall"),
    1.0f,
    1.0f);

const BlockSoundType MUD_BRICKS(ResourceLocation("minecraft:block.mud_bricks.break"),
    ResourceLocation("minecraft:block.mud_bricks.step"),
    ResourceLocation("minecraft:block.mud_bricks.place"),
    ResourceLocation("minecraft:block.mud_bricks.hit"),
    ResourceLocation("minecraft:block.mud_bricks.fall"),
    1.0f,
    1.0f);

const BlockSoundType PACKED_MUD(ResourceLocation("minecraft:block.packed_mud.break"),
    ResourceLocation("minecraft:block.packed_mud.step"),
    ResourceLocation("minecraft:block.packed_mud.place"),
    ResourceLocation("minecraft:block.packed_mud.hit"),
    ResourceLocation("minecraft:block.packed_mud.fall"),
    1.0f,
    1.0f);

const BlockSoundType MUDDY_MANGROVE_ROOTS(ResourceLocation("minecraft:block.muddy_mangrove_roots.break"),
    ResourceLocation("minecraft:block.muddy_mangrove_roots.step"),
    ResourceLocation("minecraft:block.muddy_mangrove_roots.place"),
    ResourceLocation("minecraft:block.muddy_mangrove_roots.hit"),
    ResourceLocation("minecraft:block.muddy_mangrove_roots.fall"),
    1.0f,
    1.0f);

const BlockSoundType MANGROVE_ROOTS(ResourceLocation("minecraft:block.mangrove_roots.break"),
    ResourceLocation("minecraft:block.mangrove_roots.step"),
    ResourceLocation("minecraft:block.mangrove_roots.place"),
    ResourceLocation("minecraft:block.mangrove_roots.hit"),
    ResourceLocation("minecraft:block.mangrove_roots.fall"),
    1.0f,
    1.0f);

const BlockSoundType MANGROVE_WOOD(ResourceLocation("minecraft:block.mangrove_wood.break"),
    ResourceLocation("minecraft:block.mangrove_wood.step"),
    ResourceLocation("minecraft:block.mangrove_wood.place"),
    ResourceLocation("minecraft:block.mangrove_wood.hit"),
    ResourceLocation("minecraft:block.mangrove_wood.fall"),
    1.0f,
    1.0f);

const BlockSoundType FROGLIGHT(ResourceLocation("minecraft:block.froglight.break"),
    ResourceLocation("minecraft:block.froglight.step"),
    ResourceLocation("minecraft:block.froglight.place"),
    ResourceLocation("minecraft:block.froglight.hit"),
    ResourceLocation("minecraft:block.froglight.fall"),
    1.0f,
    1.0f);

const BlockSoundType FROGSPAWN(ResourceLocation("minecraft:block.frogspawn.break"),
    ResourceLocation("minecraft:block.frogspawn.step"),
    ResourceLocation("minecraft:block.frogspawn.place"),
    ResourceLocation("minecraft:block.frogspawn.hit"),
    ResourceLocation("minecraft:block.frogspawn.fall"),
    1.0f,
    1.0f);

// ============================================================================
// 1.20 足迹与故事
// ============================================================================

const BlockSoundType CHERRY_WOOD(ResourceLocation("minecraft:block.cherry_wood.break"),
    ResourceLocation("minecraft:block.cherry_wood.step"),
    ResourceLocation("minecraft:block.cherry_wood.place"),
    ResourceLocation("minecraft:block.cherry_wood.hit"),
    ResourceLocation("minecraft:block.cherry_wood.fall"),
    1.0f,
    1.0f);

const BlockSoundType CHERRY_LEAVES(ResourceLocation("minecraft:block.cherry_leaves.break"),
    ResourceLocation("minecraft:block.cherry_leaves.step"),
    ResourceLocation("minecraft:block.cherry_leaves.place"),
    ResourceLocation("minecraft:block.cherry_leaves.hit"),
    ResourceLocation("minecraft:block.cherry_leaves.fall"),
    1.0f,
    1.0f);

const BlockSoundType CHERRY_SAPLING(ResourceLocation("minecraft:block.cherry_sapling.break"),
    ResourceLocation("minecraft:block.cherry_sapling.step"),
    ResourceLocation("minecraft:block.cherry_sapling.place"),
    ResourceLocation("minecraft:block.cherry_sapling.hit"),
    ResourceLocation("minecraft:block.cherry_sapling.fall"),
    1.0f,
    1.0f);

const BlockSoundType BAMBOO_WOOD(ResourceLocation("minecraft:block.bamboo_wood.break"),
    ResourceLocation("minecraft:block.bamboo_wood.step"),
    ResourceLocation("minecraft:block.bamboo_wood.place"),
    ResourceLocation("minecraft:block.bamboo_wood.hit"),
    ResourceLocation("minecraft:block.bamboo_wood.fall"),
    1.0f,
    1.0f);

const BlockSoundType SUSPICIOUS_SAND(ResourceLocation("minecraft:block.suspicious_sand.break"),
    ResourceLocation("minecraft:block.suspicious_sand.step"),
    ResourceLocation("minecraft:block.suspicious_sand.place"),
    ResourceLocation("minecraft:block.suspicious_sand.hit"),
    ResourceLocation("minecraft:block.suspicious_sand.fall"),
    1.0f,
    1.0f);

const BlockSoundType SUSPICIOUS_GRAVEL(ResourceLocation("minecraft:block.suspicious_gravel.break"),
    ResourceLocation("minecraft:block.suspicious_gravel.step"),
    ResourceLocation("minecraft:block.suspicious_gravel.place"),
    ResourceLocation("minecraft:block.suspicious_gravel.hit"),
    ResourceLocation("minecraft:block.suspicious_gravel.fall"),
    1.0f,
    1.0f);

const BlockSoundType DECORATED_POT(ResourceLocation("minecraft:block.decorated_pot.break"),
    ResourceLocation("minecraft:block.decorated_pot.step"),
    ResourceLocation("minecraft:block.decorated_pot.place"),
    ResourceLocation("minecraft:block.decorated_pot.hit"),
    ResourceLocation("minecraft:block.decorated_pot.fall"),
    1.0f,
    1.0f);

// 饰纹陶罐被打破（CRACKED 状态）后改用碎裂声：原版 `SoundType.DECORATED_POT_CRACKED`。
const BlockSoundType DECORATED_POT_CRACKED(ResourceLocation("minecraft:block.decorated_pot.shatter"),
    ResourceLocation("minecraft:block.decorated_pot.step"),
    ResourceLocation("minecraft:block.decorated_pot.place"),
    ResourceLocation("minecraft:block.decorated_pot.hit"),
    ResourceLocation("minecraft:block.decorated_pot.fall"),
    1.0f,
    1.0f);

const BlockSoundType CHISELED_BOOKSHELF(ResourceLocation("minecraft:block.chiseled_bookshelf.break"),
    ResourceLocation("minecraft:block.chiseled_bookshelf.step"),
    ResourceLocation("minecraft:block.chiseled_bookshelf.place"),
    ResourceLocation("minecraft:block.chiseled_bookshelf.hit"),
    ResourceLocation("minecraft:block.chiseled_bookshelf.fall"),
    1.0f,
    1.0f);

const BlockSoundType PINK_PETALS(ResourceLocation("minecraft:block.pink_petals.break"),
    ResourceLocation("minecraft:block.pink_petals.step"),
    ResourceLocation("minecraft:block.pink_petals.place"),
    ResourceLocation("minecraft:block.pink_petals.hit"),
    ResourceLocation("minecraft:block.pink_petals.fall"),
    1.0f,
    1.0f);

const BlockSoundType TORCHFLOWER(ResourceLocation("minecraft:block.torchflower.break"),
    ResourceLocation("minecraft:block.torchflower.step"),
    ResourceLocation("minecraft:block.torchflower.place"),
    ResourceLocation("minecraft:block.torchflower.hit"),
    ResourceLocation("minecraft:block.torchflower.fall"),
    1.0f,
    1.0f);

const BlockSoundType PITCHER_CROP(ResourceLocation("minecraft:block.pitcher_crop.break"),
    ResourceLocation("minecraft:block.pitcher_crop.step"),
    ResourceLocation("minecraft:block.pitcher_crop.place"),
    ResourceLocation("minecraft:block.pitcher_crop.hit"),
    ResourceLocation("minecraft:block.pitcher_crop.fall"),
    1.0f,
    1.0f);

const BlockSoundType SNIFFER_EGG(ResourceLocation("minecraft:block.sniffer_egg.break"),
    ResourceLocation("minecraft:block.sniffer_egg.step"),
    ResourceLocation("minecraft:block.sniffer_egg.place"),
    ResourceLocation("minecraft:block.sniffer_egg.hit"),
    ResourceLocation("minecraft:block.sniffer_egg.fall"),
    1.0f,
    1.0f);

// ============================================================================
// 1.21 棘巧试炼
// ============================================================================

// TUFF 已在前面定义
const BlockSoundType POLISHED_TUFF(ResourceLocation("minecraft:block.polished_tuff.break"),
    ResourceLocation("minecraft:block.polished_tuff.step"),
    ResourceLocation("minecraft:block.polished_tuff.place"),
    ResourceLocation("minecraft:block.polished_tuff.hit"),
    ResourceLocation("minecraft:block.polished_tuff.fall"),
    1.0f,
    1.0f);

const BlockSoundType TUFF_BRICKS(ResourceLocation("minecraft:block.tuff_bricks.break"),
    ResourceLocation("minecraft:block.tuff_bricks.step"),
    ResourceLocation("minecraft:block.tuff_bricks.place"),
    ResourceLocation("minecraft:block.tuff_bricks.hit"),
    ResourceLocation("minecraft:block.tuff_bricks.fall"),
    1.0f,
    1.0f);

const BlockSoundType TRIAL_SPAWNER(ResourceLocation("minecraft:block.trial_spawner.break"),
    ResourceLocation("minecraft:block.trial_spawner.step"),
    ResourceLocation("minecraft:block.trial_spawner.place"),
    ResourceLocation("minecraft:block.trial_spawner.hit"),
    ResourceLocation("minecraft:block.trial_spawner.fall"),
    1.0f,
    1.0f);

const BlockSoundType VAULT(ResourceLocation("minecraft:block.vault.break"),
    ResourceLocation("minecraft:block.vault.step"),
    ResourceLocation("minecraft:block.vault.place"),
    ResourceLocation("minecraft:block.vault.hit"),
    ResourceLocation("minecraft:block.vault.fall"),
    1.0f,
    1.0f);

const BlockSoundType CRAFTER(ResourceLocation("minecraft:block.crafter.break"),
    ResourceLocation("minecraft:block.crafter.step"),
    ResourceLocation("minecraft:block.crafter.place"),
    ResourceLocation("minecraft:block.crafter.hit"),
    ResourceLocation("minecraft:block.crafter.fall"),
    1.0f,
    1.0f);

// ============================================================================
// 1.21.2+ 苍白花园
// ============================================================================

const BlockSoundType PALE_MOSS(ResourceLocation("minecraft:block.pale_moss.break"),
    ResourceLocation("minecraft:block.pale_moss.step"),
    ResourceLocation("minecraft:block.pale_moss.place"),
    ResourceLocation("minecraft:block.pale_moss.hit"),
    ResourceLocation("minecraft:block.pale_moss.fall"),
    1.0f,
    1.0f);

const BlockSoundType PALE_MOSS_CARPET(ResourceLocation("minecraft:block.pale_moss_carpet.break"),
    ResourceLocation("minecraft:block.pale_moss_carpet.step"),
    ResourceLocation("minecraft:block.pale_moss_carpet.place"),
    ResourceLocation("minecraft:block.pale_moss_carpet.hit"),
    ResourceLocation("minecraft:block.pale_moss_carpet.fall"),
    1.0f,
    1.0f);

const BlockSoundType PALE_HANGING_MOSS(ResourceLocation("minecraft:block.pale_hanging_moss.break"),
    ResourceLocation("minecraft:block.pale_hanging_moss.step"),
    ResourceLocation("minecraft:block.pale_hanging_moss.place"),
    ResourceLocation("minecraft:block.pale_hanging_moss.hit"),
    ResourceLocation("minecraft:block.pale_hanging_moss.fall"),
    1.0f,
    1.0f);

const BlockSoundType CREAKING_HEART(ResourceLocation("minecraft:block.creaking_heart.break"),
    ResourceLocation("minecraft:block.creaking_heart.step"),
    ResourceLocation("minecraft:block.creaking_heart.place"),
    ResourceLocation("minecraft:block.creaking_heart.hit"),
    ResourceLocation("minecraft:block.creaking_heart.fall"),
    1.0f,
    1.0f);

// 树脂：原版 hit 为 SoundEvents.EMPTY（不可击打出声）
const BlockSoundType RESIN(ResourceLocation("minecraft:block.resin.break"),
    ResourceLocation("minecraft:block.resin.step"),
    ResourceLocation("minecraft:block.resin.place"),
    ResourceLocation("minecraft:intentionally_empty"),
    ResourceLocation("minecraft:block.resin.fall"),
    1.0f,
    1.0f);

const BlockSoundType RESIN_BRICKS(ResourceLocation("minecraft:block.resin_bricks.break"),
    ResourceLocation("minecraft:block.resin_bricks.step"),
    ResourceLocation("minecraft:block.resin_bricks.place"),
    ResourceLocation("minecraft:block.resin_bricks.hit"),
    ResourceLocation("minecraft:block.resin_bricks.fall"),
    1.0f,
    1.0f);

const BlockSoundType HEAVY_CORE(ResourceLocation("minecraft:block.heavy_core.break"),
    ResourceLocation("minecraft:block.heavy_core.step"),
    ResourceLocation("minecraft:block.heavy_core.place"),
    ResourceLocation("minecraft:block.heavy_core.hit"),
    ResourceLocation("minecraft:block.heavy_core.fall"),
    1.0f,
    1.0f);

const BlockSoundType EYEBLOSSOM(ResourceLocation("minecraft:block.eyeblossom.break"),
    ResourceLocation("minecraft:block.eyeblossom.step"),
    ResourceLocation("minecraft:block.eyeblossom.place"),
    ResourceLocation("minecraft:block.eyeblossom.hit"),
    ResourceLocation("minecraft:block.eyeblossom.fall"),
    1.0f,
    1.0f);

// 1.21.4+ 花园觉醒
const BlockSoundType CACTUS_FLOWER(ResourceLocation("minecraft:block.cactus_flower.break"),
    ResourceLocation("minecraft:intentionally_empty"),
    ResourceLocation("minecraft:block.cactus_flower.place"),
    ResourceLocation("minecraft:intentionally_empty"),
    ResourceLocation("minecraft:intentionally_empty"),
    1.0f,
    1.0f);

// 书架（1.21.4+ 新增的各种木质书架）
const BlockSoundType SHELF(ResourceLocation("minecraft:block.shelf.break"),
    ResourceLocation("minecraft:block.shelf.step"),
    ResourceLocation("minecraft:block.shelf.place"),
    ResourceLocation("minecraft:block.shelf.hit"),
    ResourceLocation("minecraft:block.shelf.fall"),
    1.0f,
    1.0f);

// 蜡烛
const BlockSoundType CANDLE(ResourceLocation("minecraft:block.candle.break"),
    ResourceLocation("minecraft:block.candle.step"),
    ResourceLocation("minecraft:block.candle.place"),
    ResourceLocation("minecraft:block.candle.hit"),
    ResourceLocation("minecraft:block.candle.fall"),
    1.0f,
    1.0f);

// 干燥恶魂 - place/hit 音效为空（原版 SoundEvents.EMPTY 的注册名是 minecraft:intentionally_empty）
const BlockSoundType DRIED_GHAST(ResourceLocation("minecraft:block.dried_ghast.break"),
    ResourceLocation("minecraft:block.dried_ghast.step"),
    ResourceLocation("minecraft:intentionally_empty"),
    ResourceLocation("minecraft:intentionally_empty"),
    ResourceLocation("minecraft:block.dried_ghast.fall"),
    1.0f,
    1.0f);

// 悬挂告示牌（橡木等通用木质变体）
const BlockSoundType HANGING_SIGN(ResourceLocation("minecraft:block.hanging_sign.break"),
    ResourceLocation("minecraft:block.hanging_sign.step"),
    ResourceLocation("minecraft:block.hanging_sign.place"),
    ResourceLocation("minecraft:block.hanging_sign.hit"),
    ResourceLocation("minecraft:block.hanging_sign.fall"),
    1.0f,
    1.0f);

// 绯红/诡异悬挂告示牌
const BlockSoundType NETHER_WOOD_HANGING_SIGN(ResourceLocation("minecraft:block.nether_wood_hanging_sign.break"),
    ResourceLocation("minecraft:block.nether_wood_hanging_sign.step"),
    ResourceLocation("minecraft:block.nether_wood_hanging_sign.place"),
    ResourceLocation("minecraft:block.nether_wood_hanging_sign.hit"),
    ResourceLocation("minecraft:block.nether_wood_hanging_sign.fall"),
    1.0f,
    1.0f);

// 竹悬挂告示牌
const BlockSoundType BAMBOO_WOOD_HANGING_SIGN(ResourceLocation("minecraft:block.bamboo_wood_hanging_sign.break"),
    ResourceLocation("minecraft:block.bamboo_wood_hanging_sign.step"),
    ResourceLocation("minecraft:block.bamboo_wood_hanging_sign.place"),
    ResourceLocation("minecraft:block.bamboo_wood_hanging_sign.hit"),
    ResourceLocation("minecraft:block.bamboo_wood_hanging_sign.fall"),
    1.0f,
    1.0f);

// 樱花悬挂告示牌
const BlockSoundType CHERRY_WOOD_HANGING_SIGN(ResourceLocation("minecraft:block.cherry_wood_hanging_sign.break"),
    ResourceLocation("minecraft:block.cherry_wood_hanging_sign.step"),
    ResourceLocation("minecraft:block.cherry_wood_hanging_sign.place"),
    ResourceLocation("minecraft:block.cherry_wood_hanging_sign.hit"),
    ResourceLocation("minecraft:block.cherry_wood_hanging_sign.fall"),
    1.0f,
    1.0f);

// 珊瑚块
const BlockSoundType CORAL_BLOCK(ResourceLocation("minecraft:block.coral_block.break"),
    ResourceLocation("minecraft:block.coral_block.step"),
    ResourceLocation("minecraft:block.coral_block.place"),
    ResourceLocation("minecraft:block.coral_block.hit"),
    ResourceLocation("minecraft:block.coral_block.fall"),
    1.0f,
    1.0f);

// 硬质作物：破坏与踩踏取木质声，放置取作物种下声
const BlockSoundType HARD_CROP(ResourceLocation("minecraft:block.wood.break"),
    ResourceLocation("minecraft:block.wood.step"),
    ResourceLocation("minecraft:item.crop.plant"),
    ResourceLocation("minecraft:block.wood.hit"),
    ResourceLocation("minecraft:block.wood.fall"),
    1.0f,
    1.0f);

// 睡莲：破坏/踩踏/击打/坠落取大垂滴叶声，放置是自己的
const BlockSoundType LILY_PAD(ResourceLocation("minecraft:block.big_dripleaf.break"),
    ResourceLocation("minecraft:block.big_dripleaf.step"),
    ResourceLocation("minecraft:block.lily_pad.place"),
    ResourceLocation("minecraft:block.big_dripleaf.hit"),
    ResourceLocation("minecraft:block.big_dripleaf.fall"),
    1.0f,
    1.0f);

// 蜘蛛网
const BlockSoundType COBWEB(ResourceLocation("minecraft:block.cobweb.break"),
    ResourceLocation("minecraft:block.cobweb.step"),
    ResourceLocation("minecraft:block.cobweb.place"),
    ResourceLocation("minecraft:block.cobweb.hit"),
    ResourceLocation("minecraft:block.cobweb.fall"),
    1.0f,
    1.0f);

// 刷怪笼
const BlockSoundType SPAWNER(ResourceLocation("minecraft:block.spawner.break"),
    ResourceLocation("minecraft:block.spawner.step"),
    ResourceLocation("minecraft:block.spawner.place"),
    ResourceLocation("minecraft:block.spawner.hit"),
    ResourceLocation("minecraft:block.spawner.fall"),
    1.0f,
    1.0f);

// 铁质方块：与 METAL 不同，音调是 1.0
const BlockSoundType IRON(ResourceLocation("minecraft:block.iron.break"),
    ResourceLocation("minecraft:block.iron.step"),
    ResourceLocation("minecraft:block.iron.place"),
    ResourceLocation("minecraft:block.iron.hit"),
    ResourceLocation("minecraft:block.iron.fall"),
    1.0f,
    1.0f);

// 铜傀儡雕像
const BlockSoundType COPPER_GOLEM_STATUE(ResourceLocation("minecraft:block.copper_golem_statue.break"),
    ResourceLocation("minecraft:block.copper_golem_statue.step"),
    ResourceLocation("minecraft:block.copper_golem_statue.place"),
    ResourceLocation("minecraft:block.copper_golem_statue.hit"),
    ResourceLocation("minecraft:block.copper_golem_statue.fall"),
    1.0f,
    1.0f);

// 落叶堆（1.21.5+）
const BlockSoundType LEAF_LITTER(ResourceLocation("minecraft:block.leaf_litter.break"),
    ResourceLocation("minecraft:block.leaf_litter.step"),
    ResourceLocation("minecraft:block.leaf_litter.place"),
    ResourceLocation("minecraft:block.leaf_litter.hit"),
    ResourceLocation("minecraft:block.leaf_litter.fall"),
    1.0f,
    1.0f);

void initialize()
{
    // 静态初始化已在全局对象构造时完成
    // 此函数保留用于未来可能的动态加载
}

} // namespace BlockSoundTypes

} // namespace mc
