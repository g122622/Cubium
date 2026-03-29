#include "BlockSoundType.hpp"

namespace mc {

BlockSoundType::BlockSoundType(
    const ResourceLocation& breakSound,
    const ResourceLocation& stepSound,
    const ResourceLocation& placeSound,
    const ResourceLocation& hitSound,
    const ResourceLocation& fallSound,
    f32 volume,
    f32 pitch
)
    : m_breakSound(breakSound)
    , m_stepSound(stepSound)
    , m_placeSound(placeSound)
    , m_hitSound(hitSound)
    , m_fallSound(fallSound)
    , m_volume(volume)
    , m_pitch(pitch)
{
}

// ============================================================================
// 预定义声音类型
// ============================================================================

namespace BlockSoundTypes {

// 木头 - 木头碰撞声
const BlockSoundType WOOD(
    ResourceLocation("minecraft:block.wood.break"),
    ResourceLocation("minecraft:block.wood.step"),
    ResourceLocation("minecraft:block.wood.place"),
    ResourceLocation("minecraft:block.wood.hit"),
    ResourceLocation("minecraft:block.wood.fall"),
    1.0f, 1.0f
);

// 石头 - 石头碰撞声
const BlockSoundType STONE(
    ResourceLocation("minecraft:block.stone.break"),
    ResourceLocation("minecraft:block.stone.step"),
    ResourceLocation("minecraft:block.stone.place"),
    ResourceLocation("minecraft:block.stone.hit"),
    ResourceLocation("minecraft:block.stone.fall"),
    1.0f, 1.0f
);

// 泥土 - 柔软的泥土声
const BlockSoundType DIRT(
    ResourceLocation("minecraft:block.dirt.break"),
    ResourceLocation("minecraft:block.dirt.step"),
    ResourceLocation("minecraft:block.dirt.place"),
    ResourceLocation("minecraft:block.dirt.hit"),
    ResourceLocation("minecraft:block.dirt.fall"),
    1.0f, 1.0f
);

// 草方块 - 草地声
const BlockSoundType GRASS(
    ResourceLocation("minecraft:block.grass.break"),
    ResourceLocation("minecraft:block.grass.step"),
    ResourceLocation("minecraft:block.grass.place"),
    ResourceLocation("minecraft:block.grass.hit"),
    ResourceLocation("minecraft:block.grass.fall"),
    1.0f, 1.0f
);

// 沙子 - 沙沙声
const BlockSoundType SAND(
    ResourceLocation("minecraft:block.sand.break"),
    ResourceLocation("minecraft:block.sand.step"),
    ResourceLocation("minecraft:block.sand.place"),
    ResourceLocation("minecraft:block.sand.hit"),
    ResourceLocation("minecraft:block.sand.fall"),
    1.0f, 1.0f
);

// 砾石 - 碎石声
const BlockSoundType GRAVEL(
    ResourceLocation("minecraft:block.gravel.break"),
    ResourceLocation("minecraft:block.gravel.step"),
    ResourceLocation("minecraft:block.gravel.place"),
    ResourceLocation("minecraft:block.gravel.hit"),
    ResourceLocation("minecraft:block.gravel.fall"),
    1.0f, 1.0f
);

// 玻璃 - 清脆的玻璃声
const BlockSoundType GLASS(
    ResourceLocation("minecraft:block.glass.break"),
    ResourceLocation("minecraft:block.glass.step"),
    ResourceLocation("minecraft:block.glass.place"),
    ResourceLocation("minecraft:block.glass.hit"),
    ResourceLocation("minecraft:block.glass.fall"),
    1.0f, 1.0f
);

// 金属 - 金属碰撞声
const BlockSoundType METAL(
    ResourceLocation("minecraft:block.metal.break"),
    ResourceLocation("minecraft:block.metal.step"),
    ResourceLocation("minecraft:block.metal.place"),
    ResourceLocation("minecraft:block.metal.hit"),
    ResourceLocation("minecraft:block.metal.fall"),
    1.0f, 1.0f
);

// 水 - 水声
const BlockSoundType WATER(
    ResourceLocation("minecraft:block.water.ambient"),
    ResourceLocation("minecraft:block.water.step"),
    ResourceLocation("minecraft:block.water.place"),
    ResourceLocation("minecraft:block.water.hit"),
    ResourceLocation("minecraft:block.water.fall"),
    1.0f, 1.0f
);

// 岩浆 - 岩浆声
const BlockSoundType LAVA(
    ResourceLocation("minecraft:block.lava.ambient"),
    ResourceLocation("minecraft:block.lava.step"),
    ResourceLocation("minecraft:block.lava.place"),
    ResourceLocation("minecraft:block.lava.hit"),
    ResourceLocation("minecraft:block.lava.fall"),
    1.0f, 1.0f
);

// 雪 - 柔软的雪声
const BlockSoundType SNOW(
    ResourceLocation("minecraft:block.snow.break"),
    ResourceLocation("minecraft:block.snow.step"),
    ResourceLocation("minecraft:block.snow.place"),
    ResourceLocation("minecraft:block.snow.hit"),
    ResourceLocation("minecraft:block.snow.fall"),
    1.0f, 1.0f
);

// 叶子 - 叶子沙沙声
const BlockSoundType LEAVES(
    ResourceLocation("minecraft:block.grass.break"),
    ResourceLocation("minecraft:block.grass.step"),
    ResourceLocation("minecraft:block.grass.place"),
    ResourceLocation("minecraft:block.grass.hit"),
    ResourceLocation("minecraft:block.grass.fall"),
    1.0f, 1.0f
);

// 羊毛 - 柔软的羊毛声
const BlockSoundType WOOL(
    ResourceLocation("minecraft:block.wool.break"),
    ResourceLocation("minecraft:block.wool.step"),
    ResourceLocation("minecraft:block.wool.place"),
    ResourceLocation("minecraft:block.wool.hit"),
    ResourceLocation("minecraft:block.wool.fall"),
    1.0f, 1.0f
);

// 地狱岩 - 地狱岩声
const BlockSoundType NETHERRACK(
    ResourceLocation("minecraft:block.netherrack.break"),
    ResourceLocation("minecraft:block.netherrack.step"),
    ResourceLocation("minecraft:block.netherrack.place"),
    ResourceLocation("minecraft:block.netherrack.hit"),
    ResourceLocation("minecraft:block.netherrack.fall"),
    1.0f, 1.0f
);

// 灵魂沙 - 灵魂沙声
const BlockSoundType SOUL_SAND(
    ResourceLocation("minecraft:block.soul_sand.break"),
    ResourceLocation("minecraft:block.soul_sand.step"),
    ResourceLocation("minecraft:block.soul_sand.place"),
    ResourceLocation("minecraft:block.soul_sand.hit"),
    ResourceLocation("minecraft:block.soul_sand.fall"),
    1.0f, 1.0f
);

// 灵魂土 - 灵魂土声
const BlockSoundType SOUL_SOIL(
    ResourceLocation("minecraft:block.soul_soil.break"),
    ResourceLocation("minecraft:block.soul_soil.step"),
    ResourceLocation("minecraft:block.soul_soil.place"),
    ResourceLocation("minecraft:block.soul_soil.hit"),
    ResourceLocation("minecraft:block.soul_soil.fall"),
    1.0f, 1.0f
);

// 基岩 - 玄武岩声
const BlockSoundType BASALT(
    ResourceLocation("minecraft:block.basalt.break"),
    ResourceLocation("minecraft:block.basalt.step"),
    ResourceLocation("minecraft:block.basalt.place"),
    ResourceLocation("minecraft:block.basalt.hit"),
    ResourceLocation("minecraft:block.basalt.fall"),
    1.0f, 1.0f
);

// 骨头 - 骨块声
const BlockSoundType BONE(
    ResourceLocation("minecraft:block.bone_block.break"),
    ResourceLocation("minecraft:block.bone_block.step"),
    ResourceLocation("minecraft:block.bone_block.place"),
    ResourceLocation("minecraft:block.bone_block.hit"),
    ResourceLocation("minecraft:block.bone_block.fall"),
    1.0f, 1.0f
);

// 下界金矿 - 下界金矿声
const BlockSoundType NETHER_GOLD_ORE(
    ResourceLocation("minecraft:block.nether_gold_ore.break"),
    ResourceLocation("minecraft:block.nether_gold_ore.step"),
    ResourceLocation("minecraft:block.nether_gold_ore.place"),
    ResourceLocation("minecraft:block.nether_gold_ore.hit"),
    ResourceLocation("minecraft:block.nether_gold_ore.fall"),
    1.0f, 1.0f
);

// 下界合金块 - 下界合金声
const BlockSoundType NETHERITE(
    ResourceLocation("minecraft:block.netherite_block.break"),
    ResourceLocation("minecraft:block.netherite_block.step"),
    ResourceLocation("minecraft:block.netherite_block.place"),
    ResourceLocation("minecraft:block.netherite_block.hit"),
    ResourceLocation("minecraft:block.netherite_block.fall"),
    1.0f, 1.0f
);

// 古代遗迹 - 古代遗迹声
const BlockSoundType ANCIENT_DEBRIS(
    ResourceLocation("minecraft:block.ancient_debris.break"),
    ResourceLocation("minecraft:block.ancient_debris.step"),
    ResourceLocation("minecraft:block.ancient_debris.place"),
    ResourceLocation("minecraft:block.ancient_debris.hit"),
    ResourceLocation("minecraft:block.ancient_debris.fall"),
    1.0f, 1.0f
);

// 锚 - 重生锚声
const BlockSoundType RESPAWN_ANCHOR(
    ResourceLocation("minecraft:block.respawn_anchor.break"),
    ResourceLocation("minecraft:block.respawn_anchor.step"),
    ResourceLocation("minecraft:block.respawn_anchor.place"),
    ResourceLocation("minecraft:block.respawn_anchor.hit"),
    ResourceLocation("minecraft:block.respawn_anchor.fall"),
    1.0f, 1.0f
);

// 紫水晶 - 紫水晶声
const BlockSoundType AMETHYST(
    ResourceLocation("minecraft:block.amethyst_block.break"),
    ResourceLocation("minecraft:block.amethyst_block.step"),
    ResourceLocation("minecraft:block.amethyst_block.place"),
    ResourceLocation("minecraft:block.amethyst_block.hit"),
    ResourceLocation("minecraft:block.amethyst_block.fall"),
    1.0f, 1.0f
);

// 铜块 - 铜块声
const BlockSoundType COPPER(
    ResourceLocation("minecraft:block.copper.break"),
    ResourceLocation("minecraft:block.copper.step"),
    ResourceLocation("minecraft:block.copper.place"),
    ResourceLocation("minecraft:block.copper.hit"),
    ResourceLocation("minecraft:block.copper.fall"),
    1.0f, 1.0f
);

// 深板岩 - 深板岩声
const BlockSoundType DEEPSLATE(
    ResourceLocation("minecraft:block.deepslate.break"),
    ResourceLocation("minecraft:block.deepslate.step"),
    ResourceLocation("minecraft:block.deepslate.place"),
    ResourceLocation("minecraft:block.deepslate.hit"),
    ResourceLocation("minecraft:block.deepslate.fall"),
    1.0f, 1.0f
);

// 凝灰岩 - 凝灰岩声
const BlockSoundType TUFF(
    ResourceLocation("minecraft:block.tuff.break"),
    ResourceLocation("minecraft:block.tuff.step"),
    ResourceLocation("minecraft:block.tuff.place"),
    ResourceLocation("minecraft:block.tuff.hit"),
    ResourceLocation("minecraft:block.tuff.fall"),
    1.0f, 1.0f
);

// 浮冰 - 冰声
const BlockSoundType PACKED_ICE(
    ResourceLocation("minecraft:block.packed_ice.break"),
    ResourceLocation("minecraft:block.packed_ice.step"),
    ResourceLocation("minecraft:block.packed_ice.place"),
    ResourceLocation("minecraft:block.packed_ice.hit"),
    ResourceLocation("minecraft:block.packed_ice.fall"),
    1.0f, 1.0f
);

// 冰 - 冰声
const BlockSoundType ICE(
    ResourceLocation("minecraft:block.ice.break"),
    ResourceLocation("minecraft:block.ice.step"),
    ResourceLocation("minecraft:block.ice.place"),
    ResourceLocation("minecraft:block.ice.hit"),
    ResourceLocation("minecraft:block.ice.fall"),
    1.0f, 1.0f
);

// 萤石 - 萤石声
const BlockSoundType GLOWSTONE(
    ResourceLocation("minecraft:block.glowstone.break"),
    ResourceLocation("minecraft:block.glowstone.step"),
    ResourceLocation("minecraft:block.glowstone.place"),
    ResourceLocation("minecraft:block.glowstone.hit"),
    ResourceLocation("minecraft:block.glowstone.fall"),
    1.0f, 1.0f
);

// 海晶石 - 海晶石声
const BlockSoundType PRISMARINE(
    ResourceLocation("minecraft:block.prismarine.break"),
    ResourceLocation("minecraft:block.prismarine.step"),
    ResourceLocation("minecraft:block.prismarine.place"),
    ResourceLocation("minecraft:block.prismarine.hit"),
    ResourceLocation("minecraft:block.prismarine.fall"),
    1.0f, 1.0f
);

// 海绵 - 海绵声
const BlockSoundType SPONGE(
    ResourceLocation("minecraft:block.sponge.break"),
    ResourceLocation("minecraft:block.sponge.step"),
    ResourceLocation("minecraft:block.sponge.place"),
    ResourceLocation("minecraft:block.sponge.hit"),
    ResourceLocation("minecraft:block.sponge.fall"),
    1.0f, 1.0f
);

// 湿海绵 - 湿海绵声
const BlockSoundType WET_SPONGE(
    ResourceLocation("minecraft:block.wet_sponge.break"),
    ResourceLocation("minecraft:block.wet_sponge.step"),
    ResourceLocation("minecraft:block.wet_sponge.place"),
    ResourceLocation("minecraft:block.wet_sponge.hit"),
    ResourceLocation("minecraft:block.wet_sponge.fall"),
    1.0f, 1.0f
);

// 干草块 - 干草声
const BlockSoundType HAY(
    ResourceLocation("minecraft:block.hay_block.break"),
    ResourceLocation("minecraft:block.hay_block.step"),
    ResourceLocation("minecraft:block.hay_block.place"),
    ResourceLocation("minecraft:block.hay_block.hit"),
    ResourceLocation("minecraft:block.hay_block.fall"),
    1.0f, 1.0f
);

// 地毯 - 布料声
const BlockSoundType CLOTH(
    ResourceLocation("minecraft:block.wool.break"),
    ResourceLocation("minecraft:block.wool.step"),
    ResourceLocation("minecraft:block.wool.place"),
    ResourceLocation("minecraft:block.wool.hit"),
    ResourceLocation("minecraft:block.wool.fall"),
    1.0f, 1.0f
);

// 空气 - 静音
const BlockSoundType AIR(
    ResourceLocation("minecraft:block.air.ambient"),  // 不存在的声音，用于静音
    ResourceLocation("minecraft:block.air.ambient"),
    ResourceLocation("minecraft:block.air.ambient"),
    ResourceLocation("minecraft:block.air.ambient"),
    ResourceLocation("minecraft:block.air.ambient"),
    0.0f, 1.0f  // 音量为0，静音
);

void initialize() {
    // 静态初始化已在全局对象构造时完成
    // 此函数保留用于未来可能的动态加载
}

} // namespace BlockSoundTypes

} // namespace mc
