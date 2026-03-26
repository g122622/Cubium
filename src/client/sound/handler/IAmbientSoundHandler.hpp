#pragma once

namespace mc::client::sound {

class SoundEngine;

/**
 * @brief 环境音效处理器接口
 *
 * 环境音效处理器负责根据游戏状态播放环境音效，
 * 如生物群系背景音、水下音效、气泡柱音效等。
 *
 * 参考: net.minecraft.client.audio.IAmbientSoundHandler
 *
 * 使用示例:
 * @code
 * class BiomeAmbientHandler : public IAmbientSoundHandler {
 * public:
 *     void tick(SoundEngine& engine) override {
 *         // 检查玩家所在群系
 *         auto biome = player.getBiome();
 *         if (biome == BiomeId::Swamp) {
 *             // 播放沼泽环境音
 *         }
 *     }
 * };
 * @endcode
 */
class IAmbientSoundHandler {
public:
    virtual ~IAmbientSoundHandler() = default;

    /**
     * @brief 每帧更新
     *
     * 检查游戏状态并在需要时播放环境音效。
     * 此方法每帧调用一次，应避免昂贵的操作。
     *
     * @param engine 声音引擎
     */
    virtual void tick(SoundEngine& engine) = 0;
};

} // namespace mc::client::sound
