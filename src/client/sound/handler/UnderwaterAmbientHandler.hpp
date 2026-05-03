#pragma once

#include "client/sound/handler/IAmbientSoundHandler.hpp"
#include "client/sound/SoundEngine.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::client::sound {

/**
 * @brief 水下环境音效处理器
 *
 * 当玩家在水下时播放水下环境音效。
 * 包括主循环音效和三个稀有度级别的附加音效。
 *
 * 参考: net.minecraft.client.audio.UnderwaterAmbientSoundHandler
 *
 * 稀有度概率:
 * - 普通音效: 0.9% 每tick
 * - 稀有音效: 0.09% 每tick
 * - 超稀有音效: 0.01% 每tick
 *
 * 使用示例:
 * @code
 * auto handler = std::make_unique<UnderwaterAmbientHandler>();
 * engine.addAmbientHandler(std::move(handler));
 * @endcode
 *
 * TODO：需要通过 setUnderwater() 方法更新玩家是否在水下。
 */
class UnderwaterAmbientHandler : public IAmbientSoundHandler {
public:
    UnderwaterAmbientHandler();
    ~UnderwaterAmbientHandler() override = default;

    /**
     * @brief 每帧更新
     *
     * 如果玩家在水下，按概率播放水下环境音效。
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

    /// 随机数生成器
    math::Random m_rng{0};
};

} // namespace mc::client::sound
