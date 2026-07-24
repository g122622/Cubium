/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the rights
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

#pragma once

#include "common/core/Types.hpp"
#include "common/network/ir/packets/configuration/ConfigurationPackets.hpp"

#include <vector>

namespace mc::server::net {

/**
 * @brief 构造 Configuration 阶段需要推送的 RegistryData 列表
 *
 * 1.21.11 服务端在 Configuration 阶段须按 registryKey 推送注册表：dimension_type、
 * worldgen/biome、chat_type、damage_type、trim_pattern、trim_material、wolf_variant、
 * painting_variant、instrument 等。
 *
 * 风险声明（最高风险项）：本项目的 BiomeRegistry/BlockRegistry/ItemRegistry/EntityRegistry
 * 均无 Java 端 NBT 序列化路径（dimension_type 的维度设置、biome effects 等 NBT 复杂）。
 * 故 Phase 4 策略：所有条目以 RegistryEntry{ id, data=nullopt } 发送——声明"客户端已知"，
 * 仅在客户端 SelectKnownPacks 回命中 minecraft:core 时合法。对我方互通（双方均硬编码
 * vanilla registry）成立；真 Java 客户端因 NBT 缺失会在 Configuration 失败——标 TODO(Phase6)。
 */
[[nodiscard]] std::vector<mc::network::ir::configuration::RegistryData> buildConfigurationRegistryData();

/**
 * @brief 服务端已知的数据包集合（发给客户端协商）
 *
 * 仅声明 minecraft:core（原版核心数据包）。客户端命中后，RegistryData 中 data=nullopt
 * 的条目视为"客户端已通过 core 数据包掌握"，无需 NBT。
 */
[[nodiscard]] std::vector<mc::network::ir::configuration::KnownPack> buildServerKnownPacks();

} // namespace mc::server::net
