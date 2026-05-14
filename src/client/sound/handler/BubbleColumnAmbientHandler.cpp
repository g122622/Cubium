#include "client/sound/handler/BubbleColumnAmbientHandler.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"

#include <chrono>

namespace mc::client::sound {

BubbleColumnAmbientHandler::BubbleColumnAmbientHandler()
    : m_rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()))
{}

void BubbleColumnAmbientHandler::tick(SoundEngine& engine)
{
    // 参考 MC 1.16.5: BubbleColumnAmbientSoundHandler.tick()
    // 检测玩家是否刚刚进入气泡柱
    if (m_isInBubbleColumn && !m_wasInBubbleColumn && !m_firstTick) {
        // 刚进入气泡柱，播放音效
        if (m_isDrag) {
            // 向下气泡柱（whirlpool）
            auto sound = std::make_unique<SoundInstance>(SoundInstance::createGlobal(
                SoundEvents::BLOCK_BUBBLE_COLUMN_WHIRLPOOL_INSIDE, SoundCategory::Blocks, 1.0f, 1.0f));
            engine.play(std::move(sound));
        } else {
            // 向上气泡柱
            auto sound = std::make_unique<SoundInstance>(SoundInstance::createGlobal(
                SoundEvents::BLOCK_BUBBLE_COLUMN_UPWARDS_INSIDE, SoundCategory::Blocks, 1.0f, 1.0f));
            engine.play(std::move(sound));
        }
    }

    // 更新状态
    m_wasInBubbleColumn = m_isInBubbleColumn;
    m_firstTick = false;
}

} // namespace mc::client::sound
