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
 * The copyright notice and this permission notice shall be included in all
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

#include "NbtPath.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// Bring operator<< for nbt::tags::tag into scope for ADL
using mc::nbt::operator<<;

namespace mc {
namespace command {

// ========== NbtPath 实现 ==========

NbtPath::NbtPath(std::string rawText, std::vector<std::unique_ptr<NbtPathNode>> nodes)
    : m_rawText(std::move(rawText))
    , m_nodes(std::move(nodes))
{}

NbtPath::NbtPath(const NbtPath& other)
    : m_rawText(other.m_rawText)
{
    // 深拷贝节点
    for (const auto& node : other.m_nodes) {
        m_nodes.push_back(node->clone());
    }
}

NbtPath& NbtPath::operator=(const NbtPath& other)
{
    if (this != &other) {
        m_rawText = other.m_rawText;
        m_nodes.clear();
        for (const auto& node : other.m_nodes) {
            m_nodes.push_back(node->clone());
        }
    }
    return *this;
}

std::vector<const nbt::tags::tag*> NbtPath::get(const nbt::tags::compound_tag& tag) const
{
    std::vector<const nbt::tags::tag*> result;
    result.push_back(&tag);

    for (const auto& node : m_nodes) {
        std::vector<const nbt::tags::tag*> next;
        for (const nbt::tags::tag* t : result) {
            auto nodeResults = node->get(t);
            next.insert(next.end(), nodeResults.begin(), nodeResults.end());
        }
        result = std::move(next);
        if (result.empty()) {
            break;
        }
    }

    return result;
}

const nbt::tags::tag* NbtPath::getSingle(const nbt::tags::compound_tag& tag) const
{
    auto results = get(tag);
    if (results.empty()) {
        throw CommandException(CommandErrorType::NbtPathNotFound, "NBT path '" + m_rawText + "' not found", 0);
    }
    if (results.size() > 1) {
        throw CommandException(
            CommandErrorType::NbtPathMultipleResults, "NBT path '" + m_rawText + "' returned multiple results", 0);
    }
    return results[0];
}

i32 NbtPath::count(const nbt::tags::compound_tag& tag) const
{
    return static_cast<i32>(get(tag).size());
}

bool NbtPath::exists(const nbt::tags::compound_tag& tag) const
{
    return !get(tag).empty();
}

i32 NbtPath::set(nbt::tags::compound_tag& tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const
{
    if (m_nodes.empty()) {
        return 0;
    }

    // 获取到倒数第二个节点的所有父标签
    std::vector<nbt::tags::tag*> parents;
    parents.push_back(&tag);

    for (size_t i = 0; i < m_nodes.size() - 1; ++i) {
        std::vector<nbt::tags::tag*> next;
        for (nbt::tags::tag* t : parents) {
            // 使用 getOrCreate 让中间节点自动创建
            auto nodeResults = m_nodes[i]->getOrCreate(t, nullptr);
            next.insert(next.end(), nodeResults.begin(), nodeResults.end());
        }
        parents = std::move(next);
        if (parents.empty()) {
            return 0;
        }
    }

    // 在最后一个节点上设置值
    const auto& lastNode = m_nodes.back();
    i32 count = 0;
    for (nbt::tags::tag* parent : parents) {
        count += lastNode->set(parent, valueSupplier);
    }

    return count;
}

i32 NbtPath::remove(nbt::tags::compound_tag& tag) const
{
    if (m_nodes.empty()) {
        return 0;
    }

    // 获取到倒数第二个节点的所有父标签
    std::vector<nbt::tags::tag*> parents;
    parents.push_back(&tag);

    for (size_t i = 0; i < m_nodes.size() - 1; ++i) {
        std::vector<nbt::tags::tag*> next;
        for (nbt::tags::tag* t : parents) {
            auto nodeResults = m_nodes[i]->get(t);
            next.insert(next.end(), nodeResults.begin(), nodeResults.end());
        }
        parents = std::move(next);
        if (parents.empty()) {
            return 0;
        }
    }

    // 在最后一个节点上删除值
    const auto& lastNode = m_nodes.back();
    i32 count = 0;
    for (nbt::tags::tag* parent : parents) {
        count += lastNode->remove(parent);
    }

    return count;
}

i32 NbtPath::insert(
    nbt::tags::compound_tag& tag, i32 index, const std::vector<std::unique_ptr<nbt::tags::tag>>& values) const
{
    if (m_nodes.empty() || values.empty()) {
        return 0;
    }

    // 获取目标列表
    std::vector<nbt::tags::tag*> targets;
    targets.push_back(&tag);

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        std::vector<nbt::tags::tag*> next;
        for (nbt::tags::tag* t : targets) {
            auto nodeResults = m_nodes[i]->get(t);
            next.insert(next.end(), nodeResults.begin(), nodeResults.end());
        }
        targets = std::move(next);
        if (targets.empty()) {
            return 0;
        }
    }

    i32 count = 0;
    for (nbt::tags::tag* target : targets) {
        if (target->id() != nbt::TagId::List) {
            throw CommandException(
                CommandErrorType::NbtPathInvalidType, "Expected list at path '" + m_rawText + "'", 0);
        }

        // 只支持 tag_list_tag 类型的插入操作
        auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(target);
        if (tagList == nullptr) {
            // 对于其他类型的列表（如 int_list_tag），不支持插入
            // 因为它们有固定类型的元素
            throw CommandException(
                CommandErrorType::NbtPathInvalidType, "Cannot insert into typed list at path '" + m_rawText + "'", 0);
        }

        i32 insertIndex = index < 0 ? static_cast<i32>(tagList->value.size()) + index + 1 : index;
        if (insertIndex < 0 || insertIndex > static_cast<i32>(tagList->value.size())) {
            throw CommandException(CommandErrorType::NbtPathIndexOutOfBounds,
                "Index " + std::to_string(index) + " out of bounds for list of size " +
                    std::to_string(tagList->value.size()),
                0);
        }

        // 插入值
        for (const auto& value : values) {
            tagList->value.insert(tagList->value.begin() + insertIndex, value->copy());
            insertIndex++;
            count++;
        }
    }

    return count;
}

i32 NbtPath::append(nbt::tags::compound_tag& tag, const std::vector<std::unique_ptr<nbt::tags::tag>>& values) const
{
    // append 等同于 insert 在末尾
    return insert(tag, -1, values);
}

i32 NbtPath::prepend(nbt::tags::compound_tag& tag, const std::vector<std::unique_ptr<nbt::tags::tag>>& values) const
{
    // prepend 等同于 insert 在开头
    return insert(tag, 0, values);
}

i32 NbtPath::merge(nbt::tags::compound_tag& tag, const nbt::tags::compound_tag& value) const
{
    if (m_nodes.empty()) {
        return 0;
    }

    // 获取目标标签
    std::vector<nbt::tags::tag*> targets;
    targets.push_back(&tag);

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        std::vector<nbt::tags::tag*> next;
        for (nbt::tags::tag* t : targets) {
            auto nodeResults = m_nodes[i]->getOrCreate(t, []() { return std::make_unique<nbt::tags::compound_tag>(); });
            next.insert(next.end(), nodeResults.begin(), nodeResults.end());
        }
        targets = std::move(next);
        if (targets.empty()) {
            return 0;
        }
    }

