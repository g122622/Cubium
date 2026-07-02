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

#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {

/**
 * @brief 方块声音类型
 *
 * 定义方块在不同操作时播放的声音事件。
 * 每个 BlockSoundType 包含破坏、踩踏、放置、击打和坠落声音。
 *
 * 参考: net.minecraft.block.SoundType
 *
 * 使用示例:
 * @code
 * // 获取方块的破坏声音
 * const BlockSoundType& soundType = block.getSoundType();
 * ResourceLocation breakSound = soundType.getBreakSound();
 *
 * // 在服务端触发声音
 * world.playSound(pos, breakSound, SoundCategory::Blocks, soundType.getVolume(), soundType.getPitch());
 * @endcode
 */
class BlockSoundType {
public:
    /**
     * @brief 构造方块声音类型
     *
     * @param breakSound 破坏声音事件ID
     * @param stepSound 踩踏声音事件ID
     * @param placeSound 放置声音事件ID
     * @param hitSound 击打声音事件ID
     * @param fallSound 坠落声音事件ID
     * @param volume 音量倍率
     * @param pitch 音调倍率
     */
    BlockSoundType(const ResourceLocation& breakSound,
        const ResourceLocation& stepSound,
        const ResourceLocation& placeSound,
        const ResourceLocation& hitSound,
        const ResourceLocation& fallSound,
        f32 volume = 1.0f,
        f32 pitch = 1.0f);

    /**
     * @brief 默认构造函数（创建静音类型）
     */
    BlockSoundType() = default;

    // ========================================================================
    // 声音事件访问器
    // ========================================================================

    /**
     * @brief 获取破坏声音事件ID
     *
     * 方块被破坏时播放的声音。
     * 例如：石头破坏声 "minecraft:block.stone.break"
     */
    [[nodiscard]] const ResourceLocation& getBreakSound() const noexcept { return m_breakSound; }

    /**
     * @brief 获取踩踏声音事件ID
     *
     * 玩家在方块上行走时播放的声音。
     * 例如：石头踩踏声 "minecraft:block.stone.step"
     */
    [[nodiscard]] const ResourceLocation& getStepSound() const noexcept { return m_stepSound; }

    /**
     * @brief 获取放置声音事件ID
     *
     * 方块被放置时播放的声音。
     * 例如：石头放置声 "minecraft:block.stone.place"
     */
    [[nodiscard]] const ResourceLocation& getPlaceSound() const noexcept { return m_placeSound; }

    /**
     * @brief 获取击打声音事件ID
     *
     * 玩家左键点击（挖掘）方块时播放的声音。
     * 例如：石头击打声 "minecraft:block.stone.hit"
     */
    [[nodiscard]] const ResourceLocation& getHitSound() const noexcept { return m_hitSound; }

    /**
     * @brief 获取坠落声音事件ID
     *
     * 实体从高处坠落到此方块上时播放的声音。
     * 例如：石头坠落声 "minecraft:block.stone.fall"
     */
    [[nodiscard]] const ResourceLocation& getFallSound() const noexcept { return m_fallSound; }

    // ========================================================================
    // 音量和音调
    // ========================================================================

    /**
     * @brief 获取音量倍率
     *
     * 音量倍率乘以声音事件的默认音量得到实际音量。
     * 例如： explosions.n 的音量为 4.0，使声音更响亮。
     *
     * @return 音量倍率 (默认 1.0)
     */
    [[nodiscard]] f32 getVolume() const noexcept { return m_volume; }

    /**
     * @brief 获取音调倍率
     *
     * 音调倍率乘以声音事件的默认音调得到实际音调。
     * MC 中通常在 pitch * 0.8 到 pitch * 1.2 范围内随机变化。
     *
     * @return 音调倍率 (默认 1.0)
     */
    [[nodiscard]] f32 getPitch() const noexcept { return m_pitch; }

private:
    ResourceLocation m_breakSound;
    ResourceLocation m_stepSound;
    ResourceLocation m_placeSound;
    ResourceLocation m_hitSound;
    ResourceLocation m_fallSound;
    f32 m_volume = 1.0f;
    f32 m_pitch = 1.0f;
};

/**
 * @brief 预定义的方块声音类型
 *
 * 参考: net.minecraft.block.SoundEvents
 */
