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

#pragma once

#include "common/particle/ParticleTypes.hpp"

/**
 * @file ParticleTypes.hpp
 *
 * 客户端层的粒子类型别名头文件。
 * ParticleTypeId 枚举及其辅助函数已移至 common 层（common/particle/ParticleTypes.hpp），
 * 此文件仅提供命名空间别名以保持向后兼容。
 *
 * 已有的 client::renderer::trident::particle::ParticleTypeId 代码
 * 通过命名空间别名继续可用，但新代码应直接使用 mc::particle::ParticleTypeId。
 */

namespace mc::client::renderer::trident::particle {

/// 粒子类型 ID 枚举（从 common 层引入）
using ParticleTypeId = ::mc::particle::ParticleTypeId;

/// 检查粒子类型 ID 是否有效（从 common 层引入）
using ::mc::particle::isValidParticleType;

/// 检查粒子是否需要方块状态数据（从 common 层引入）
using ::mc::particle::requiresBlockState;

/// 检查粒子是否需要物品数据（从 common 层引入）
using ::mc::particle::requiresItemData;

/// 检查粒子是否需要红石颜色数据（从 common 层引入）
using ::mc::particle::requiresDustColor;

/// 检查粒子是否需要颜色数据（从 common 层引入）
using ::mc::particle::requiresColorData;

/// 检查粒子类型是否需要振动数据（从 common 层引入）
using ::mc::particle::requiresVibrationData;

/// 检查粒子是否需要药水类型数据（从 common 层引入）
using ::mc::particle::requiresSpellData;

/// 检查粒子是否需要幽匿充能数据（从 common 层引入）
using ::mc::particle::requiresSculkChargeData;

/// 检查粒子是否需要尖啸延迟数据（从 common 层引入）
using ::mc::particle::requiresShriekData;

/// 检查粒子是否需要轨迹数据（从 common 层引入）
using ::mc::particle::requiresTrailData;

/// 检查粒子是否需要力量数据（从 common 层引入）
using ::mc::particle::requiresPowerData;

/// 检查粒子类型 ID 是否为 MC 协议中定义的类型（从 common 层引入）
using ::mc::particle::isProtocolParticleType;

/// MC 协议粒子类型数量（从 common 层引入）
using ::mc::particle::PROTOCOL_PARTICLE_TYPE_COUNT;

} // namespace mc::client::renderer::trident::particle
