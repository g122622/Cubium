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
 * IMPLIED, ANY KIND OF INCLUDING
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "CopyNbtFunction.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/NbtPath.hpp"
#include "common/command/arguments/NbtPathArgumentType.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <cstddef>
#include <exception>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace loot {

// ============================================================================
// NbtOperation 构造函数
// ============================================================================

CopyNbtFunction::NbtOperation::NbtOperation(std::string srcPath, std::string tgtPath, Operation op)
    : sourcePath(std::move(srcPath))
    , targetPath(std::move(tgtPath))
    , operation(op)
    , parsedSourcePath()
    , parsedTargetPath()
    , pathsParsed(false)
{}

// ============================================================================
// CopyNbtFunction
// ============================================================================

CopyNbtFunction::CopyNbtFunction(Source source) noexcept
    : m_source(source)
{}

void CopyNbtFunction::addOperation(const std::string& sourcePath, const std::string& targetPath, Operation operation)
{
    m_operations.emplace_back(sourcePath, targetPath, operation);
}

ItemStack CopyNbtFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty() || m_operations.empty()) {
        return stack;
    }

    // 1. 从 LootContext 中获取源 NBT 数据
    auto sourceTagPtr = _resolveSourceNbt(context);
    if (sourceTagPtr == nullptr) {
        return stack;
    }

    nbt::tags::compound_tag& sourceTag = *sourceTagPtr;

    // 2. 将 ItemStack 的自定义数据从 JSON 转为 NBT，用于路径操作
    bool targetModified = false;
    std::unique_ptr<nbt::tags::compound_tag> targetNbt;

    // 获取 ItemStack 现有的自定义数据标签
    const nlohmann::json* existingTag = stack.getTag();
    if (existingTag != nullptr && existingTag->is_object() && !existingTag->empty()) {
        targetNbt = _jsonToNbtCompound(*existingTag);
    }
    if (targetNbt == nullptr) {
        targetNbt = std::make_unique<nbt::tags::compound_tag>();
    }

    // 3. 对每个操作应用合并策略
    for (const auto& op : m_operations) {
        _ensurePathsParsed(op);

        // 跳过空路径（解析失败）
        if (op.parsedSourcePath.empty() || op.parsedTargetPath.empty()) {
            continue;
        }

        // 从源 NBT 中读取路径匹配的值
        auto sourceValues = op.parsedSourcePath.get(sourceTag);
        if (sourceValues.empty()) {
            continue;
        }

        // 应用合并策略到目标 NBT
        _applyOperation(op, sourceTag, *targetNbt);
        targetModified = true;
    }

    // 4. 将修改后的 NBT 转回 JSON 并写回 ItemStack
    if (targetModified) {
        nlohmann::json jsonTag = _nbtCompoundToJson(*targetNbt);
        if (jsonTag.is_object() && !jsonTag.empty()) {
            stack.mergeTag(jsonTag);
        }
    }

    return stack;
}

