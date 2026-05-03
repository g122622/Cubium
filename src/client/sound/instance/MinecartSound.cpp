#include "client/sound/instance/MinecartSound.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <algorithm>

namespace mc::client::sound {

// ============================================================================
// MinecartTickableSound
// ============================================================================

MinecartTickableSound::MinecartTickableSound(Entity& minecart)
    : TickableSound(
          SoundEvents::ENTITY_MINECART_RIDING,
          SoundCategory::Neutral,
          glm::vec3{minecart.x(), minecart.y(), minecart.z()},
          0.0f,   // 初始音量为0
          1.0f,   // 音调
          true,   // 循环
          AttenuationType::Linear,
          16.0f   // 衰减距离
      )
    , m_minecart(minecart)
{
}

void MinecartTickableSound::tick() {
    // 参考: MinecartTickableSound.tick()
    if (m_minecart.isRemoved()) {
        markDone();
        return;
    }

    // 更新位置
    setPosition(glm::vec3(m_minecart.x(), m_minecart.y(), m_minecart.z()));

    // 计算水平速度平方 (MC: Entity.horizontalMag)
    auto vel = m_minecart.velocity();
    f32 horizontalSpeedSq = vel.x * vel.x + vel.z * vel.z;
    f32 horizontalSpeed = std::sqrt(horizontalSpeedSq);

    // 音量计算
    // 参考: MC 源码
    // if ((double)f >= 0.01D) {
    //     this.distance = MathHelper.clamp(this.distance + 0.0025F, 0.0F, 1.0F);
    //     this.volume = MathHelper.lerp(MathHelper.clamp(f, 0.0F, 0.5F), 0.0F, 0.7F);
    // } else {
    //     this.distance = 0.0F;
    //     this.volume = 0.0F;
    // }
    if (horizontalSpeed >= 0.01f) {
        // distance 用于平滑音量变化
        m_distance = std::clamp(m_distance + 0.0025f, 0.0f, 1.0f);
        // 音量基于水平速度线性插值，范围 [0.0, 0.7]
        f32 t = std::clamp(horizontalSpeed, 0.0f, 0.5f) / 0.5f;
        f32 volume = mc::math::lerp(0.0f, 0.7f, t);
        setVolume(volume);
    } else {
        m_distance = 0.0f;
        setVolume(0.0f);
    }
}

// ============================================================================
// RidingMinecartTickableSound
// ============================================================================

RidingMinecartTickableSound::RidingMinecartTickableSound(Entity& player, Entity& minecart)
    : TickableSound(
          SoundEvents::ENTITY_MINECART_INSIDE,
          SoundCategory::Neutral,
          glm::vec3{player.x(), player.y(), player.z()},
          0.0f,   // 初始音量为0
          1.0f,   // 音调
          true,   // 循环
          AttenuationType::None,  // 无衰减（玩家内部声音）
          16.0f   // 衰减距离（无意义）
      )
    , m_player(player)
    , m_minecart(minecart)
{
}

void RidingMinecartTickableSound::tick() {
    // 参考: RidingMinecartTickableSound.tick()
    // 检查矿车是否被移除
    if (m_minecart.isRemoved()) {
        markDone();
        return;
    }

    // TODO: 检查玩家是否仍在骑乘同一辆矿车
    // MC: this.player.isPassenger() && this.player.getRidingEntity() == this.minecart

    // 更新位置（跟随玩家）
    setPosition(glm::vec3(m_player.x(), m_player.y(), m_player.z()));

    // 计算水平速度
    auto vel = m_minecart.velocity();
    f32 horizontalSpeedSq = vel.x * vel.x + vel.z * vel.z;
    f32 horizontalSpeed = std::sqrt(horizontalSpeedSq);

    // 音量计算
    // 参考: MC 源码
    // if ((double)f >= 0.01D) {
    //     this.volume = 0.0F + MathHelper.clamp(f, 0.0F, 1.0F) * 0.75F;
    // } else {
    //     this.volume = 0.0F;
    // }
    if (horizontalSpeed >= 0.01f) {
        f32 volume = std::clamp(horizontalSpeed, 0.0f, 1.0f) * 0.75f;
        setVolume(volume);
    } else {
        setVolume(0.0f);
    }
}

} // namespace mc::client::sound