    i32 count = 0;
    for (nbt::tags::tag* target : targets) {
        if (target->id() != nbt::TagId::Compound) {
            throw CommandException(
                CommandErrorType::NbtPathInvalidType, "Expected compound tag at path '" + m_rawText + "'", 0);
        }

        auto* compound = dynamic_cast<nbt::tags::compound_tag*>(target);
        if (compound == nullptr) {
            continue;
        }

        // 合并值
        for (const auto& [key, val] : value.value) {
            compound->value[key] = val->copy();
        }
        count++;
    }

    return count;
}

// ========== NbtPathStringNode 实现 ==========

std::vector<nbt::tags::tag*> NbtPathStringNode::get(nbt::tags::tag* tag) const
{
    std::vector<nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return result;
    }

    auto* compound = dynamic_cast<nbt::tags::compound_tag*>(tag);
    if (compound == nullptr) {
        return result;
    }

    auto it = compound->value.find(m_name);
    if (it != compound->value.end()) {
        result.push_back(it->second.get());
    }

    return result;
}

std::vector<const nbt::tags::tag*> NbtPathStringNode::get(const nbt::tags::tag* tag) const
{
    std::vector<const nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return result;
    }

    const auto* compound = dynamic_cast<const nbt::tags::compound_tag*>(tag);
    if (compound == nullptr) {
        return result;
    }

    auto it = compound->value.find(m_name);
    if (it != compound->value.end()) {
        result.push_back(it->second.get());
    }

    return result;
}

