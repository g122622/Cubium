#include "GuardianSound.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/sound/SoundEvents.hpp"
#include <cmath>
#include <glm/glm.hpp>

namespace mc::client::sound {

GuardianSound::GuardianSound(const ClientEntity& guardian)
    : TickableSound(
          SoundEvents::ENTITY_GUARDIAN_ATTACK,
          SoundCategory::Hostile,
          glm::vec3(guardian.x(), guardian.y(), guardian.z()),
          0.0f,   // 初始音量为0
          0.7f,   // 音调
          true,   // 循环
          AttenuationType::None,  // 无衰减（全局声音）
          16.0f   // 衰减距离
      )
    , m_guardian(guardian)
{
    // MC 1.16.5: GuardianSound 使用无衰减
    // 守卫者攻击声音是全局可听的
}

void GuardianSound::tick() {
    // MC 1.16.5: 当守卫者未被移除且有攻击目标时播放
    // TODO: 需要从实体元数据获取攻击目标和攻击动画状态
    // 当前使用简化实现

    if (!m_guardian.isRemoved()) {
        // 更新位置
        setPosition(glm::vec3(m_guardian.x(), m_guardian.y(), m_guardian.z()));

        // TODO: 从实体元数据获取攻击动画进度
        // MC 1.16.5: volume = 0.0F + 1.0F * f * f, pitch = 0.7F + 0.5F * f
        // 其中 f = getAttackAnimationScale(0.0F)
        // 当前使用固定值作为占位
        f32 attackAnimScale = 0.5f;  // 占位值

        f32 volume = 1.0f * attackAnimScale * attackAnimScale;
        f32 pitch = 0.7f + 0.5f * attackAnimScale;

        setVolume(volume);
        setPitch(pitch);
    } else {
        // 守卫者被移除，停止播放
        markDone();
    }
}

} // namespace mc::client::sound
