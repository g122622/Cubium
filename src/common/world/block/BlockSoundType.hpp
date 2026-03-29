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
    BlockSoundType(
        const ResourceLocation& breakSound,
        const ResourceLocation& stepSound,
        const ResourceLocation& placeSound,
        const ResourceLocation& hitSound,
        const ResourceLocation& fallSound,
        f32 volume = 1.0f,
        f32 pitch = 1.0f
    );

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

    /**
     * @brief 初始化预定义声音类型
     *
     * 必须在使用预定义声音类型前调用。
     */
    void initialize();
}

} // namespace mc
