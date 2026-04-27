#pragma once

#include "ParticleRegistry.hpp"

namespace mc::client::renderer::trident::particle {

/**
 * @brief 注册所有内置粒子工厂函数
 *
 * 此函数注册所有粒子类型的工厂函数到 ParticleRegistry。
 * 必须在程序启动时调用（如 ClientApplication 初始化时）。
 */
void registerBuiltinParticleFactories();

} // namespace mc::client::renderer::trident::particle
