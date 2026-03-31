#include "client/sound/instance/EntitySoundInstance.hpp"
#include "common/entity/core/Entity.hpp"

namespace mc::client::sound {

EntitySoundInstance::EntitySoundInstance(
    const ResourceLocation& soundEventId,
    SoundCategory category,
    Entity& entity,
    f32 volume,
    f32 pitch
)
    : TickableSound(
        soundEventId,
        category,
        glm::vec3(entity.x(), entity.y(), entity.z()),
        volume,
        pitch,
        false,  // 默认不循环
        AttenuationType::Linear,
        DEFAULT_ATTENUATION_DISTANCE
    )
    , m_entity(entity)
{
    // 初始化位置为实体当前位置
    setPosition(glm::vec3(entity.x(), entity.y(), entity.z()));
}

void EntitySoundInstance::tick() {
    // 检查实体是否仍然有效
    if (!isEntityValid()) {
        markDone();
        return;
    }

    // 更新位置跟随实体
    setPosition(glm::vec3(m_entity.x(), m_entity.y(), m_entity.z()));
}

bool EntitySoundInstance::isEntityValid() const {
    // 实体被移除
    if (m_entity.isRemoved()) {
        return false;
    }

    // 实体静音
    if (m_entity.isSilent()) {
        return false;
    }

    return true;
}

} // namespace mc::client::sound