i32 NbtPathStringNode::set(nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return 0;
    }

    auto* compound = dynamic_cast<nbt::tags::compound_tag*>(tag);
    if (compound == nullptr || valueSupplier == nullptr) {
        return 0;
    }

    auto value = valueSupplier();
    if (value == nullptr) {
        return 0;
    }

    compound->value[m_name] = std::move(value);
    return 1;
}

i32 NbtPathStringNode::remove(nbt::tags::tag* tag) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return 0;
    }

    auto* compound = dynamic_cast<nbt::tags::compound_tag*>(tag);
    if (compound == nullptr) {
        return 0;
    }

    return compound->erase(m_name) ? 1 : 0;
}

std::vector<nbt::tags::tag*> NbtPathStringNode::getOrCreate(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const
{
    std::vector<nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return result;
    }

    auto* compound = dynamic_cast<nbt::tags::compound_tag*>(tag);
    if (compound == nullptr) {
        return result;
    }

    auto it = compound->value.find(m_name);
    if (it != compound->value.end()) {
        result.push_back(it->second.get());
    } else if (creator) {
        auto value = creator();
        if (value != nullptr) {
            result.push_back(value.get());
            compound->value[m_name] = std::move(value);
        }
    }

    return result;
}

// ========== NbtPathIndexNode 实现 ==========

std::vector<nbt::tags::tag*> NbtPathIndexNode::get(nbt::tags::tag* tag) const
{
    std::vector<nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return result;
    }

    auto* list = dynamic_cast<nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return result;
    }

    i32 index = m_index < 0 ? static_cast<i32>(list->size()) + m_index : m_index;
    if (index < 0 || index >= static_cast<i32>(list->size())) {
        return result;
    }

    result.push_back((*list)[static_cast<size_t>(index)].get());
    return result;
}

std::vector<const nbt::tags::tag*> NbtPathIndexNode::get(const nbt::tags::tag* tag) const
{
    std::vector<const nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return result;
    }

    const auto* list = dynamic_cast<const nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return result;
    }

    i32 index = m_index < 0 ? static_cast<i32>(list->size()) + m_index : m_index;
    if (index < 0 || index >= static_cast<i32>(list->size())) {
        return result;
    }

    result.push_back((*list)[static_cast<size_t>(index)].get());
    return result;
}

i32 NbtPathIndexNode::set(nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::List || valueSupplier == nullptr) {
        return 0;
    }

    auto* list = dynamic_cast<nbt::tags::tag_list_tag*>(tag);
    if (list == nullptr) {
        // 尝试其他列表类型
        return 0;
    }

    i32 index = m_index < 0 ? static_cast<i32>(list->size()) + m_index : m_index;
    if (index < 0 || index >= static_cast<i32>(list->size())) {
        return 0;
    }

    auto value = valueSupplier();
    if (value == nullptr) {
        return 0;
    }

    list->value[static_cast<size_t>(index)] = std::move(value);
    return 1;
}

i32 NbtPathIndexNode::remove(nbt::tags::tag* tag) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return 0;
    }

    auto* list = dynamic_cast<nbt::tags::tag_list_tag*>(tag);
    if (list == nullptr) {
        return 0;
    }

    i32 index = m_index < 0 ? static_cast<i32>(list->size()) + m_index : m_index;
    if (index < 0 || index >= static_cast<i32>(list->size())) {
        return 0;
    }

    list->value.erase(list->value.begin() + index);
    return 1;
}

