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

#include "client/sound/SoundEngine.hpp"
#include "client/sound/handler/IAmbientSoundHandler.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeAmbientSounds.hpp"

#include <optional>
#include <unordered_map>

namespace mc::client::sound {

// 前向声明
class BiomeLoopSound;

/**
 * @brief 生物群系环境音效处理器
 *
 * 根据玩家所在的生物群系播放三种类型的环境音效：
 * 1. 循环音效 (Loop Sound) - 持续播放的背景音效，群系切换时淡入淡出
 * 2. 心境音效 (Mood Sound) - 在黑暗环境中根据光照等级触发
 * 3. 附加音效 (Additions Sound) - 按概率随机播放的补充音效
 *
 * 参考: net.minecraft.client.audio.BiomeSoundHandler
 *
 * 使用示例:
 * @code
 * auto handler = std::make_unique<BiomeAmbientHandler>();
 * engine.addAmbientHandler(std::move(handler));
 * // 在游戏循环中调用:
 * handler->setBiomeId(biomeId);
 * handler->setPlayerPosition(x, y, z);
 * handler->setLightLevel(skyLight, blockLight);
 * @endcode
 */
class BiomeAmbientHandler : public IAmbientSoundHandler {
public:
    BiomeAmbientHandler();
    ~BiomeAmbientHandler() override;

    /**
     * @brief 每帧更新
     *
     * 根据当前群系和光照条件播放环境音效。
     *
     * @param engine 声音引擎
     */
    void tick(SoundEngine& engine) override;

    /**
     * @brief 停止所有环境音效
     */
    void stopAll();

    /**
     * @brief 设置当前群系ID
     *
     * @param biomeId 群系ID
     */
    void setBiomeId(u32 biomeId) { m_currentBiomeId = biomeId; }

    /**
     * @brief 获取当前群系ID
     */
    [[nodiscard]] u32 getBiomeId() const noexcept { return m_currentBiomeId; }

    /**
     * @brief 设置玩家位置（用于心境音效位置计算）
     *
     * @param x 玩家X坐标
     * @param y 玩家Y坐标（眼睛高度）
     * @param z 玩家Z坐标
     */
    void setPlayerPosition(f64 x, f64 y, f64 z)
    {
        m_playerX = x;
        m_playerY = y;
        m_playerZ = z;
    }

    /**
     * @brief 设置光照等级（用于心境音效触发）
     *
     * @param skyLight 天空光照等级 (0-15)
     * @param blockLight 方块光照等级 (0-15)
     */
    void setLightLevel(u8 skyLight, u8 blockLight)
    {
        m_skyLight = skyLight;
        m_blockLight = blockLight;
    }

private:
    /// 当前群系ID
    u32 m_currentBiomeId = 0;

    /// 上一个群系ID（用于检测群系切换）
    u32 m_previousBiomeId = static_cast<u32>(-1);

    /// 玩家位置
    f64 m_playerX = 0.0;
    f64 m_playerY = 0.0;
    f64 m_playerZ = 0.0;

    /// 光照等级
    u8 m_skyLight = 15;
    u8 m_blockLight = 15;

    /// 心境音效计时器 (0.0 - 1.0)
    f32 m_moodTimer = 0.0f;

    /// 当前心境音效配置
    std::optional<world::biome::MoodSoundAmbience> m_currentMoodSound;

    /// 当前附加音效配置
    std::optional<world::biome::SoundAdditionsAmbience> m_currentAdditionsSound;

    /// 活动的循环音效（按群系ID索引）
    std::unordered_map<u32, SoundInstanceId> m_loopSounds;

    /// 随机数生成器
    math::Random m_rng;
};

} // namespace mc::client::sound
