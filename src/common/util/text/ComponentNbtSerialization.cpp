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
#include "common/util/json/JsonRead.hpp"

#include "common/core/Types.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/StyleNbt.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "common/util/text/contents/KeybindTextComponent.hpp"
#include "common/util/text/contents/NbtTextComponent.hpp"
#include "common/util/text/contents/ObjectTextComponent.hpp"
#include "common/util/text/contents/ScoreTextComponent.hpp"
#include "common/util/text/contents/SelectorTextComponent.hpp"

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

/// 把不可折叠组件递归序列化为 compound_tag（contents + style + extra）。
/// contents 覆盖原版全部 7 种：text / translate / score / selector / keybind / nbt / object，
/// 字段名与各 Contents 的 MAP_CODEC 一致。
void writeComponentToCompound(nbt::tags::compound_tag& comp, const ITextComponent& component)
{
    // contents：纯文本走 "text" 键（对齐 PlainTextContents.MAP_CODEC fieldOf("text")）
    const auto* str = dynamic_cast<const StringTextComponent*>(&component);
    const auto* translatable = dynamic_cast<const TranslationTextComponent*>(&component);
    const auto* score = dynamic_cast<const ScoreTextComponent*>(&component);
    const auto* selector = dynamic_cast<const SelectorTextComponent*>(&component);
    const auto* keybind = dynamic_cast<const KeybindTextComponent*>(&component);
    const auto* nbtContents = dynamic_cast<const NbtTextComponent*>(&component);
    const auto* object = dynamic_cast<const ObjectTextComponent*>(&component);
    if (str != nullptr) {
        comp.put("text", str->getText());
    } else if (score != nullptr) {
        // ScoreContents：{"score":{"name":...,"objective":...}}
        auto scoreTag = std::make_unique<nbt::tags::compound_tag>();
        scoreTag->put("name", score->name());
        scoreTag->put("objective", score->objective());
        comp.value.emplace("score", std::unique_ptr<nbt::tags::tag>(scoreTag.release()));
    } else if (selector != nullptr) {
        // SelectorContents：{"selector":...,"separator":<Component>?}
        comp.put("selector", selector->selector());
        if (selector->separator() != nullptr) {
            comp.value.emplace("separator", componentToNbtTag(selector->separator().get()));
        }
    } else if (keybind != nullptr) {
        // KeybindContents：{"keybind":"key.jump"}
        comp.put("keybind", keybind->key());
    } else if (nbtContents != nullptr) {
        // NbtContents：{"nbt":...,"interpret":1b,"separator":<Component>?,"block"/"entity"/"storage":...}
        comp.put("nbt", nbtContents->path());
        if (nbtContents->isInterpreting()) {
            comp.put("interpret", static_cast<std::int8_t>(1));
        }
        if (nbtContents->separator() != nullptr) {
            comp.value.emplace("separator", componentToNbtTag(nbtContents->separator().get()));
        }
        comp.put(nbtContents->dataSourceKey(), nbtContents->dataSourceValue());
    } else if (object != nullptr) {
        // ObjectContents：{"object":{...}}，子对象按原样转 NBT（结构与客户端 codec 对应）。
        auto objectTag = ::mc::nbt::jsonToNbt(object->object());
        if (objectTag != nullptr) {
            comp.value.emplace("object", std::unique_ptr<nbt::tags::tag>(objectTag.release()));
        } else {
            spdlog::warn("Component object serialization failed; falling back to plain text");
            comp.put("text", component.getUnformattedText());
        }
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

    // style：与数字格式、Sign 等所有 NBT 场景共用同一份样式编解码
    styleToNbt(comp, component.getStyle());

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

std::unique_ptr<nbt::tags::tag> componentToNbtTag(const ITextComponent* component)
{
    if (component == nullptr) {
        return std::make_unique<nbt::tags::string_tag>(std::string{});
    }
    if (isCollapsible(*component)) {
        // 可折叠 → StringTag（对齐 tryCollapseToString）
        return std::make_unique<nbt::tags::string_tag>(static_cast<const StringTextComponent*>(component)->getText());
    }
    auto comp = std::make_unique<nbt::tags::compound_tag>();
    writeComponentToCompound(*comp, *component);
    return comp;
}

std::vector<u8> componentToNbtBytes(const ITextComponent* component)
{
    const auto tag = componentToNbtTag(component);
    return ::mc::network::buffer::nbt_io::serializeRootTagToBytes(*tag);
}

namespace {

/// 取某个键的字符串标签值；键不存在返回 nullptr
const nbt::tags::string_tag* _findString(const nbt::tags::compound_tag& tag, const char* key)
{
    const auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return nullptr;
    }
    return dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
}

/// 取某个键的复合标签值；键不存在或类型不符返回 nullptr
const nbt::tags::compound_tag* _findCompound(const nbt::tags::compound_tag& tag, const char* key)
{
    const auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return nullptr;
    }
    return dynamic_cast<const nbt::tags::compound_tag*>(it->second.get());
}

/// 取某个键的 ByteTag 值（NBT 布尔）；键不存在或类型不符返回 nullopt
std::optional<i8> _findByte(const nbt::tags::compound_tag& tag, const char* key)
{
    const auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return std::nullopt;
    }
    const auto* byte = dynamic_cast<const nbt::tags::byte_tag*>(it->second.get());
    if (byte == nullptr) {
        return std::nullopt;
    }
    return byte->value;
}

} // namespace

