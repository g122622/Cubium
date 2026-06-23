/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "DripstoneDripParticle.hpp"
#include "client/world/ClientWorld.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// DripstoneWaterDripParticle
// ============================================================================

DripstoneWaterDripParticle::DripstoneWaterDripParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : DripParticle(pos, velocity, DripType::Water)
{
    // 与 DripWaterParticle 一致的参数，对齐 MC Java 版 DripstoneWaterHangProvider
    setSize(0.01f + m_random.nextFloat() * 0.01f);
    // MC Java 版 dripstone water 水滴颜色: (0.2F, 0.3F, 1.0F)
    setColor(glm::vec4(0.2f, 0.3f, 1.0f, 0.8f));
    setMaxAge(40.0f);
}

std::unique_ptr<Particle> DripstoneWaterDripParticle::createDripping(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<DripstoneWaterDripParticle>(pos, velocity);
}

std::unique_ptr<Particle> DripstoneWaterDripParticle::createFalling(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripstoneWaterDripParticle>(pos, velocity);
    particle->m_dripState = DripState::Falling;
    particle->m_dripProgress = 1.0;
    particle->setGravity(0.06f);
    return particle;
}

std::unique_ptr<Particle> DripstoneWaterDripParticle::createLanding(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripstoneWaterDripParticle>(pos, velocity);
    particle->m_dripState = DripState::Landed;
    particle->setMaxAge(16.0f);
    particle->setSize(0.04f);
    return particle;
}

u32 DripstoneWaterDripParticle::getLightColor(mc::client::ClientWorld* world) const
{
    return Particle::getLightColor(world);
}

void DripstoneWaterDripParticle::onLand(mc::client::ClientWorld* world)
{
    // 先调用父类处理落地状态和生成落地粒子
    DripParticle::onLand(world);

    // 滴水石水滴落地时播放滴水音效
    // 对齐 MC Java: SoundEvents.POINTED_DRIPSTONE_DRIP_WATER
    // 音量: Mth.randomBetween(random, 0.3F, 1.0F)，音调: 1.0F
    if (world != nullptr) {
        f32 volume = 0.3f + m_random.nextFloat() * 0.7f;
        world->playLocalSound(SoundEvents::BLOCK_POINTED_DRIPSTONE_DRIP_WATER,
            sound::SoundCategory::Blocks,
            Vector3(m_position.x, m_position.y, m_position.z),
            volume,
            1.0f);
    }
}

// ============================================================================
// DripstoneLavaDripParticle
// ============================================================================

DripstoneLavaDripParticle::DripstoneLavaDripParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : DripParticle(pos, velocity, DripType::Lava)
{
    // 与 DripParticle(Lava) 一致的参数，对齐 MC Java 版 DripstoneLavaHangProvider
    setSize(0.01f + m_random.nextFloat() * 0.01f);
    // MC Java 版 dripstone lava 滴颜色继承自 Lava: (1.0F, 0.3F, 0.0F, 1.0F)
    setColor(glm::vec4(1.0f, 0.3f, 0.0f, 1.0f));
    setMaxAge(40.0f);
}

std::unique_ptr<Particle> DripstoneLavaDripParticle::createDripping(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<DripstoneLavaDripParticle>(pos, velocity);
}

std::unique_ptr<Particle> DripstoneLavaDripParticle::createFalling(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripstoneLavaDripParticle>(pos, velocity);
    particle->m_dripState = DripState::Falling;
    particle->m_dripProgress = 1.0;
    particle->setGravity(0.06f);
    return particle;
}

std::unique_ptr<Particle> DripstoneLavaDripParticle::createLanding(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripstoneLavaDripParticle>(pos, velocity);
    particle->m_dripState = DripState::Landed;
    particle->setMaxAge(16.0f);
    particle->setSize(0.04f);
    return particle;
}

u32 DripstoneLavaDripParticle::getLightColor(mc::client::ClientWorld* world) const
{
    // 熔岩滴发光：基础光照 + 随生命周期的变化
    f64 lifeRatio = m_age / m_maxAge;
    u8 blockLight = 15;
    u8 skyLight = static_cast<u8>(std::min(15.0, lifeRatio * 15.0));
    return (static_cast<u32>(skyLight) << 4) | static_cast<u32>(blockLight);
}

void DripstoneLavaDripParticle::onLand(mc::client::ClientWorld* world)
{
    // 先调用父类处理落地状态和生成落地粒子
    DripParticle::onLand(world);

    // 滴水石熔岩滴落地时播放滴熔岩音效
    // 对齐 MC Java: SoundEvents.POINTED_DRIPSTONE_DRIP_LAVA
    // 音量: Mth.randomBetween(random, 0.3F, 1.0F)，音调: 1.0F
    if (world != nullptr) {
        f32 volume = 0.3f + m_random.nextFloat() * 0.7f;
        world->playLocalSound(SoundEvents::BLOCK_POINTED_DRIPSTONE_DRIP_LAVA,
            sound::SoundCategory::Blocks,
            Vector3(m_position.x, m_position.y, m_position.z),
            volume,
            1.0f);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
