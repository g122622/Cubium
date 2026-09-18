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

namespace mc::resource {
class DataPackRepository;
} // namespace mc::resource

namespace mc::server::net {

/**
 * @brief 启动期注册 registry 内联 NBT 构建所需的 datapack 源
 *
 * 服务端 initializeRegistries() 中与 setEnchantmentDatapackSource 同处调用。
 * RegistryDataBuilder 持文件静态 `const DataPackRepository*`，与服务器同生命周期。
 * 注册后 buildConfigurationRegistryDataForUnknownClient() 可在握手阶段安全调用。
 */
void setRegistryDatapackSource(const mc::resource::DataPackRepository& repo);

/**
 * @brief 构造 Configuration 阶段需要推送的 RegistryData 列表
 *
 * 1.21.11 服务端在 Configuration 阶段须按 registryKey 推送全部 23 个
 * RegistryDataLoader.SYNCHRONIZED_REGISTRIES：dimension_type、worldgen/biome、
 * chat_type、trim_material、trim_pattern、wolf_variant、wolf_sound_variant、
 * pig_variant、frog_variant、cat_variant、cow_variant、chicken_variant、
 * zombie_nautilus_variant、painting_variant、damage_type、banner_pattern、
 * enchantment、jukebox_song、instrument、test_environment、test_instance、
 * dialog、timeline。
 *
 * 所有条目以 RegistryEntry{ id, data=nullopt } 发送——声明"客户端已知"。
 * 该策略对真 Java 客户端成立：客户端 SelectKnownPacks 回命中 minecraft:core 后，
 * 对命中 core 的条目按 RegistrySynchronization.packRegistry 规则视为"客户端已知"，
 * 从本地 core 包加载完整 NBT，无需服务端内联（RegistrySynchronization.java:52-62）。
 * 前提是 id 集合与客户端 1.21.11 vanilla core 包完全一致——本列表严格匹配
 * C:\Users\Administrator\minecraft_reborn\datapacks\Vanilla（pack_format=94）。
 *
 * **例外：enchantment 发【内联 NBT】而非 nullopt**（见 EnchantmentNbtBuilder）。
 * enchantment JSON 的 supported_items/primary_items 引用 ITEM 标签（静态注册表），
 * 服务端无法发 ITEM UpdateTags（ITEM 整数 id 由客户端 bootstrap 决定、不可复制），
 * 故 nullopt 路径下客户端解码因 ITEM 标签未绑定报 "Failed to parse local data"。
 * 内联 NBT 把 #tag 展平为显式名字列表绕开该问题。
 *
 * 整数 id（UpdateTags 的 elementId）= 条目在此处的发送顺序索引（0-based）。
 * 客户端 loadContentsFromNetwork 按条目顺序 register() 自增分配 id
 * （RegistryDataLoader.java:343-364）。timeline 等需发标签的注册表，条目顺序须与
 * buildConfigurationUpdateTags() 的索引计算保持一致，故各注册表顺序固定勿乱序。
 */
[[nodiscard]] std::vector<mc::network::ir::configuration::RegistryData> buildConfigurationRegistryData();

/**
 * @brief 构造 Configuration 阶段需要推送的 UpdateTags 列表
 *
 * 网络同步的 registry 完全替换客户端本地 registry（新建空 MappedRegistry），本地 core
 * 包的 tags/ 目录在网络路径不被读取，故标签必须由 UpdateTags 显式 bindTag
 * （TagLoader.loadTagsForRegistry 仅用于资源路径，非网络路径）。
 *
 * 关键：timeline 的 in_overworld/in_nether/in_end tag 在 dimension_type 解码时因
 * HolderSetCodec.lookupTag → getOrCreateTagForRegistration 被创建为未绑定
 * （MappedRegistry.java:237-243），freeze() 校验未绑定 tag 抛 "Unbound tags"
 * （MappedRegistry.java:290-301）。必须由 UpdateTags 提供非空 payload 才能 bindTag。
 *
 * elementId = 条目在对应 registry（buildConfigurationRegistryData）中的发送顺序索引。
 * 发 timeline 的 4 个 tag（universal/in_overworld/in_nether/in_end，展平后 id）让 timeline
 * 通过 freeze()。另发 dialog 的 2 个空 tag（pause_screen_additions/quick_actions，空 id 列表）：
 * dialog JSON 引用这两个 DIALOG 标签，解码时创建未绑定 Named，须 UpdateTags bindTag(空列表)
 * 绑定（HolderSet.Named.bind 空列表即 bound）。其他 synchronized registry 的元素解码未引用
 * 未绑定 tag，无需额外标签。
 */
[[nodiscard]] std::vector<mc::network::ir::configuration::TagRegistry> buildConfigurationUpdateTags();

/**
 * @brief 服务端已知的数据包集合（发给客户端协商）
 *
 * 仅声明 minecraft:core（原版核心数据包）。客户端命中后，RegistryData 中 data=nullopt
 * 的条目视为"客户端已通过 core 数据包掌握"，无需 NBT。
 */
[[nodiscard]] std::vector<mc::network::ir::configuration::KnownPack> buildServerKnownPacks();

/**
 * @brief 为「未在 SelectKnownPacks 中声明 minecraft:core 的客户端」构造 RegistryData
 *
 * 对齐 Java RegistrySynchronization.packRegistry（RegistrySynchronization.java:37-72）：
 * 该方法**逐条目**判断 `registrationInfo.knownPackInfo` 是否落在客户端声明的 known packs
 * 集合内——命中则发 `Optional.empty()`（无 NBT），未命中则用 `elementCodec().encodeStart`
 * 编码完整 NBT。本函数即「未命中」分支的服务端实现。
 *
 * 为什么必须区分：真 Java 客户端在 SelectKnownPacks(C→S) 中回包含 minecraft:core 的集合，
 * 走 buildConfigurationRegistryData() 的 data=nullopt 路径即可（客户端从本地 core 包加载）。
 * 但第三方客户端（node-minecraft-protocol / mineflayer）恒定回**空** pack 列表，
 * 按 vanilla 语义服务端此时必须下发完整 NBT——否则客户端解析条目时因 value 缺失而失败。
 *
 * **当前实现范围（过渡态）**：仅 dimension_type 下发完整 NBT，其余 22 个注册表**跳过发送**。
 * 依据与限制：
 * - dimension_type 必须发：客户端（mineflayer game.js:70-77）从 dimensionsByName 取
 *   minY/height，取不到时**静默退回 0/256**，而本项目真实世界为 -64/384，会导致区块
 *   section 解码整体错位。故它必须带 NBT 下发。
 * - 其余 22 个注册表跳过发送：客户端保留其本地（minecraft-data）数据。这优于发送空
 *   entries——prismarine-registry 对空 entries 会**清空**本地数据（见其
 *   lib/pc/index.js:71 的 handler 语义），反而更糟。
 * - 但跳过发送**不等价于** vanilla（vanilla 会下发全量 NBT）。
 * TODO: 为其余 22 个注册表实现完整 NBT 编码以彻底对齐 vanilla。阻塞点是各注册表的
 *       字段类型规则需逐个核对 Java codec（整数须精确 int_tag，不能用通用 jsonToNbt —
 *       后者会把 [-128,127] 窄化为 byte_tag，被 Codec.INT 拒绝，参见
 *       EnchantmentNbtBuilder.cpp:462 的同类记录）。
 */
[[nodiscard]] std::vector<mc::network::ir::configuration::RegistryData>
buildConfigurationRegistryDataForUnknownClient();

} // namespace mc::server::net