Result<std::unique_ptr<ITextComponent>> componentFromNbtTag(const nbt::tags::tag& tag)
{
    // StringTag：折叠的纯文本
    if (tag.id() == nbt::TagId::String) {
        return std::unique_ptr<ITextComponent>(
            std::make_unique<StringTextComponent>(dynamic_cast<const nbt::tags::string_tag&>(tag).value));
    }
    if (tag.id() != nbt::TagId::Compound) {
        return Error(ErrorCode::InvalidData,
            "Component NBT must be String or Compound, got tag id " + std::to_string(static_cast<i32>(tag.id())),
            "text::componentFromNbtTag");
    }

    const auto& comp = dynamic_cast<const nbt::tags::compound_tag&>(tag);

    // contents：按写出方向的同一套键识别 7 种 contents
    std::unique_ptr<ITextComponent> result;
    if (const auto* textTag = _findString(comp, "text"); textTag != nullptr) {
        result = std::make_unique<StringTextComponent>(textTag->value);
    } else if (const auto* scoreTag = _findCompound(comp, "score"); scoreTag != nullptr) {
        const auto* name = _findString(*scoreTag, "name");
        const auto* objective = _findString(*scoreTag, "objective");
        result = std::make_unique<ScoreTextComponent>(
            name != nullptr ? name->value : std::string{}, objective != nullptr ? objective->value : std::string{});
    } else if (const auto* selectorTag = _findString(comp, "selector"); selectorTag != nullptr) {
        auto selector = std::make_unique<SelectorTextComponent>(selectorTag->value);
        const auto separatorIt = comp.value.find("separator");
        if (separatorIt != comp.value.end()) {
            auto separator = componentFromNbtTag(*separatorIt->second);
            if (separator.failed()) {
                return separator.error();
            }
            selector->setSeparator(separator.value());
        }
        result = std::move(selector);
    } else if (const auto* keybindTag = _findString(comp, "keybind"); keybindTag != nullptr) {
        result = std::make_unique<KeybindTextComponent>(keybindTag->value);
    } else if (const auto* nbtPathTag = _findString(comp, "nbt"); nbtPathTag != nullptr) {
        NbtDataSourceKind kind = NbtDataSourceKind::Storage;
        std::string dataSourceValue;
        if (const auto* blockTag = _findString(comp, "block"); blockTag != nullptr) {
            kind = NbtDataSourceKind::Block;
            dataSourceValue = blockTag->value;
        } else if (const auto* entityTag = _findString(comp, "entity"); entityTag != nullptr) {
            kind = NbtDataSourceKind::Entity;
            dataSourceValue = entityTag->value;
        } else if (const auto* storageTag = _findString(comp, "storage"); storageTag != nullptr) {
            kind = NbtDataSourceKind::Storage;
            dataSourceValue = storageTag->value;
        } else {
            return Error(ErrorCode::InvalidData,
                "Component NBT contents is missing a data source (block/entity/storage)",
                "text::componentFromNbtTag");
        }

        auto nbt = std::make_unique<NbtTextComponent>(
            nbtPathTag->value, _findByte(comp, "interpret").value_or(0) != 0, kind, std::move(dataSourceValue));
        const auto separatorIt = comp.value.find("separator");
        if (separatorIt != comp.value.end()) {
            auto separator = componentFromNbtTag(*separatorIt->second);
            if (separator.failed()) {
                return separator.error();
            }
            nbt->setSeparator(separator.value());
        }
        result = std::move(nbt);
    } else if (const auto objectIt = comp.value.find("object"); objectIt != comp.value.end()) {
        result = std::make_unique<ObjectTextComponent>(::mc::nbt::nbtToJson(*objectIt->second));
    } else if (const auto* translateTag = _findString(comp, "translate"); translateTag != nullptr) {
        std::vector<std::unique_ptr<ITextComponent>> params;
        const auto withIt = comp.value.find("with");
        if (withIt != comp.value.end()) {
            if (withIt->second->id() != nbt::TagId::List) {
                return Error(ErrorCode::InvalidData, "Component 'with' must be a list", "text::componentFromNbtTag");
            }
            const auto& withList = dynamic_cast<const nbt::tags::list_tag&>(*withIt->second);
            for (size_t i = 0; i < withList.size(); ++i) {
                const auto element = withList[i];
                auto param = componentFromNbtTag(*element);
                if (param.failed()) {
                    return param.error();
                }
                params.push_back(param.value());
            }
        }
        result = std::make_unique<TranslationTextComponent>(translateTag->value, std::move(params));
    } else {
        return Error(ErrorCode::InvalidData,
            "Component NBT has no recognized contents (text/translate/score/selector/keybind/nbt/object)",
            "text::componentFromNbtTag");
    }

    // style（字段与 contents 同层平铺）
    auto style = styleFromNbt(comp);
    if (style.failed()) {
        return style.error();
    }
    result->setStyle(style.value());

    // extra（siblings）
    const auto extraIt = comp.value.find("extra");
    if (extraIt != comp.value.end()) {
        if (extraIt->second->id() != nbt::TagId::List) {
            return Error(ErrorCode::InvalidData, "Component 'extra' must be a list", "text::componentFromNbtTag");
        }
        const auto& extraList = dynamic_cast<const nbt::tags::list_tag&>(*extraIt->second);
        for (size_t i = 0; i < extraList.size(); ++i) {
            const auto element = extraList[i];
            auto sibling = componentFromNbtTag(*element);
            if (sibling.failed()) {
                return sibling.error();
            }
            result->append(sibling.value());
        }
    }

    return result;
}