std::vector<nbt::tags::tag*> NbtPathIndexNode::getOrCreate(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const
{
    // 索引节点不支持创建
    MC_UNUSED(creator);
    return get(tag);
}

// ========== NbtPathAllElementsNode 实现 ==========

std::vector<nbt::tags::tag*> NbtPathAllElementsNode::get(nbt::tags::tag* tag) const
{
    std::vector<nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return result;
    }

    auto* list = dynamic_cast<nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return result;
    }

    for (size_t i = 0; i < list->size(); ++i) {
        result.push_back((*list)[i].get());
    }

    return result;
}

std::vector<const nbt::tags::tag*> NbtPathAllElementsNode::get(const nbt::tags::tag* tag) const
{
    std::vector<const nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return result;
    }

    const auto* list = dynamic_cast<const nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return result;
    }

    for (size_t i = 0; i < list->size(); ++i) {
        result.push_back((*list)[i].get());
    }

    return result;
}

i32 NbtPathAllElementsNode::set(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const
{
    // 空列表节点不支持设置（需要知道具体索引）
    MC_UNUSED(tag);
    MC_UNUSED(valueSupplier);
    return 0;
}

i32 NbtPathAllElementsNode::remove(nbt::tags::tag* tag) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return 0;
    }

    auto* list = dynamic_cast<nbt::tags::tag_list_tag*>(tag);
    if (list == nullptr) {
        return 0;
    }

    i32 count = static_cast<i32>(list->size());
    list->value.clear();
    return count;
}

std::vector<nbt::tags::tag*> NbtPathAllElementsNode::getOrCreate(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const
{
    MC_UNUSED(creator);
    return get(tag);
}

// ========== NbtPathCompoundFilterNode 实现 ==========

NbtPathCompoundFilterNode::NbtPathCompoundFilterNode(std::unique_ptr<nbt::tags::compound_tag> filter)
    : m_filter(std::move(filter))
{}

std::unique_ptr<NbtPathNode> NbtPathCompoundFilterNode::clone() const
{
    auto filterCopy = m_filter
        ? std::unique_ptr<nbt::tags::compound_tag>(dynamic_cast<nbt::tags::compound_tag*>(m_filter->copy().release()))
        : nullptr;
    return std::make_unique<NbtPathCompoundFilterNode>(std::move(filterCopy));
}

bool NbtPathCompoundFilterNode::_matches(const nbt::tags::compound_tag& tag) const
{
    if (m_filter == nullptr) {
        return true;
    }

    // 检查 filter 中的所有键是否都在 tag 中存在且值匹配
    for (const auto& [key, value] : m_filter->value) {
        auto it = tag.value.find(key);
        if (it == tag.value.end()) {
            return false;
        }
        // 简单比较：值类型相同即认为匹配（不深度比较）
        if (it->second->id() != value->id()) {
            return false;
        }
    }

    return true;
}

std::vector<nbt::tags::tag*> NbtPathCompoundFilterNode::get(nbt::tags::tag* tag) const
{
    std::vector<nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return result;
    }

    auto* compound = dynamic_cast<nbt::tags::compound_tag*>(tag);
    if (compound == nullptr) {
        return result;
    }

    if (_matches(*compound)) {
        result.push_back(compound);
    }

    return result;
}

std::vector<const nbt::tags::tag*> NbtPathCompoundFilterNode::get(const nbt::tags::tag* tag) const
{
    std::vector<const nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return result;
    }

    const auto* compound = dynamic_cast<const nbt::tags::compound_tag*>(tag);
    if (compound == nullptr) {
        return result;
    }

    if (_matches(*compound)) {
        result.push_back(compound);
    }

    return result;
}

