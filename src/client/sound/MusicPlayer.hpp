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

#include "client/sound/instance/ISoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeAmbientSounds.hpp"
#include "common/world/dimension/DimensionManager.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace mc {

namespace client {
namespace sound {
class SoundEngine;
}

// 前向声明
class ClientSettings;
class ClientWorld;
} // namespace client

namespace client::sound {

/**
 * @brief 音乐播放器
 *
 * 负责管理背景音乐播放：
 * - 随机选择音乐曲目
 * - 音乐淡入淡出
 * - 播放间隔控制
 * - 根据游戏状态选择音乐（主菜单、游戏、创造模式等）
 * - 根据生物群系选择音乐（下界各生物群系有专属音乐）
 */
class MusicPlayer {
public:
    /**
     * @brief 音乐类型
     */
    enum class MusicType : u8 {
        None,       // 无音乐
        Menu,       // 主菜单音乐
        Game,       // 游戏音乐
        Creative,   // 创造模式音乐
        Credits,    // 制作人员名单音乐
        End,        // 末地音乐
        Nether,     // 下界音乐（已弃用，使用生物群系音乐）
        Underwater, // 水下音乐
        Dragon,     // 末影龙战斗音乐
        Biome       // 生物群系专属音乐
    };

    /**
     * @brief 音乐选择器
     *
     * 定义特定音乐类型的选择规则。
     */
    struct MusicSelector {
        ResourceLocation soundEventId; // 声音事件ID
        u32 minDelayTicks = 12000;     // 最小延迟（ticks，600秒 = 10分钟）
        u32 maxDelayTicks = 24000;     // 最大延迟（ticks，1200秒 = 20分钟）
        bool replaceCurrent = false;   // 是否替换当前音乐

        MusicSelector() = default;
        MusicSelector(const ResourceLocation& id, u32 minDelay, u32 maxDelay, bool replace)
            : soundEventId(id)
            , minDelayTicks(minDelay)
            , maxDelayTicks(maxDelay)
            , replaceCurrent(replace)
        {}

        /**
         * @brief 从生物群系音乐配置创建选择器
         */
        static MusicSelector fromBiomeMusic(const world::biome::BiomeMusic& biomeMusic)
        {
            return MusicSelector(biomeMusic.soundEvent(),
                biomeMusic.minDelayTicks(),
                biomeMusic.maxDelayTicks(),
                biomeMusic.replaceCurrent());
        }
    };

    /**
     * @brief 构造音乐播放器
     *
     * @param engine 声音引擎
     */
    explicit MusicPlayer(SoundEngine& engine);

    /**
     * @brief 析构函数
     */
    ~MusicPlayer();

    // 禁止拷贝
    MusicPlayer(const MusicPlayer&) = delete;
    MusicPlayer& operator=(const MusicPlayer&) = delete;

    // ========================================================================
    // 生命周期
    // ========================================================================

    /**
     * @brief 每帧更新
     *
     * @param isPaused 游戏是否暂停
     * @param inMenu 是否在主菜单
     * @param dimension 当前维度ID
     * @param inWater 是否在水中
     * @param inCreative 是否在创造模式
     * @param inBossFight 是否在Boss战斗中
     * @param biomeMusic 当前生物群系的音乐配置（可选）
     */
    void tick(bool isPaused,
        bool inMenu,
        DimensionId dimension,
        bool inWater,
        bool inCreative,
        bool inBossFight,
        const std::optional<world::biome::BiomeMusic>& biomeMusic);

    /**
     * @brief 停止当前音乐
     *
     * @param fadeOut 是否淡出
     */
    void stop(bool fadeOut);

    // ========================================================================
    // 音乐控制
    // ========================================================================

    /**
     * @brief 设置音乐类型
     *
     * @param type 音乐类型
     */
    void setMusicType(MusicType type);

    /**
     * @brief 获取当前音乐类型
     */
    [[nodiscard]] MusicType musicType() const noexcept { return m_currentType; }

    /**
     * @brief 检查是否正在播放音乐
     */
    [[nodiscard]] bool isPlaying() const noexcept;

    /**
     * @brief 获取当前播放的音乐ID
     */
    [[nodiscard]] SoundInstanceId currentSoundId() const noexcept { return m_currentSoundId; }

    /**
     * @brief 启用/禁用音乐
     *
     * @param enabled 是否启用
     */
    void setEnabled(bool enabled) noexcept { m_enabled = enabled; }

    /**
     * @brief 检查音乐是否启用
     */
    [[nodiscard]] bool isEnabled() const noexcept { return m_enabled; }

private:
    /**
     * @brief 获取指定类型的音乐选择器
     *
     * @param type 音乐类型
     * @return 音乐选择器
     */
    [[nodiscard]] const MusicSelector& _getSelector(MusicType type) const;

    /**
     * @brief 选择下一个延迟时间
     *
     * @param selector 音乐选择器
     * @return 延迟ticks
     */
    [[nodiscard]] u32 _selectDelay(const MusicSelector& selector);

    /**
     * @brief 开始播放音乐
     *
     * @param selector 音乐选择器
     */
    void _startPlaying(const MusicSelector& selector);

    /**
     * @brief 更新淡入淡出
     */
    void _updateFade();

private:
    SoundEngine& m_engine;

    /// 当前音乐类型
    MusicType m_currentType = MusicType::None;

    /// 当前播放的音乐ID
    SoundInstanceId m_currentSoundId = 0;

    /// 下次播放延迟计数器
    u32 m_delayCounter = 0;

    /// 音乐是否启用
    bool m_enabled = true;

    /// 是否正在淡出
    bool m_fadingOut = false;

    /// 淡出计数器
    u32 m_fadeCounter = 0;

    /// 淡出持续时间（ticks）
    static constexpr u32 FADE_DURATION = 40; // 2秒 @ 20 TPS

    /// 随机数生成器
    mutable math::Random m_rng{0};

    // ========================================================================
    // 音乐选择器定义
    // ========================================================================

    /// 游戏音乐选择器
    std::vector<MusicSelector> m_gameMusicSelectors;

    /// 创造模式音乐选择器
    std::vector<MusicSelector> m_creativeMusicSelectors;

    /// 下界音乐选择器
    std::vector<MusicSelector> m_netherMusicSelectors;

    /// 末地音乐选择器
    std::vector<MusicSelector> m_endMusicSelectors;

    /// 水下音乐选择器
    std::vector<MusicSelector> m_underwaterMusicSelectors;
};

} // namespace client::sound
} // namespace mc
