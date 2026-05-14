#include "BeeSound.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <glm/glm.hpp>

namespace mc::client::sound {

// ============================================================================
// BeeSound 实现
// ============================================================================

BeeSound::BeeSound(const ClientEntity& bee, const ResourceLocation& soundEventId)
    : TickableSound(soundEventId,
          SoundCategory::Neutral,
          glm::vec3(bee.x(), bee.y(), bee.z()),
          0.0f,
          0.0f,
          true,
          AttenuationType::Linear,
          16.0f)
    , m_bee(bee)
{
    // 设置初始位置
    setPosition(glm::vec3(bee.x(), bee.y(), bee.z()));
    setLooping(true);
    setVolume(0.0f);
}

void BeeSound::tick()
{
    // 检查是否需要切换声音
    bool shouldSwitch = shouldSwitchSound();

    if (shouldSwitch && !isDone()) {
        // 使用 SoundEngine 播放下一个声音
        // 注意：这里需要通过 SoundEngine 的 playOnNextTick 方法
        // 由于 TickableSound 可能没有直接访问 SoundEngine 的权限，
        // 我们将切换逻辑放到外部处理
        m_hasSwitchedSound = true;
        markDone();
    }

    // 检查蜜蜂是否仍然有效
    if (m_bee.isRemoved()) {
        markDone();
        return;
    }

    // 如果已经切换声音，停止更新
    if (m_hasSwitchedSound) {
        markDone();
        return;
    }

    // 更新位置跟随蜜蜂
    setPosition(glm::vec3(m_bee.x(), m_bee.y(), m_bee.z()));

    // 根据水平速度计算音量和音调
    // MC 1.16.5: 使用 horizontalMag (水平速度平方)
    auto vel = m_bee.velocity();
    f32 horizontalSpeed = std::sqrt(vel.x * vel.x + vel.z * vel.z);

    if (horizontalSpeed >= 0.01f) {
        // 根据速度插值音调
        // MC 1.16.5: pitch = lerp(clampedSpeed, minPitch, maxPitch)
        // 其中 lerp(pct, start, end) = start + pct * (end - start)
        // 我们的 lerp(a, b, t) = a + (b - a) * t，所以参数顺序是 (start, end, factor)
        f32 minPitch = getMinPitch();
        f32 maxPitch = getMaxPitch();
        f32 clampedSpeed = std::clamp(horizontalSpeed, minPitch, maxPitch);
        f32 pitch = math::lerp(minPitch, maxPitch, clampedSpeed);
        setPitch(pitch);

        // 根据速度插值音量
        // MC 1.16.5: volume = lerp(clampedVol, 0.0F, 1.2F)
        f32 clampedVol = std::clamp(horizontalSpeed, 0.0f, 0.5f);
        f32 volume = math::lerp(0.0f, 1.2f, clampedVol);
        setVolume(volume);
    } else {
        // 速度太低，静音
        setPitch(0.0f);
        setVolume(0.0f);
    }
}

f32 BeeSound::getMinPitch() const
{
    // MC 1.16.5: 幼年蜜蜂音调更高
    return m_bee.isChild() ? 1.1f : 0.7f;
}

f32 BeeSound::getMaxPitch() const
{
    // MC 1.16.5: 幼年蜜蜂音调更高
    return m_bee.isChild() ? 1.5f : 1.1f;
}

// ============================================================================
// BeeFlightSound 实现
// ============================================================================

BeeFlightSound::BeeFlightSound(const ClientEntity& bee)
    : BeeSound(bee, SoundEvents::ENTITY_BEE_LOOP)
{}

std::unique_ptr<TickableSound> BeeFlightSound::getNextSound()
{
    // 切换到愤怒声音
    return std::unique_ptr<TickableSound>(new BeeAngrySound(bee()));
}

bool BeeFlightSound::shouldSwitchSound()
{
    // MC 1.16.5: 当蜜蜂愤怒时切换到愤怒声音
    // 愤怒状态从 ClientEntity 的元数据参数读取（ANGER_TIME > 0）
    return bee().isAngry();
}

// ============================================================================
// BeeAngrySound 实现
// ============================================================================

BeeAngrySound::BeeAngrySound(const ClientEntity& bee)
    : BeeSound(bee, SoundEvents::ENTITY_BEE_LOOP_AGGRESSIVE)
{}

std::unique_ptr<TickableSound> BeeAngrySound::getNextSound()
{
    // 切换回飞行声音
    return std::unique_ptr<TickableSound>(new BeeFlightSound(bee()));
}

bool BeeAngrySound::shouldSwitchSound()
{
    // MC 1.16.5: 当蜜蜂不再愤怒时切换回飞行声音
    // 愤怒状态从 ClientEntity 的元数据参数读取（ANGER_TIME > 0）
    return !bee().isAngry();
}

} // namespace mc::client::sound