std::unique_ptr<LootFunction> CopyNbtFunction::clone() const noexcept
{
    auto func = std::make_unique<CopyNbtFunction>(m_source);
    func->m_operations = m_operations;
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

std::unique_ptr<nbt::tags::compound_tag> CopyNbtFunction::_resolveSourceNbt(LootContext& context) const
{
    switch (m_source) {
        case Source::This: {
            auto* entity = context.get<Entity>(LootParams::THIS_ENTITY);
            if (entity == nullptr) {
                return nullptr;
            }
            auto tag = std::make_unique<nbt::tags::compound_tag>();
            entity->writeToNBT(*tag);
            return tag;
        }
        case Source::Killer: {
            auto* killer = context.get<Entity>(LootParams::KILLER_ENTITY);
            if (killer == nullptr) {
                return nullptr;
            }
            auto tag = std::make_unique<nbt::tags::compound_tag>();
            killer->writeToNBT(*tag);
            return tag;
        }
        case Source::KillerPlayer: {
            auto* player = context.get<Player>(LootParams::KILLER_PLAYER);
            if (player == nullptr) {
                return nullptr;
            }
            auto tag = std::make_unique<nbt::tags::compound_tag>();
            player->writeToNBT(*tag);
            return tag;
        }
        case Source::BlockEntity: {
            auto* blockEntity = context.get<BlockEntity>(LootParams::BLOCK_ENTITY);
            if (blockEntity == nullptr) {
                return nullptr;
            }
            auto tag = std::make_unique<nbt::tags::compound_tag>();
            blockEntity->saveToNBT(*tag);
            return tag;
        }
    }
    return nullptr;
}

void CopyNbtFunction::_ensurePathsParsed(const NbtOperation& op) const
{
    if (op.pathsParsed) {
        return;
    }

    try {
        command::StringReader sourceReader(op.sourcePath);
        command::NbtPathArgumentType parser;
        op.parsedSourcePath = parser.parse(sourceReader);
    }
    catch (const std::exception&) {
        op.parsedSourcePath = command::NbtPath();
    }

    try {
        command::StringReader targetReader(op.targetPath);
        command::NbtPathArgumentType parser;
        op.parsedTargetPath = parser.parse(targetReader);
    }
    catch (const std::exception&) {
        op.parsedTargetPath = command::NbtPath();
    }

    op.pathsParsed = true;
}

void CopyNbtFunction::_applyOperation(
    const NbtOperation& op, nbt::tags::compound_tag& sourceTag, nbt::tags::compound_tag& targetTag) const
{
    // 从源 NBT 中获取路径匹配的值
    auto sourceValues = op.parsedSourcePath.get(sourceTag);
    if (sourceValues.empty()) {
        return;
    }

    switch (op.operation) {
        case Operation::Replace: {
            // 替换：将目标路径设置为源路径的最后一个匹配值
            const nbt::tags::tag* lastValue = sourceValues.back();
            op.parsedTargetPath.set(targetTag, [lastValue]() { return lastValue->copy(); });
            break;
        }
        case Operation::Append: {
            // 追加：将所有源值的副本追加到目标路径处的列表中
            // 先尝试获取目标路径处的现有值
            auto existingValues = op.parsedTargetPath.get(targetTag);

            // 如果目标不存在或是列表类型，使用 append
            bool targetIsList = false;
            if (!existingValues.empty()) {
                for (const auto* val : existingValues) {
                    if (val->id() == nbt::TagId::List) {
                        targetIsList = true;
                        break;
                    }
                }
            }

            if (targetIsList || existingValues.empty()) {
                // 使用 NbtPath::append 追加所有源值
                std::vector<std::unique_ptr<nbt::tags::tag>> copies;
                for (const auto* srcVal : sourceValues) {
                    copies.push_back(srcVal->copy());
                }
                op.parsedTargetPath.append(targetTag, copies);
            }
            break;
        }
        case Operation::Merge: {
            // 合并：将源复合标签的键值对合并到目标路径处的复合标签中
            for (const auto* srcVal : sourceValues) {
                if (srcVal->id() == nbt::TagId::Compound) {
                    const auto* compoundSrc = dynamic_cast<const nbt::tags::compound_tag*>(srcVal);
                    if (compoundSrc != nullptr) {
                        op.parsedTargetPath.merge(targetTag, *compoundSrc);
                    }
                }
            }
            break;
        }
    }
}

// ============================================================================
// NBT 与 JSON 互转辅助函数
// ============================================================================

namespace {

/**
 * @brief 将 NBT 标签转换为 nlohmann::json
 */
nlohmann::json nbtTagToJson(const nbt::tags::tag& tag);

nlohmann::json nbtListToJson(const nbt::tags::list_tag& list)
{
    nlohmann::json result = nlohmann::json::array();
    for (size_t i = 0; i < list.size(); ++i) {
        auto elem = list[i];
        if (elem) {
            result.push_back(nbtTagToJson(*elem));
        }
    }
    return result;
}

nlohmann::json nbtTagToJson(const nbt::tags::tag& tag)
{
    using namespace nbt::tags;

    switch (tag.id()) {
        case nbt::TagId::Byte: {
            const auto& t = dynamic_cast<const byte_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Short: {
            const auto& t = dynamic_cast<const short_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Int: {
            const auto& t = dynamic_cast<const int_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Long: {
            const auto& t = dynamic_cast<const long_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Float: {
            const auto& t = dynamic_cast<const float_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Double: {
            const auto& t = dynamic_cast<const double_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::ByteArray: {
            const auto& t = dynamic_cast<const bytearray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (const auto& val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case nbt::TagId::String: {
            const auto& t = dynamic_cast<const string_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::List: {
            const auto& t = dynamic_cast<const list_tag&>(tag);
            return nbtListToJson(t);
        }
        case nbt::TagId::Compound: {
            const auto& t = dynamic_cast<const compound_tag&>(tag);
            nlohmann::json result = nlohmann::json::object();
            for (const auto& [key, value] : t.value) {
                if (value) {
                    result[key] = nbtTagToJson(*value);
                }
            }
            return result;
        }
        case nbt::TagId::IntArray: {
            const auto& t = dynamic_cast<const intarray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (const auto& val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case nbt::TagId::LongArray: {
            const auto& t = dynamic_cast<const longarray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (const auto& val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case nbt::TagId::End:
        default:
            return nullptr;
    }
}

/**
 * @brief 将 nlohmann::json 转换为 NBT tag
 */
std::unique_ptr<nbt::tags::tag> jsonToNbtTag(const nlohmann::json& json)
{
    using namespace nbt::tags;

    switch (json.type()) {
        case nlohmann::json::value_t::null:
            return nullptr;

        case nlohmann::json::value_t::boolean: {
            auto tag = std::make_unique<byte_tag>();
            tag->value = json.get<bool>() ? 1 : 0;
            return tag;
        }

        case nlohmann::json::value_t::number_integer: {
            auto tag = std::make_unique<int_tag>();
            tag->value = json.get<i32>();
            return tag;
        }

        case nlohmann::json::value_t::number_unsigned: {
            auto val = json.get<u64>();
            if (val <= static_cast<u64>(std::numeric_limits<i32>::max())) {
                auto tag = std::make_unique<int_tag>();
                tag->value = static_cast<i32>(val);
                return tag;
            }
            auto tag = std::make_unique<long_tag>();
            tag->value = static_cast<i64>(val);
            return tag;
        }

        case nlohmann::json::value_t::number_float: {
            auto tag = std::make_unique<double_tag>();
            tag->value = json.get<f64>();
            return tag;
        }

        case nlohmann::json::value_t::string: {
            auto tag = std::make_unique<string_tag>();
            tag->value = json.get<std::string>();
            return tag;
        }

        case nlohmann::json::value_t::array: {
            auto listTag = std::make_unique<tag_list_tag>();
            listTag->eid = nbt::TagId::Compound;
            bool firstElement = true;

            for (const auto& elem : json) {
                auto childTag = jsonToNbtTag(elem);
                if (childTag != nullptr) {
                    if (firstElement) {
                        listTag->eid = childTag->id();
                        firstElement = false;
                    }
                    listTag->value.push_back(std::move(childTag));
                }
            }

            if (listTag->value.empty()) {
                listTag->eid = nbt::TagId::End;
            }

            return listTag;
        }

        case nlohmann::json::value_t::object: {
            auto compound = std::make_unique<compound_tag>();
            for (const auto& [key, value] : json.items()) {
                auto childTag = jsonToNbtTag(value);
                if (childTag != nullptr) {
                    compound->value[key] = std::move(childTag);
                }
            }
            return compound;
        }

        default:
            return nullptr;
    }
}

} // anonymous namespace

std::unique_ptr<nbt::tags::compound_tag> CopyNbtFunction::_jsonToNbtCompound(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return nullptr;
    }

    auto result = jsonToNbtTag(json);
    if (result == nullptr || result->id() != nbt::TagId::Compound) {
        return nullptr;
    }

    return std::unique_ptr<nbt::tags::compound_tag>(dynamic_cast<nbt::tags::compound_tag*>(result.release()));
}

nlohmann::json CopyNbtFunction::_nbtCompoundToJson(const nbt::tags::compound_tag& tag)
{
    return nbtTagToJson(tag);
}

} // namespace loot
} // namespace mc
