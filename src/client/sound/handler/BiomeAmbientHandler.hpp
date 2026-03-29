#pragma once

#include "client/sound/handler/IAmbientSoundHandler.hpp"
#include "client/sound/SoundEngine.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <random>

namespace mc::client::sound {

/**
 * @brief 生物群系环境音效处理器
 *
 * 根据玩家所在的生物群系播放背景环境音效。
 * 不同的群系有不同的环境声音（如沼泽的青蛙声、森林的鸟鸣等）。
 *
 * 参考: net.minecraft.client.audio.BiomeAmbientSoundHandler
 *
 * 使用示例:
 * @code
 * auto handler = std::make_unique<BiomeAmbientHandler>();
 * engine.addAmbientHandler(std::move(handler));
 * @endcode
 *
 * 注意：需要通过 setBiomeId() 方法更新当前群系ID。
 */
class BiomeAmbientHandler : public IAmbientSoundHandler {
public:
    BiomeAmbientHandler();
    ~BiomeAmbientHandler() override = default;

    /**
     * @brief 每帧更新
     *
     * 根据当前群系和延迟计时器播放环境音效。
     *
     * @param engine 声音引擎
     */
    void tick(SoundEngine& engine) override;

    /**
     * @brief 设置当前群系ID
     *
     * 每帧由客户端调用以更新玩家所在的群系。
     *
     * @param biomeId 群系ID
     */
    void setBiomeId(u32 biomeId) { m_currentBiomeId = biomeId; }

    /**
     * @brief 获取当前群系ID
     */
    [[nodiscard]] u32 getBiomeId() const noexcept { return m_currentBiomeId; }

private:
    /**
     * @brief 获取群系的环境音效
     *
     * @param biomeId 群系ID
     * @return 环境音效资源位置，无则返回空
     */
    [[nodiscard]] static Optional<ResourceLocation> getBiomeAmbientSound(u32 biomeId);

    /// 当前群系ID
    u32 m_currentBiomeId = 0;

    /// 延迟计时器（ticks）
    u32 m_delayTimer = 0;

    /// 上次播放的群系ID
    u32 m_lastPlayedBiomeId = 0;

    /// 随机数生成器
    std::mt19937 m_rng;
};

} // namespace mc::client::sound