namespace BlockSoundTypes {
// 木头
extern const BlockSoundType WOOD;

// 石头
extern const BlockSoundType STONE;

// 泥土
extern const BlockSoundType DIRT;

// 草方块
extern const BlockSoundType GRASS;

// 沙子
extern const BlockSoundType SAND;

// 砾石
extern const BlockSoundType GRAVEL;

// 玻璃
extern const BlockSoundType GLASS;

// 金属（铁块等）
extern const BlockSoundType METAL;

// 水
extern const BlockSoundType WATER;

// 岩浆
extern const BlockSoundType LAVA;

// 雪
extern const BlockSoundType SNOW;

// 叶子
extern const BlockSoundType LEAVES;

// 羊毛
extern const BlockSoundType WOOL;

// 地狱岩
extern const BlockSoundType NETHERRACK;

// 灵魂沙
extern const BlockSoundType SOUL_SAND;

// 灵魂土
extern const BlockSoundType SOUL_SOIL;

// 基岩
extern const BlockSoundType BASALT;

// 骨头
extern const BlockSoundType BONE;

// 下界金矿
extern const BlockSoundType NETHER_GOLD_ORE;

// 下界合金块
extern const BlockSoundType NETHERITE;

// 古代遗迹
extern const BlockSoundType ANCIENT_DEBRIS;

// 锚
extern const BlockSoundType RESPAWN_ANCHOR;

// 紫水晶
extern const BlockSoundType AMETHYST;

// 铜块
extern const BlockSoundType COPPER;

// 深板岩
extern const BlockSoundType DEEPSLATE;

// 凝灰岩
extern const BlockSoundType TUFF;

// 浮冰
extern const BlockSoundType PACKED_ICE;

// 冰
extern const BlockSoundType ICE;

// 萤石
extern const BlockSoundType GLOWSTONE;

// 海晶石
extern const BlockSoundType PRISMARINE;

// 海绵
extern const BlockSoundType SPONGE;

// 湿海绵
extern const BlockSoundType WET_SPONGE;

// 干草块
extern const BlockSoundType HAY;

// 地毯
extern const BlockSoundType CLOTH;

// 空气（静音）
extern const BlockSoundType AIR;

// ========== MC 1.16.5 缺失的声音类型 ==========

// 地面（泥土/砾石变体）
extern const BlockSoundType GROUND;

// 植物
extern const BlockSoundType PLANT;

// 梯子
extern const BlockSoundType LADDER;

// 铁砧（volume=0.3）
extern const BlockSoundType ANVIL;

// 黏液块
extern const BlockSoundType SLIME;

// 蜂蜜块
extern const BlockSoundType HONEY;

// 湿草
extern const BlockSoundType WET_GRASS;

// 珊瑚
extern const BlockSoundType CORAL;

// 竹子
extern const BlockSoundType BAMBOO;

// 竹笋
extern const BlockSoundType BAMBOO_SAPLING;

// 脚手架
extern const BlockSoundType SCAFFOLDING;

// 甜浆果丛
extern const BlockSoundType SWEET_BERRY_BUSH;

// 农作物
extern const BlockSoundType CROP;

// 菌柄（下界木质）
extern const BlockSoundType STEM;

// 下界木（绯红/诡异木板、楼梯、台阶、栅栏等）
extern const BlockSoundType NETHER_WOOD;

// 藤蔓
extern const BlockSoundType VINE;

// 地狱疣
extern const BlockSoundType NETHER_WART;

// 灯笼
extern const BlockSoundType LANTERN;

// 菌核（下界木质内部）
extern const BlockSoundType HYPHAE;

// 菌岩
extern const BlockSoundType NYLIUM;

// 真菌
extern const BlockSoundType FUNGUS;

// 菌索
extern const BlockSoundType ROOT;

// 菌光体
extern const BlockSoundType SHROOMLIGHT;

// 下界藤蔓
extern const BlockSoundType NETHER_VINE;

// 下界藤蔓（低音调）
extern const BlockSoundType NETHER_VINE_LOWER_PITCH;

// 疣块
extern const BlockSoundType WART;

// 下界砖
extern const BlockSoundType NETHER_BRICK;

// 下界苗
extern const BlockSoundType NETHER_SPROUT;

// 下界矿石
extern const BlockSoundType NETHER_ORE;

// 磁石
extern const BlockSoundType LODESTONE;

// 锁链
extern const BlockSoundType CHAIN;

// 镶金黑石
extern const BlockSoundType GILDED_BLACKSTONE;

// ========================================================================
// 1.17 洞穴与山崖 Part 1
// ========================================================================

// 深板岩系列
extern const BlockSoundType DEEPSLATE;
extern const BlockSoundType COBBLED_DEEPSLATE;
extern const BlockSoundType POLISHED_DEEPSLATE;
extern const BlockSoundType DEEPSLATE_BRICKS;
extern const BlockSoundType DEEPSLATE_TILES;

// 铜系列
extern const BlockSoundType COPPER;
extern const BlockSoundType COPPER_BULB;
extern const BlockSoundType COPPER_GRATE;

// 紫水晶系列
extern const BlockSoundType AMETHYST;
extern const BlockSoundType AMETHYST_CLUSTER;
extern const BlockSoundType SMALL_AMETHYST_BUD;
extern const BlockSoundType MEDIUM_AMETHYST_BUD;
extern const BlockSoundType LARGE_AMETHYST_BUD;

// 洞穴装饰
extern const BlockSoundType MOSS;
extern const BlockSoundType MOSS_CARPET;
extern const BlockSoundType AZALEA;
extern const BlockSoundType AZALEA_LEAVES;
extern const BlockSoundType CAVE_VINES;
extern const BlockSoundType SPORE_BLOSSOM;
extern const BlockSoundType DRIPSTONE_BLOCK;
extern const BlockSoundType POINTED_DRIPSTONE;
extern const BlockSoundType CALCITE;
extern const BlockSoundType POWDER_SNOW;
extern const BlockSoundType HANGING_ROOTS;
extern const BlockSoundType ROOTED_DIRT;
extern const BlockSoundType BIG_DRIPLEAF;
extern const BlockSoundType SMALL_DRIPLEAF;
extern const BlockSoundType GLOW_LICHEN;
extern const BlockSoundType FLOWERING_AZALEA;

// ========================================================================
// 1.19 荒野更新
// ========================================================================

// 幽匿系列
extern const BlockSoundType SCULK;
extern const BlockSoundType SCULK_CATALYST;
extern const BlockSoundType SCULK_SENSOR;
extern const BlockSoundType SCULK_SHRIEKER;
extern const BlockSoundType SCULK_VEIN;

// 泥巴系列
extern const BlockSoundType MUD;
extern const BlockSoundType MUD_BRICKS;
extern const BlockSoundType PACKED_MUD;
extern const BlockSoundType MUDDY_MANGROVE_ROOTS;

// 红树林
extern const BlockSoundType MANGROVE_ROOTS;
extern const BlockSoundType MANGROVE_WOOD;

// 蛙明灯
extern const BlockSoundType FROGLIGHT;
extern const BlockSoundType FROGSPAWN;

// ========================================================================
// 1.20 足迹与故事
// ========================================================================

// 樱花系列
extern const BlockSoundType CHERRY_WOOD;
extern const BlockSoundType CHERRY_LEAVES;
extern const BlockSoundType CHERRY_SAPLING;

// 竹木系列
extern const BlockSoundType BAMBOO_WOOD;

// 考古
extern const BlockSoundType SUSPICIOUS_SAND;
extern const BlockSoundType SUSPICIOUS_GRAVEL;

// 装饰
extern const BlockSoundType DECORATED_POT;
extern const BlockSoundType CHISELED_BOOKSHELF;

// 植物
extern const BlockSoundType PINK_PETALS;
extern const BlockSoundType TORCHFLOWER;
extern const BlockSoundType PITCHER_CROP;
extern const BlockSoundType SNIFFER_EGG;

// ========================================================================
// 1.21 棘巧试炼
// ========================================================================

// 凝灰岩系列
extern const BlockSoundType TUFF;
extern const BlockSoundType POLISHED_TUFF;
extern const BlockSoundType TUFF_BRICKS;

// 试炼密室
extern const BlockSoundType TRIAL_SPAWNER;
extern const BlockSoundType VAULT;
extern const BlockSoundType CRAFTER;

// ========================================================================
// 1.21.2+ 苍白花园
// ========================================================================

extern const BlockSoundType PALE_MOSS;
extern const BlockSoundType PALE_HANGING_MOSS;
extern const BlockSoundType CREAKING_HEART;
extern const BlockSoundType RESIN;
extern const BlockSoundType RESIN_BRICKS;

// 其他新方块
extern const BlockSoundType HEAVY_CORE;
extern const BlockSoundType EYEBLOSSOM;

// 1.21.4+ 花园觉醒
extern const BlockSoundType CACTUS_FLOWER;

// 书架（1.21.4+ 新增的各种木质书架）
extern const BlockSoundType SHELF;

/**
 * @brief 初始化预定义声音类型
 *
 * 必须在使用预定义声音类型前调用。
 */
void initialize();
} // namespace BlockSoundTypes

} // namespace mc