i32 NbtPathCompoundFilterNode::set(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const
{
    // 过滤器节点不支持设置
    MC_UNUSED(tag);
    MC_UNUSED(valueSupplier);
    return 0;
}

i32 NbtPathCompoundFilterNode::remove(nbt::tags::tag* tag) const
{
    // 过滤器节点不支持删除
    MC_UNUSED(tag);
    return 0;
}

std::vector<nbt::tags::tag*> NbtPathCompoundFilterNode::getOrCreate(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const
{
    MC_UNUSED(creator);
    return get(tag);
}

std::string NbtPathCompoundFilterNode::toString() const
{
    if (m_filter == nullptr) {
        return "{}";
    }
    std::ostringstream ss;
    ss << nbt::contexts::mojangson << *m_filter;
    return ss.str();
}

// ========== NbtPathListFilterNode 实现 ==========

NbtPathListFilterNode::NbtPathListFilterNode(std::unique_ptr<nbt::tags::compound_tag> filter)
    : m_filter(std::move(filter))
{}

std::unique_ptr<NbtPathNode> NbtPathListFilterNode::clone() const
{
    auto filterCopy = m_filter
        ? std::unique_ptr<nbt::tags::compound_tag>(dynamic_cast<nbt::tags::compound_tag*>(m_filter->copy().release()))
        : nullptr;
    return std::make_unique<NbtPathListFilterNode>(std::move(filterCopy));
}

bool NbtPathListFilterNode::_matches(const nbt::tags::compound_tag& tag) const
{
    if (m_filter == nullptr) {
        return true;
    }

    for (const auto& [key, value] : m_filter->value) {
        auto it = tag.value.find(key);
        if (it == tag.value.end()) {
            return false;
        }
        if (it->second->id() != value->id()) {
            return false;
        }
    }

    return true;
}

std::vector<nbt::tags::tag*> NbtPathListFilterNode::get(nbt::tags::tag* tag) const
{
    std::vector<nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return result;
    }

    auto* list = dynamic_cast<nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return result;
    }

    for (size_t i = 0; i < list->size(); ++i) {
        auto element = (*list)[i];
        if (element && element->id() == nbt::TagId::Compound) {
            auto* compound = dynamic_cast<nbt::tags::compound_tag*>(element.get());
            if (compound && _matches(*compound)) {
                result.push_back(element.get());
            }
        }
    }

    return result;
}

std::vector<const nbt::tags::tag*> NbtPathListFilterNode::get(const nbt::tags::tag* tag) const
{
    std::vector<const nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return result;
    }

    const auto* list = dynamic_cast<const nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return result;
    }

    for (size_t i = 0; i < list->size(); ++i) {
        auto element = (*list)[i];
        if (element && element->id() == nbt::TagId::Compound) {
            const auto* compound = dynamic_cast<const nbt::tags::compound_tag*>(element.get());
            if (compound && _matches(*compound)) {
                result.push_back(element.get());
            }
        }
    }

    return result;
}

i32 NbtPathListFilterNode::set(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::List || valueSupplier == nullptr) {
        return 0;
    }

    auto* list = dynamic_cast<nbt::tags::tag_list_tag*>(tag);
    if (list == nullptr) {
        return 0;
    }

    i32 count = 0;
    for (size_t i = 0; i < list->size(); ++i) {
        auto& element = list->value[i];
        if (element && element->id() == nbt::TagId::Compound) {
            auto* compound = dynamic_cast<nbt::tags::compound_tag*>(element.get());
            if (compound && _matches(*compound)) {
                auto newValue = valueSupplier();
                if (newValue) {
                    list->value[i] = std::move(newValue);
                    count++;
                }
            }
        }
    }

    return count;
}

i32 NbtPathListFilterNode::remove(nbt::tags::tag* tag) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return 0;
    }

    auto* list = dynamic_cast<nbt::tags::tag_list_tag*>(tag);
    if (list == nullptr) {
        return 0;
    }

    i32 count = 0;
    for (auto it = list->value.begin(); it != list->value.end();) {
        if (*it && (*it)->id() == nbt::TagId::Compound) {
            auto* compound = dynamic_cast<nbt::tags::compound_tag*>(it->get());
            if (compound && _matches(*compound)) {
                it = list->value.erase(it);
                count++;
                continue;
            }
        }
        ++it;
    }

    return count;
}

