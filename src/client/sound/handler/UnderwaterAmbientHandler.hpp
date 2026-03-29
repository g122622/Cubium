#pragma once

#include "client/sound/handler/IAmbientSoundHandler.hpp"
#include "client/sound/SoundEngine.hpp"

#include <random>

namespace mc::client::sound {

/**
 * @brief 水下环境音效处理器
 *
 * 当玩家在水下时播放水下环境音效。
 * 包括水下的回声效果和特定的水下声音。
 *
 * 参考: net.minecraft.client.audio.UnderwaterAmbientSoundHandler
 *
 * 使用示例:
 * @code
 * auto handler = std::make_unique<UnderwaterAmbientHandler>();
 * engine.addAmbientHandler(std::move(handler));
 * @endcode
 *
 * 注意：需要通过 setUnderwater() 方法更新玩家是否在水下。
 */
class UnderwaterAmbientHandler : public IAmbientSoundHandler {
public:
    UnderwaterAmbientHandler();
    ~UnderwaterAmbientHandler() override = default;

    /**
     * @brief 每帧更新
     *
     * 如果玩家在水下，定期播放水下环境音效。
     *
     * @param engine 声音引擎
     */
    void tick(SoundEngine& engine) override;

    /**
     * @brief 设置玩家是否在水下
     *
     * @param underwater 是否在水下
     */
    void setUnderwater(bool underwater) { m_isUnderwater = underwater; }

    /**
     * @brief 检查玩家是否在水下
     */
    [[nodiscard]] bool isUnderwater() const noexcept { return m_isUnderwater; }

private:
    /// 是否在水下
    bool m_isUnderwater = false;

    /// 延迟计时器（ticks）
    u32 m_delayTimer = 0;

    /// 随机数生成器
    std::mt19937 m_rng;
};

} // namespace mc::client::sound
