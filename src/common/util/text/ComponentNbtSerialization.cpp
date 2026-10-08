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

#include "common/util/text/ComponentNbtSerialization.hpp"

#include "common/core/Types.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/util/text/TranslationTextComponent.hpp"

#include <spdlog/spdlog.h>

#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc::text {

namespace {

/// 写 StringTag 线格式字节（0x08 + U16 大端长度 + UTF8），对齐 vanilla NbtIo.writeAnyTag(StringTag)
std::vector<u8> stringTagToBytes(const std::string& text)
{
    std::vector<u8> bytes;
    bytes.push_back(0x08); // StringTag tag id
    const usize len = text.size();
    bytes.push_back(static_cast<u8>((len >> 8) & 0xFF)); // U16 大端高字节
    bytes.push_back(static_cast<u8>(len & 0xFF));        // U16 大端低字节
    if (len > 0) {
        bytes.insert(
            bytes.end(), reinterpret_cast<const u8*>(text.data()), reinterpret_cast<const u8*>(text.data()) + len);
    }
    return bytes;
}

/// 判断组件是否可折叠为纯文本 StringTag（对齐 Component.tryCollapseToString）：
/// 纯 StringTextComponent + 空 style + 无 siblings
bool isCollapsible(const ITextComponent& component)
{
    const auto* str = dynamic_cast<const StringTextComponent*>(&component);
    if (str == nullptr) {
        return false;
    }
    return component.getStyle().isEmpty() && component.getSiblings().empty();
}

/// 把 Style 序列化为 compound_tag 的 style 相关键（color/bold/italic/underlined/strikethrough/obfuscated）。
/// 对齐 vanilla Style.Serializer.MAP_CODEC（snake_case 键名，bool 字段以 ByteTag 承载）。
void writeStyleToCompound(nbt::tags::compound_tag& comp, const Style& style)
{
    if (const auto color = style.getColor(); color.has_value() && *color != TextFormatting::None) {
        comp.put("color", ::mc::text::toName(*color));
    }
    if (style.isBold()) {
        comp.put("bold", static_cast<std::int8_t>(1));
    }
    if (style.isItalic()) {
        comp.put("italic", static_cast<std::int8_t>(1));
    }
    if (style.isUnderlined()) {
        comp.put("underlined", static_cast<std::int8_t>(1));
    }
    if (style.isStrikethrough()) {
        comp.put("strikethrough", static_cast<std::int8_t>(1));
    }
    if (style.isObfuscated()) {
        comp.put("obfuscated", static_cast<std::int8_t>(1));
    }
}

/// 把不可折叠组件递归序列化为 compound_tag（contents + style + extra）。
/// contents 支持纯文本（StringTextComponent → "text" 键）与翻译（TranslationTextComponent →
/// "translate" + "with" 键，对齐 vanilla TranslatableContents.MAP_CODEC）；其余类型以
/// text=getUnformattedText 降级承载。
void writeComponentToCompound(nbt::tags::compound_tag& comp, const ITextComponent& component)
{
    // contents：纯文本走 "text" 键（对齐 PlainTextContents.MAP_CODEC fieldOf("text")）
    const auto* str = dynamic_cast<const StringTextComponent*>(&component);
    const auto* translatable = dynamic_cast<const TranslationTextComponent*>(&component);
    if (str != nullptr) {
        comp.put("text", str->getText());
    } else if (translatable != nullptr) {
        // 翻译组件：translate(键) + with(参数列表)，对齐 vanilla TranslatableContents.MAP_CODEC
        // （fieldOf("translate") + optionalFieldOf("with")）。真 Java 客户端据此查语言表本地化，
        // 故必须保留 translate/with 结构，不能折叠成 text。
        comp.put("translate", translatable->getKey());
        const auto& params = translatable->getParams();
        if (!params.empty()) {
            auto withList = std::make_unique<nbt::tags::tag_list_tag>(nbt::TagId::Compound);
            for (const auto& param : params) {
                if (param == nullptr) {
                    continue;
                }
                // with 元素是 Component：可折叠则写 StringTag，否则 CompoundTag。ListTag 元素类型
                // 须一致，故统一用 CompoundTag 承载（可折叠元素也包成 {text:"..."}），与 extra 同范式。
                auto child = std::make_unique<nbt::tags::compound_tag>();
                writeComponentToCompound(*child, *param);
                withList->value.push_back(std::unique_ptr<nbt::tags::tag>(child.release()));
            }
            comp.value.emplace("with", std::unique_ptr<nbt::tags::tag>(withList.release()));
        }
    } else {
        // 其余 contents（keybind/score/selector 等）暂以纯文本降级（项目业务无此类 S→C 包）
        comp.put("text", component.getUnformattedText());
    }

    // style
    writeStyleToCompound(comp, component.getStyle());

    // extra（siblings）→ ListTag of CompoundTag
    const auto& siblings = component.getSiblings();
    if (!siblings.empty()) {
        auto extraList = std::make_unique<nbt::tags::tag_list_tag>(nbt::TagId::Compound);
        for (const auto& sibling : siblings) {
            if (sibling == nullptr) {
                continue;
            }
            // siblings 内可折叠则写 StringTag，否则 CompoundTag。vanilla extra 是 List<Component>，
            // 元素经 tryCollapseToString 可折叠为 StringTag。ListTag 元素类型须一致，故统一用 CompoundTag
            // 承载（不可折叠元素的最小形式），可折叠元素也包成 {text:"..."}。
            auto child = std::make_unique<nbt::tags::compound_tag>();
            writeComponentToCompound(*child, *sibling);
            extraList->value.push_back(std::unique_ptr<nbt::tags::tag>(child.release()));
        }
        comp.value.emplace("extra", std::unique_ptr<nbt::tags::tag>(extraList.release()));
    }
}

} // namespace

