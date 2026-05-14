/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "PufferfishEntity.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

PufferfishEntity::PufferfishEntity(LegacyEntityType type, EntityId id)
    : AbstractFishEntity(type, id)
{}

std::unique_ptr<Entity> PufferfishEntity::create(IWorld* /*world*/)
{
    return std::make_unique<PufferfishEntity>(LegacyEntityType::Unknown, 0);
}

f32 PufferfishEntity::getPuffSize() const
{
    switch (m_puffState) {
        case PuffState::Deflated:
            return 0.35f;
        case PuffState::SemiPuffed:
            return 0.5f;
        case PuffState::FullyPuffed:
            return 0.7f;
        default:
            return 0.35f;
    }
}

void PufferfishEntity::tick()
{
    AbstractFishEntity::tick();

    if (m_puffState == PuffState::Deflated) {
        return;
    }

    m_puffTimer++;

    // TODO: 实现 PuffGoal，检测 2 格内的敌人并触发膨胀

    if (m_puffTimer < PUFF_DURATION) {
        return;
    }

    m_deflateTimer++;

    // MC 1.16.5: 收缩逻辑
    if (m_deflateTimer > DEFLATE_FULL_TO_SEMI && m_puffState == PuffState::FullyPuffed) {
        m_puffState = PuffState::SemiPuffed;
        playSound(SoundEvents::ENTITY_PUFFER_FISH_BLOW_OUT, 1.0f, 1.0f);
        m_deflateTimer = 0;
    } else if (m_deflateTimer > DEFLATE_SEMI_TO_DEFLATE && m_puffState == PuffState::SemiPuffed) {
        m_puffState = PuffState::Deflated;
        playSound(SoundEvents::ENTITY_PUFFER_FISH_BLOW_OUT, 1.0f, 1.0f);
        m_deflateTimer = 0;
        m_puffTimer = 0;
    }
}

void PufferfishEntity::setPuffState(PuffState state)
{
    if (state == m_puffState) {
        return;
    }

    PuffState oldState = m_puffState;
    m_puffState = state;

    if (static_cast<i32>(state) > static_cast<i32>(oldState)) {
        playSound(SoundEvents::ENTITY_PUFFER_FISH_BLOW_UP, 1.0f, 1.0f);
    }
}

std::optional<ResourceLocation> PufferfishEntity::getAmbientSound() const
{
    if (!isInWater()) {
        return SoundEvents::ENTITY_PUFFER_FISH_FLOP;
    }
    return SoundEvents::ENTITY_PUFFER_FISH_AMBIENT;
}

void PufferfishEntity::registerAttributes()
{
    // 调用父类方法
    AbstractFishEntity::registerAttributes();

    // 河豚的属性
    // 参考 MC 1.16.5 河豚属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

} // namespace mc
