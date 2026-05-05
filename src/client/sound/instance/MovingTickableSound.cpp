#include "client/sound/instance/MovingTickableSound.hpp"
#include "client/sound/SoundEngine.hpp"

namespace mc::client::sound {

MovingTickableSound::MovingTickableSound(
    const ResourceLocation& soundEventId,
    SoundCategory category,
    const EntitySoundHandler* handler,
    EntityId entityId,
    f32 volume,
    f32 pitch
)
    : TickableSound(
        soundEventId,
        category,
        glm::vec3(0.0f),  // 初始位置，将在tick中更新
        volume,
        pitch,
        true,  // 循环
        AttenuationType::Linear,
        DEFAULT_ATTENUATION_DISTANCE
    )
    , m_handler(handler)
    , m_entityId(entityId)
{
    // 设置初始衰减距离为16格
    m_attenuationDistance = 16.0f;
}

void MovingTickableSound::tick() {
    // 检查处理器是否有效
    if (!m_handler) {
        markDone();
        return;
    }

    // 获取实体状态
    const EntitySoundState* state = m_handler->getEntityState(m_entityId);
    if (!state || state->isRemoved) {
        // 实体不存在或已移除，停止声音
        markDone();
        return;
    }

    // 更新声音位置
    setPosition(state->position);
}

} // namespace mc::client::sound