std::vector<u8> componentToNbtBytes(const ITextComponent* component)
{
    if (component == nullptr) {
        return stringTagToBytes(std::string{});
    }
    if (isCollapsible(*component)) {
        // 可折叠 → StringTag（对齐 tryCollapseToString）
        return stringTagToBytes(static_cast<const StringTextComponent*>(component)->getText());
    }
    // 不可折叠 → CompoundTag（0x0A + entries + 0x00，无 root name）
    nbt::tags::compound_tag comp;
    writeComponentToCompound(comp, *component);
    return ::mc::network::buffer::nbt_io::serializeRootCompoundToBytes(comp);
}

std::vector<u8> plainTextToNbtBytes(const std::string& text)
{
    return stringTagToBytes(text);
}

std::vector<u8> parseJsonComponentToNbtBytes(const std::string& jsonStr)
{
    // 尝试解析 JSON 为 ITextComponent
    try {
        auto json = nlohmann::json::parse(jsonStr);
        auto component = ITextComponent::fromJson(json);
        if (component != nullptr) {
            return componentToNbtBytes(component.get());
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("parseJsonComponentToNbtBytes: JSON parse failed ({}), fallback to plain text", e.what());
    }
    // 解析失败降级为纯文本 StringTag
    return stringTagToBytes(jsonStr);
}

namespace {

/// 从 nbtBytes 起始读一个 NBT StringTag payload（U16 大端 + UTF8），返回字符串。
/// 失败返回空 optional。nbtBytes[0] 须为 0x08。
std::optional<std::string> readStringTag(const std::vector<u8>& nbtBytes)
{
    if (nbtBytes.size() < 3) {
        return std::nullopt;
    }
    const u16 len = static_cast<u16>((static_cast<u16>(nbtBytes[1]) << 8) | nbtBytes[2]);
    if (static_cast<usize>(3) + len > nbtBytes.size()) {
        return std::nullopt;
    }
    return std::string(reinterpret_cast<const char*>(nbtBytes.data() + 3), len);
}

/// 从 CompoundTag 取 "text" 键的字符串值（不递归 extra，近似纯文本）。找不到返回空。
std::string extractTextFromCompound(const nbt::tags::compound_tag& comp)
{
    const auto it = comp.value.find("text");
    if (it != comp.value.end()) {
        const auto* str = dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
        if (str != nullptr) {
            return str->value;
        }
    }
    const auto trIt = comp.value.find("translate");
    if (trIt != comp.value.end()) {
        const auto* str = dynamic_cast<const nbt::tags::string_tag*>(trIt->second.get());
        if (str != nullptr) {
            return str->value;
        }
    }
    return {};
}

} // namespace

std::string componentNbtBytesToPlainText(const std::vector<u8>& nbtBytes)
{
    if (nbtBytes.empty()) {
        return {};
    }
    const u8 tagId = nbtBytes[0];
    if (tagId == 0x08) {
        // StringTag：纯文本折叠路径
        auto s = readStringTag(nbtBytes);
        if (s.has_value()) {
            return *s;
        }
        return {};
    }
    if (tagId == 0x0A) {
        // CompoundTag：用 NbtIo 解析后取 "text" 键
        try {
            network::buffer::ByteBuf buf;
            buf.writeBytes(nbtBytes);
            auto result = network::buffer::nbt_io::readRootCompound(buf);
            // 【重要】Result<std::unique_ptr<T>>::value() 是**转移所有权**语义（内部
            // takeValue() 会把 m_value 置空并把裸指针交出去）。因此绝不能在一个表达式里
            // 调用两次 value()：第一次调用返回的临时 unique_ptr 在完整表达式结束时即析构、
            // 对象被释放，第二次调用拿到的是 nullptr，解引用即空指针崩溃。
            // fuzz 实测：畸形 NBT 走到此处触发 SEGV（地址 0x20，正是 compound_tag 内
            // value map 的偏移）。必须先把所有权取到局部变量再用。
            std::unique_ptr<mc::nbt::tags::compound_tag> root =
                result.success() ? result.value() : nullptr;
            if (root != nullptr) {
                std::string text = extractTextFromCompound(*root);
                if (!text.empty()) {
                    return text;
                }
            }
        }
        catch (const std::exception& e) {
            spdlog::warn("componentNbtBytesToPlainText: CompoundTag parse failed ({})", e.what());
        }
        return {};
    }
    // 未知 tag id：返回空（不把原始字节当文本，避免 HUD 显示乱码）
    return {};
}

} // namespace mc::text