std::vector<nbt::tags::tag*> NbtPathListFilterNode::getOrCreate(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const
{
    MC_UNUSED(creator);
    return get(tag);
}

std::string NbtPathListFilterNode::toString() const
{
    if (m_filter == nullptr) {
        return "[{}]";
    }
    std::ostringstream ss;
    ss << "[" << nbt::contexts::mojangson << *m_filter << "]";
    return ss.str();
}

// ========== NbtPathKeyFilterNode 实现 ==========

NbtPathKeyFilterNode::NbtPathKeyFilterNode(std::string name, std::unique_ptr<nbt::tags::compound_tag> filter)
    : m_name(std::move(name))
    , m_filter(std::move(filter))
{}

std::unique_ptr<NbtPathNode> NbtPathKeyFilterNode::clone() const
{
    auto filterCopy = m_filter
        ? std::unique_ptr<nbt::tags::compound_tag>(dynamic_cast<nbt::tags::compound_tag*>(m_filter->copy().release()))
        : nullptr;
    return std::make_unique<NbtPathKeyFilterNode>(m_name, std::move(filterCopy));
}

std::vector<nbt::tags::tag*> NbtPathKeyFilterNode::get(nbt::tags::tag* tag) const
{
    std::vector<nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return result;
    }

    auto* compound = dynamic_cast<nbt::tags::compound_tag*>(tag);
    if (compound == nullptr) {
        return result;
    }

    auto it = compound->value.find(m_name);
    if (it == compound->value.end() || it->second == nullptr) {
        return result;
    }

    // 检查是否匹配过滤器
    if (it->second->id() == nbt::TagId::Compound && m_filter != nullptr) {
        auto* childCompound = dynamic_cast<nbt::tags::compound_tag*>(it->second.get());
        if (childCompound != nullptr) {
            for (const auto& [key, value] : m_filter->value) {
                auto childIt = childCompound->value.find(key);
                if (childIt == childCompound->value.end() || childIt->second->id() != value->id()) {
                    return result;
                }
            }
        }
    }

    result.push_back(it->second.get());
    return result;
}

std::vector<const nbt::tags::tag*> NbtPathKeyFilterNode::get(const nbt::tags::tag* tag) const
{
    std::vector<const nbt::tags::tag*> result;
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return result;
    }

    const auto* compound = dynamic_cast<const nbt::tags::compound_tag*>(tag);
    if (compound == nullptr) {
        return result;
    }

    auto it = compound->value.find(m_name);
    if (it == compound->value.end() || it->second == nullptr) {
        return result;
    }

    // 检查是否匹配过滤器
    if (it->second->id() == nbt::TagId::Compound && m_filter != nullptr) {
        const auto* childCompound = dynamic_cast<const nbt::tags::compound_tag*>(it->second.get());
        if (childCompound != nullptr) {
            for (const auto& [key, value] : m_filter->value) {
                auto childIt = childCompound->value.find(key);
                if (childIt == childCompound->value.end() || childIt->second->id() != value->id()) {
                    return result;
                }
            }
        }
    }

    result.push_back(it->second.get());
    return result;
}

i32 NbtPathKeyFilterNode::set(nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::Compound || valueSupplier == nullptr) {
        return 0;
    }

    auto* compound = dynamic_cast<nbt::tags::compound_tag*>(tag);
    if (compound == nullptr) {
        return 0;
    }

    auto it = compound->value.find(m_name);
    if (it == compound->value.end() || it->second == nullptr) {
        return 0;
    }

    // 如果有过滤器，需要检查是否匹配
    if (it->second->id() == nbt::TagId::Compound && m_filter != nullptr) {
        auto* childCompound = dynamic_cast<nbt::tags::compound_tag*>(it->second.get());
        if (childCompound != nullptr) {
            for (const auto& [key, value] : m_filter->value) {
                auto childIt = childCompound->value.find(key);
                if (childIt == childCompound->value.end() || childIt->second->id() != value->id()) {
                    return 0;
                }
            }
        }
    }

    auto value = valueSupplier();
    if (value == nullptr) {
        return 0;
    }

    compound->value[m_name] = std::move(value);
    return 1;
}

i32 NbtPathKeyFilterNode::remove(nbt::tags::tag* tag) const
{
    // 过滤器节点不支持删除
    MC_UNUSED(tag);
    return 0;
}

std::vector<nbt::tags::tag*> NbtPathKeyFilterNode::getOrCreate(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator) const
{
    MC_UNUSED(creator);
    return get(tag);
}

std::string NbtPathKeyFilterNode::toString() const
{
    std::ostringstream ss;
    ss << m_name;
    if (m_filter != nullptr) {
        ss << nbt::contexts::mojangson << *m_filter;
    }
    return ss.str();
}

} // namespace command
} // namespace mc
