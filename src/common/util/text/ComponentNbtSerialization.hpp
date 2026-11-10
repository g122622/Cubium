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
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <memory>
#include <string>
#include <vector>

namespace mc::text {
class ITextComponent;
} // namespace mc::text

namespace mc::text {

/**
 * @brief ITextComponent ↔ NBT 标签 / 1.21.11 Component NBT wire 字节序列化
 *
 * 对齐 vanilla `ComponentSerialization.TRUSTED_STREAM_CODEC`：Component 经 `Codec.encode(NbtOps)`
 * 序列化为 NBT，再经 `FriendlyByteBuf.writeNbt` = `NbtIo.writeAnyTag` 写出（tagId + payload，无 root name、
 * 无外层 VarInt 长度）。
 *
 * 编码规则（对齐 `ComponentSerialization.createCodec` + `Component.tryCollapseToString`）：
 * - **可折叠**（PlainTextContents 即纯 StringTextComponent，且 style 为空、无 siblings）→ `StringTag`
 *   线格式（`0x08` + U16 大端长度 + UTF8），与 `writeTextComponentNbt` 一致。
 * - **不可折叠**（带 style/extra 或非纯文本 contents）→ `CompoundTag` 线格式（`0x0A` + entries + `0x00`），
 *   含 `text`/`translate` 等 contents 键、`extra`（ListTag of CompoundTag）、style 键
 *   （`color`/`bold`/`italic`/`underlined`/`strikethrough`/`obfuscated`/`insertion`，见 `StyleNbt.hpp`）。
 *
 * 供记分板/Boss 条/标题/动作栏/聊天等 S→C 包的 Component 字段使用：业务侧调用本工具把 ITextComponent
 * 序列化为 NBT wire 字节填入 IR 的 `std::vector<u8>` 字段，codec 层 `writeComponentNbt` 直接 writeBytes
 * 该字节（无 writeOpaque 的 VarInt 长度前缀），从而与真 Java 客户端互通。
 *
 * 内存侧（存档、命令参数、数字格式载荷）只需标签而不需线格式，用 `componentToNbtTag` /
 * `componentFromNbtTag`，避免「序列化成字节再解析回来」的往返开销。
 */

/**
 * @brief 把 ITextComponent 序列化为 NBT 标签（可折叠→StringTag，否则 CompoundTag）
 *
 * @param component 文本组件（可为 nullptr，此时返回空 StringTag）
 * @return 新创建的标签（所有权归调用方）
 */
[[nodiscard]] std::unique_ptr<nbt::tags::tag> componentToNbtTag(const ITextComponent* component);

/**
 * @brief 从 NBT 标签还原 ITextComponent（`componentToNbtTag` 的逆向）
 *
 * 支持 StringTag（折叠纯文本）与 CompoundTag（`text` / `translate` + `with` / style / `extra`）。
 * `keybind` / `score` / `selector` / `nbt` 等其余 contents 尚未建模，遇到时返回错误而不是产出
 * 与原文不符的组件——这些类型在项目里也没有对应的 ITextComponent 子类。
 *
 * @param tag NBT 标签
 * @return 还原出的组件，或错误
 */
[[nodiscard]] Result<std::unique_ptr<ITextComponent>> componentFromNbtTag(const nbt::tags::tag& tag);

/**
 * @brief 从 Component NBT wire 字节还原 ITextComponent（`componentToNbtBytes` 的逆向）
 *
 * 处理 `readComponentNbt`（Java codec 层）返回的原始字节：首字节是标签类型，
 * `0x08` 为折叠纯文本（U16 大端长度 + UTF8），`0x0A` 为复合标签（交给
 * `componentFromNbtTag`）。这是网络解码路径上「字节 → 结构化组件」的唯一入口，
 * 供物品自定义名/描述等 S→C 字段使用。
 *
 * 与 `componentNbtBytesToPlainText` 的区别：本函数保真还原 style / extra / translate，
 * 后者是有损的纯文本近似，只应在「不支持的 contents 类型」降级时使用。
 *
 * @param bytes Component NBT wire 字节
 * @return 还原出的组件，或错误（空输入、未知标签类型、截断、不支持的 contents）
 */
[[nodiscard]] Result<std::unique_ptr<ITextComponent>> componentFromNbtBytes(const std::vector<u8>& bytes);

/**
 * @brief 把 ITextComponent 序列化为 1.21.11 Component NBT wire 字节
 *
 * @param component 文本组件（可为 nullptr，此时返回空 StringTag 字节）
 * @return NBT wire 字节（StringTag 或 CompoundTag）
 */
[[nodiscard]] std::vector<u8> componentToNbtBytes(const ITextComponent* component);

/**
 * @brief 把纯文本字符串序列化为可折叠 Component NBT wire 字节（StringTag）
 *
 * 等价于对纯 StringTextComponent 调用 componentToNbtBytes。供业务侧仅有裸字符串（无 ITextComponent
 * 对象）的场景使用，如 ServerPlayer::sendStatusMessage 的 actionBar 裸字符串。
 *
 * @param text 纯文本
 * @return StringTag wire 字节（0x08 + U16 + UTF8）
 */
[[nodiscard]] std::vector<u8> plainTextToNbtBytes(const std::string& text);

/**
 * @brief 把 JSON 文本（命令行传入的组件 JSON 字符串）解析后序列化为 Component NBT wire 字节
 *
 * 解析 JSON 为 ITextComponent 再走 componentToNbtBytes。解析失败时降级为纯文本 StringTag
 * （把原 JSON 字符串当纯文本承载），并记 warn 日志。供 TitleCommand 等 `/title <player> title <json>`
 * 场景使用。
 *
 * @param jsonStr 组件 JSON 字符串
 * @return NBT wire 字节
 */
[[nodiscard]] std::vector<u8> parseJsonComponentToNbtBytes(const std::string& jsonStr);

/**
 * @brief 把 Component NBT wire 字节解码为纯文本（getUnformattedText 的近似）
 *
 * 供客户端把收到的 NBT wire 字节还原为可显示字符串（标题/动作栏等 HUD 仅需纯文本）。
 * - StringTag(0x08)：取 UTF8 文本。
 * - CompoundTag(0x0A)：解析后取 "text" 键的字符串值（不递归 extra，近似纯文本）。
 * - 其他/解析失败：兜底把原始字节当 ASCII 文本（去掉首部 NBT tag id/长度字节后尽可能可读）。
 *
 * 这是 componentToNbtBytes 的**有损反向**——只取纯文本，丢弃 style/extra 结构。
 * 完整 Component 还原需 ITextComponent::fromJson/NBT 反序列化，本函数不承担。
 *
 * @param nbtBytes Component NBT wire 字节
 * @return 近似纯文本
 */
[[nodiscard]] std::string componentNbtBytesToPlainText(const std::vector<u8>& nbtBytes);

} // namespace mc::text
