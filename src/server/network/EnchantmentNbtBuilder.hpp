/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
 * @brief 启动期注册 enchantment 内联 NBT 构建所需的 datapack 源
 *
 * 服务端 initializeRegistries()（ItemTags 已加载之后）调用一次。EnchantmentNbtBuilder
 * 持文件静态 `const DataPackRepository*`，与服务器同生命周期。注册后 buildEnchantment-
 * RegistryEntries() 可在任意连接的握手阶段安全调用。
 */
void setEnchantmentDatapackSource(const mc::resource::DataPackRepository& repo);

/**
 * @brief 构造全部 43 个 enchantment RegistryEntry（带内联 NBT）
 *
 * 替代 RegistryDataBuilder 旧的 data=nullopt 硬编码列表。每个条目发送【内联 NBT】
 * （非 nullopt），原因见下方根因说明。结果 process 级缓存（std::call_once 首次构建），
 * 后续连接直接复用，避免重复 IO/解析。
 *
 * 条目 id 顺序与原硬编码列表严格一致——enchantment 名字虽按 name 解码（不依赖 int id），
 * 但保留顺序以兼容未来 UpdateTags 的 int id 映射。
 *
 * **根因（为何必须内联 NBT 而非 data=nullopt）：** enchantment JSON 的
 * supported_items/primary_items 引用 **ITEM 标签**（形如 #minecraft:enchantable/sharp_weapon）。ITEM 是
 * 静态注册表，其整数 id 由客户端运行时 bootstrap 决定、源码无显式列表，服务端无法复制 →
 * 服务端无法发 ITEM UpdateTags 绑定这些标签。data=nullopt 路径下客户端从本地 core 包加载
 * enchantment JSON 解码时，HolderSetCodec.lookupTag 因 ITEM 标签未绑定返回 "Missing tag"
 * → "Failed to parse local data"（全部 43 个）。
 *
 * 内联 NBT 把 supported_items/primary_items/exclusive_set 的 #tag 展平为显式元素名列表
 * （HolderSetCodec 走 Either.right → HolderSet.direct → RegistryFixedCodec 按 name 解码），
 * 绕开 lookupTag 与未绑定 Named。
 */
[[nodiscard]] std::vector<mc::network::ir::configuration::RegistryEntry> buildEnchantmentRegistryEntries();

/**
 * @brief 测试用：非缓存直构建（注入指定 datapack 源 + 已注册的 ItemTags）
 *
 * 生产代码用上面的缓存版。本函数不触碰进程级缓存，便于单测隔离。
 */
[[nodiscard]] std::vector<mc::network::ir::configuration::RegistryEntry> buildEnchantmentRegistryEntriesUncached(
    const mc::resource::DataPackRepository& repo);

} // namespace mc::server::net