Result<std::unique_ptr<ITextComponent>> componentFromNbtBytes(const std::vector<u8>& bytes)
{
    if (bytes.empty()) {
        return Error(ErrorCode::InvalidData, "Component NBT bytes are empty", "text::componentFromNbtBytes");
    }

    const u8 tagId = bytes.front();
    if (tagId == 0x08) {
        // StringTag：0x08 + U16 大端长度 + UTF8（折叠纯文本）
        if (bytes.size() < 3) {
            return Error(ErrorCode::InvalidData, "Truncated component StringTag", "text::componentFromNbtBytes");
        }
        const auto length = static_cast<usize>((static_cast<u16>(bytes[1]) << 8) | bytes[2]);
        if (3 + length > bytes.size()) {
            return Error(
                ErrorCode::InvalidData, "Truncated component StringTag payload", "text::componentFromNbtBytes");
        }
        return std::unique_ptr<ITextComponent>(std::make_unique<StringTextComponent>(
            std::string(reinterpret_cast<const char*>(bytes.data() + 3), length)));
    }

    if (tagId == 0x0A) {
        network::buffer::ByteBuf buf(bytes.data(), bytes.size());
        auto root = network::buffer::nbt_io::readRootCompound(buf);
        if (root.failed()) {
            return root.error();
        }
        // 所有权只取一次（value() 是转移语义）
        std::unique_ptr<nbt::tags::compound_tag> tag = root.value();
        if (tag == nullptr) {
            return Error(ErrorCode::InvalidData, "Component NBT root is null", "text::componentFromNbtBytes");
        }
        return componentFromNbtTag(*tag);
    }

    return Error(ErrorCode::InvalidData,
        "Component NBT must start with StringTag(0x08) or CompoundTag(0x0A), got " + std::to_string(tagId),
        "text::componentFromNbtBytes");
}

std::vector<u8> plainTextToNbtBytes(const std::string& text)
{
    // 等价于对一个纯 StringTextComponent 走 componentToNbtBytes（可折叠 → StringTag）。
    const StringTextComponent component(text);
    return componentToNbtBytes(&component);
}

std::vector<u8> parseJsonComponentToNbtBytes(const std::string& jsonStr)
{
    // 尝试解析 JSON 为 ITextComponent
    auto parsedJson = util::json::parse(jsonStr);
    if (parsedJson.failed()) {
        spdlog::warn("parseJsonComponentToNbtBytes: JSON parse failed ({}), fallback to plain text",
            parsedJson.error().message());
    } else {
        auto component = ITextComponent::fromJson(parsedJson.value());
        if (component != nullptr) {
            return componentToNbtBytes(component.get());
        }
    }
    // 解析失败降级为纯文本 StringTag
    return plainTextToNbtBytes(jsonStr);
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
        {
            network::buffer::ByteBuf buf;
            buf.writeBytes(nbtBytes);
            auto result = network::buffer::nbt_io::readRootCompound(buf);
            // 【重要】Result<std::unique_ptr<T>>::value() 是**转移所有权**语义（内部
            // takeValue() 会把 m_value 置空并把裸指针交出去）。因此绝不能在一个表达式里
            // 调用两次 value()：第一次调用返回的临时 unique_ptr 在完整表达式结束时即析构、
            // 对象被释放，第二次调用拿到的是 nullptr，解引用即空指针崩溃。
            // fuzz 实测：畸形 NBT 走到此处触发 SEGV（地址 0x20，正是 compound_tag 内
            // value map 的偏移）。必须先把所有权取到局部变量再用。
            std::unique_ptr<mc::nbt::tags::compound_tag> root = result.success() ? result.value() : nullptr;
            if (root != nullptr) {
                // 还原组件后交给组件的字符串化出口：translate 会查服务端语言表，
                // extra/siblings 与嵌套参数也一并拼进来，比只取 "text" 键完整。
                auto component = componentFromNbtTag(*root);
                if (component.success()) {
                    return component.value()->getUnformattedText();
                }
            }
        }
        return {};
    }
    // 未知 tag id：返回空（不把原始字节当文本，避免 HUD 显示乱码）
    return {};
}

} // namespace mc::text
