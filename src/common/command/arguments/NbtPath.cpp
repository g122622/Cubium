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

namespace {

/// NBT 最大嵌套深度，超过后拒绝写入，避免恶意深嵌套数据导致后续递归处理爆栈
inline constexpr i32 MAX_NBT_DEPTH = 512;

/**
 * @brief 取得列表中指定下标元素的标签指针
 *
 * list_tag::operator[] 返回元素的副本（std::unique_ptr<tag>），对它取 .get() 会得到指向
 * 已析构临时对象的悬垂指针，因此必须按具体列表类型取得元素的稳定地址：
 * - tag_list_tag / list_list_tag：元素本身以 unique_ptr 存储，直接返回元素指针；
 * - compound_list_tag：元素以 compound_tag 值存储，返回其地址；
 * - 数值/字符串/数组列表：元素以裸值存储，树中不存在对应的标签对象，
 *   把元素物化后放入 cache 并返回该副本的地址。
 *
 * @param list 列表标签
 * @param index 元素下标，调用方需保证已完成越界检查
 * @param cache 裸值列表元素的物化缓存，生命周期必须覆盖返回指针的使用期
 * @return 元素标签指针
 */
nbt::tags::tag* elementAt(nbt::tags::list_tag& list, size_t index, std::vector<std::unique_ptr<nbt::tags::tag>>& cache)
{
    if (auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(&list)) {
        return tagList->value[index].get();
    }
    if (auto* listList = dynamic_cast<nbt::tags::list_list_tag*>(&list)) {
        return listList->value[index].get();
    }
    if (auto* compoundList = dynamic_cast<nbt::tags::compound_list_tag*>(&list)) {
        return &compoundList->value[index];
    }
    cache.push_back(list[index]);
    return cache.back().get();
}

/**
 * @brief 取得列表中指定下标元素的标签指针（const 版本）
 */
const nbt::tags::tag* elementAt(
    const nbt::tags::list_tag& list, size_t index, std::vector<std::unique_ptr<nbt::tags::tag>>& cache)
{
    if (const auto* tagList = dynamic_cast<const nbt::tags::tag_list_tag*>(&list)) {
        return tagList->value[index].get();
    }
    if (const auto* listList = dynamic_cast<const nbt::tags::list_list_tag*>(&list)) {
        return listList->value[index].get();
    }
    if (const auto* compoundList = dynamic_cast<const nbt::tags::compound_list_tag*>(&list)) {
        return &compoundList->value[index];
    }
    cache.push_back(list[index]);
    return cache.back().get();
}

/**
 * @brief 把任意存储容器形态的列表原地转换为元素以 unique_ptr<tag> 存储的 tag_list_tag
 *
 * 数值列表、字符串列表、数组列表的元素以裸值存储，复合列表、嵌套列表、空列表的元素是
 * 值类型或 unique_ptr<list_tag>，都无法直接按下标写入/删除。NBT 路径需要原地替换或删除
 * 列表元素时，先把列表转换成一个等价的 tag_list_tag（各子类的 as_tags 会搬移元素值并清空
 * 原容器），再把新列表挂回原来的槽位，这样既保留了元素内容，也让后续的修改可以统一按
 * tag_list_tag 处理。
 *
 * @param slot 指向列表标签的槽位，转换后指向新列表
 * @param list 当前列表；必须来自 slot 且和 slot 指向同一个标签
 * @return 转换出的 tag_list_tag 指针；列表已经是 tag_list_tag 时返回自身
 */
nbt::tags::tag_list_tag* heavyListSlot(std::unique_ptr<nbt::tags::tag>& slot, nbt::tags::list_tag& list)
{
    if (auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(&list)) {
        return tagList;
    }

    auto heavy = std::make_unique<nbt::tags::tag_list_tag>(list.as_tags());
    nbt::tags::tag_list_tag* result = heavy.get();
    slot = std::move(heavy);
    return result;
}

/**
 * @brief 在 NBT 树的某个槽位上查找指定列表标签并原地替换为等价的 tag_list_tag
 *
 * 节点求值只能拿到列表对象（或其元素）本身，拿不到父槽位；而按下标写入/删除需要替换
 * 列表对象，因此这里从根复合标签出发递归查找，找到后把槽位替换成 tag_list_tag。
 *
 * @param holder 当前所在的槽位
 * @param list 要转换的列表标签
 * @param element 列表元素标签；非空时表示调用方拿到的是元素，同样视为命中该列表
 * @return 是否找到并完成了转换
 */
bool replaceListInSlot(
    std::unique_ptr<nbt::tags::tag>& holder, const nbt::tags::list_tag* list, const nbt::tags::tag* element)
{
    if (holder == nullptr) {
        return false;
    }
    if (holder.get() == list || (list != nullptr && element != nullptr && holder.get() == element)) {
        if (dynamic_cast<nbt::tags::tag_list_tag*>(holder.get()) != nullptr) {
            return false;
        }
        auto heavy = std::make_unique<nbt::tags::tag_list_tag>(static_cast<nbt::tags::list_tag&>(*holder).as_tags());
        holder = std::move(heavy);
        return true;
    }
    if (holder->id() == nbt::TagId::Compound) {
        for (auto& [key, value] : static_cast<nbt::tags::compound_tag&>(*holder).value) {
            if (replaceListInSlot(value, list, element)) {
                return true;
            }
        }
    } else if (holder->id() == nbt::TagId::List) {
        if (auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(holder.get())) {
            for (auto& child : tagList->value) {
                if (replaceListInSlot(child, list, element)) {
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * @brief 在 NBT 树中查找标签所在的直接列表容器；重复出现的同一元素以首个命中为准
 * @param holder 当前槽位
 * @param target 待查找的标签
 * @return 包含 target 的列表标签；target 本身是列表时返回它自身
 */
const nbt::tags::list_tag* findOwningList(const std::unique_ptr<nbt::tags::tag>& holder, const nbt::tags::tag* target)
{
    if (holder == nullptr || target == nullptr) {
        return nullptr;
    }
    if (holder.get() == target) {
        return dynamic_cast<const nbt::tags::list_tag*>(holder.get());
    }
    // 元素以裸值/值类型存储，其地址并不等于树中任何标签对象，只能按列表身份查找
    if (holder->id() == nbt::TagId::Compound) {
        for (const auto& [key, value] : static_cast<const nbt::tags::compound_tag&>(*holder).value) {
            if (const auto* found = findOwningList(value, target)) {
                return found;
            }
        }
    } else if (holder->id() == nbt::TagId::List) {
        const auto& list = static_cast<const nbt::tags::list_tag&>(*holder);
        for (size_t i = 0; i < list.size(); ++i) {
            auto element = list[i];
            if (element.get() == target || element->equals(*target)) {
                return &list;
            }
        }
    }
    return nullptr;
}

/**
 * @brief 从根复合标签出发，找到包含目标标签的列表
 * @param root 根复合标签
 * @param target 目标标签（列表元素或列表本身）
 * @return 所属列表，未找到返回 nullptr
 */
const nbt::tags::list_tag* resolveTargetList(const nbt::tags::compound_tag& root, const nbt::tags::tag* target)
{
    if (target == nullptr) {
        return nullptr;
    }
    if (target->id() == nbt::TagId::List) {
        return static_cast<const nbt::tags::list_tag*>(target);
    }
    for (const auto& [key, value] : root.value) {
        if (const auto* found = findOwningList(value, target)) {
            return found;
        }
    }
    return nullptr;
}

/**
 * @brief 从根复合标签出发，把包含指定列表（或列表元素）的槽位替换为 tag_list_tag
 * @param root 根复合标签
 * @param list 目标列表
 * @param element 调用方实际拿到的元素标签，可为空
 * @return 是否发生转换
 */
bool convertListInTree(nbt::tags::compound_tag& root, const nbt::tags::list_tag* list, const nbt::tags::tag* element)
{
    if (list == nullptr || dynamic_cast<const nbt::tags::tag_list_tag*>(list) != nullptr) {
        return false;
    }
    for (auto& [key, value] : root.value) {
        if (replaceListInSlot(value, list, element)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 与 MC NbtUtils.compareNbt(expected, actual, fuzzy=true) 等价的可选子集比较
 *
 * @param expected 过滤器模式，nullptr 表示匹配任意值
 * @param actual 待比较的值，nullptr 视为不匹配
 * @return 是否匹配
 */
bool compareNbt(const nbt::tags::tag* expected, const nbt::tags::tag* actual)
{
    if (expected == actual) {
        return true;
    }
    if (expected == nullptr) {
        return true;
    }
    if (actual == nullptr) {
        return false;
    }
    if (expected->id() != actual->id()) {
        return false;
    }
    if (expected->id() == nbt::TagId::Compound) {
        const auto& expectedCompound = static_cast<const nbt::tags::compound_tag&>(*expected);
        const auto& actualCompound = static_cast<const nbt::tags::compound_tag&>(*actual);
        for (const auto& [key, value] : expectedCompound.value) {
            const auto it = actualCompound.value.find(key);
            const nbt::tags::tag* actualValue = it == actualCompound.value.end() ? nullptr : it->second.get();
            if (!compareNbt(value.get(), actualValue)) {
                return false;
            }
        }
        return true;
    }
    if (expected->id() == nbt::TagId::List) {
        const auto& expectedList = static_cast<const nbt::tags::list_tag&>(*expected);
        const auto& actualList = static_cast<const nbt::tags::list_tag&>(*actual);
        for (size_t i = 0; i < expectedList.size(); ++i) {
            auto expectedElement = expectedList[i];
            bool matched = false;
            for (size_t j = 0; j < actualList.size(); ++j) {
                auto actualElement = actualList[j];
                if (compareNbt(expectedElement.get(), actualElement.get())) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                return false;
            }
        }
        return true;
    }
    return expected->equals(*actual);
}

/**
 * @brief 计算标签的嵌套深度（与原版 NbtPathArgument.isTooDeep 的累加主体一致）
 * @param tag 待计算标签
 * @param depth 当前已累计深度
 * @return tag 连同其子孙的额外嵌套层数
 */
i32 tagExtraDepth(const nbt::tags::tag& tag, i32 depth)
{
    if (depth >= MAX_NBT_DEPTH) {
        return depth;
    }

    i32 result = depth;
    if (tag.id() == nbt::TagId::Compound) {
        const auto& compound = static_cast<const nbt::tags::compound_tag&>(tag);
        for (const auto& element : compound.value) {
            result = tagExtraDepth(*element.second, result + 1);
            if (result >= MAX_NBT_DEPTH) {
                return result;
            }
        }
    } else if (tag.id() == nbt::TagId::List) {
        const auto& list = static_cast<const nbt::tags::list_tag&>(tag);
        for (size_t i = 0; i < list.size(); ++i) {
            auto element = list[i];
            result = tagExtraDepth(*element, result + 1);
            if (result >= MAX_NBT_DEPTH) {
                return result;
            }
        }
    }
    return result;
}

/**
 * @brief 检查 value 写入后是否会超过 NBT 最大嵌套深度
 * @param value 待写入的值
 * @param pathDepth 路径深度
 */
void checkNotTooDeep(const nbt::tags::tag& value, i32 pathDepth)
{
    if (tagExtraDepth(value, pathDepth) >= MAX_NBT_DEPTH) {
        throw CommandException(CommandErrorType::NbtPathDataTooDeep, "NBT data is too deep", 0);
    }
}

} // namespace

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

void NbtPath::_resetElementCaches() const
{
    for (const auto& node : m_nodes) {
        node->resetElementCache();
    }
}

std::vector<const nbt::tags::tag*> NbtPath::get(const nbt::tags::compound_tag& tag) const
{
    _resetElementCaches();

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

// valueSupplier 必须按值持有：同一次 set 可能写入多个父标签，需要多次调用并返回独立的新副本，
// 传 const 引用不会产生拷贝但也无法保证调用方给出的值可重复使用
//
// NOLINTNEXTLINE(performance-unnecessary-value-param)
i32 NbtPath::set(nbt::tags::compound_tag& tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier) const
{
    if (m_nodes.empty()) {
        return 0;
    }

    // 写入后的嵌套层级等于路径深度加上值自身在复合/列表内的层级，超过上限则整体拒绝
    if (auto value = valueSupplier()) {
        if (value != nullptr) {
            checkNotTooDeep(*value, estimatePathDepth());
        }
    }

    _resetElementCaches();

    // 求值到倒数第二个节点：parents 恰好是末级节点的输入集合（复合键节点得到根复合标签，
    // 下标/全元素/列表过滤节点得到目标列表本身）。过程中缺失的节点按原版语义创建：
    // 复合键按下一节点的 createPreferredParentTag 创建；若下一节点是列表访问（末级），
    // 该键的值整体缺失时创建空列表
    auto createForStep = [this](size_t i, const nbt::tags::tag* parent) -> std::unique_ptr<nbt::tags::tag> {
        const NbtPathNode* nextNode = m_nodes[i + 1].get();
        // 下一节点是列表访问（末级）且父标签里还没有对应值时，创建空列表
        if (i + 2 == m_nodes.size() && nextNode->canTargetList() && parent != nullptr &&
            parent->id() == nbt::TagId::Compound) {
            if (nextNode->get(const_cast<nbt::tags::tag*>(parent)).empty()) {
                return std::make_unique<nbt::tags::tag_list_tag>();
            }
        }
        return nextNode->createPreferredParentTag();
    };

    auto collectParents = [this, &tag, &createForStep](std::vector<nbt::tags::tag*>& parents) {
        parents.clear();
        parents.push_back(&tag);
        for (size_t i = 0; i + 1 < m_nodes.size(); ++i) {
            std::vector<nbt::tags::tag*> next;
            for (nbt::tags::tag* t : parents) {
                auto creator = [&createForStep, i, t]() { return createForStep(i, t); };
                auto nodeResults = m_nodes[i]->getOrCreate(t, std::move(creator), false);
                next.insert(next.end(), nodeResults.begin(), nodeResults.end());
            }
            parents = std::move(next);
            if (parents.empty()) {
                return false;
            }
        }
        return true;
    };

    std::vector<nbt::tags::tag*> parents;
    if (!collectParents(parents)) {
        return 0;
    }

    // 末级节点若作用在列表元素上（下标/列表过滤），需先把目标列表转换成可原地修改的
    // tag_list_tag：先把完整路径求值一次拿到目标列表，再在原地替换后重新求值
    if (m_nodes.back()->canTargetList()) {
        std::vector<nbt::tags::tag*> targets = {&tag};
        for (const auto& node : m_nodes) {
            std::vector<nbt::tags::tag*> next;
            for (nbt::tags::tag* t : targets) {
                auto found = node->get(t);
                next.insert(next.end(), found.begin(), found.end());
            }
            targets = std::move(next);
            if (targets.empty()) {
                break;
            }
        }

        bool converted = false;
        for (nbt::tags::tag* target : targets) {
            converted = convertListInTree(tag, resolveTargetList(tag, target), target) || converted;
        }
        if (converted) {
            _resetElementCaches();
            if (!collectParents(parents)) {
                return 0;
            }
        }
    }

    // 在最后一个节点上设置值：复制的值可能被多个父标签共享，供应商每次写入都要给出新副本
    const auto& lastNode = m_nodes.back();
    i32 count = 0;
    for (nbt::tags::tag* parent : parents) {
        count += lastNode->set(parent, valueSupplier, false);
    }

    return count;
}

i32 NbtPath::remove(nbt::tags::compound_tag& tag) const
{
    if (m_nodes.empty()) {
        return 0;
    }

    _resetElementCaches();

    // 末级节点作用在列表元素上时，先把目标列表转换为 tag_list_tag，才能原地删除元素
    if (m_nodes.back()->canTargetList()) {
        std::vector<nbt::tags::tag*> targets = {&tag};
        for (const auto& node : m_nodes) {
            std::vector<nbt::tags::tag*> next;
            for (nbt::tags::tag* t : targets) {
                auto found = node->get(t);
                next.insert(next.end(), found.begin(), found.end());
            }
            targets = std::move(next);
            if (targets.empty()) {
                break;
            }
        }
        bool converted = false;
        for (nbt::tags::tag* target : targets) {
            converted = convertListInTree(tag, resolveTargetList(tag, target), target) || converted;
        }
        if (converted) {
            _resetElementCaches();
        }
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

    // 插入的值会多出一层列表嵌套，同样需要做深度检查
    for (const auto& value : values) {
        checkNotTooDeep(*value, estimatePathDepth());
    }

    _resetElementCaches();

    // 逐级向下求值，同时让缺失的节点按原版自动创建：中间节点按下一节点的
    // createPreferredParentTag 创建，末级节点（必须指向列表）创建空列表
    std::vector<nbt::tags::tag*> targets = {&tag};
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const bool isLast = i + 1 == m_nodes.size();
        std::vector<nbt::tags::tag*> next;
        for (nbt::tags::tag* current : targets) {
            if (isLast) {
                // 末级节点是列表访问时，列表本身（键的值）可能尚未创建，按空列表创建
                auto creator = []() {
                    return std::unique_ptr<nbt::tags::tag>(std::make_unique<nbt::tags::tag_list_tag>());
                };
                auto found = m_nodes[i]->getOrCreate(current, creator, false);
                next.insert(next.end(), found.begin(), found.end());
            } else {
                auto creator = [nextNode = m_nodes[i + 1].get()]() { return nextNode->createPreferredParentTag(); };
                auto found = m_nodes[i]->getOrCreate(current, std::move(creator), false);
                next.insert(next.end(), found.begin(), found.end());
            }
        }
        targets = std::move(next);
        if (targets.empty()) {
            return 0;
        }
    }

    // 路径可能以复合键结尾（目标列表是该键的值），也可能直接以列表访问节点结尾。
    // 两种情况下求值得到的都是列表本身，统一把非 tag_list_tag 的列表原地转换为
    // tag_list_tag；转换会替换旧列表对象并释放元素物化缓存，前后取得的指针都可能失效，
    // 因此每一步转换后都从根重新求值
    auto reevaluateTargets = [this, &tag](std::vector<nbt::tags::tag*>& current) {
        _resetElementCaches();
        current = {&tag};
        for (const auto& node : m_nodes) {
            std::vector<nbt::tags::tag*> next;
            for (nbt::tags::tag* t : current) {
                auto found = node->get(t);
                next.insert(next.end(), found.begin(), found.end());
            }
            current = std::move(next);
            if (current.empty()) {
                return false;
            }
        }
        return true;
    };

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const bool mayIntroduceTargets = m_nodes[i]->canTargetList() || i + 1 == m_nodes.size();
        if (!mayIntroduceTargets) {
            continue;
        }
        bool converted = false;
        for (nbt::tags::tag* current : targets) {
            converted = m_nodes[i]->heavyList(current) || converted;
        }
        if (converted && !reevaluateTargets(targets)) {
            return 0;
        }
    }

    i32 count = 0;
    for (nbt::tags::tag* target : targets) {
        if (target->id() != nbt::TagId::List) {
            throw CommandException(
                CommandErrorType::NbtPathInvalidType, "Expected list at path '" + m_rawText + "'", 0);
        }

        auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(target);
        MC_ASSERT_RELEASE(tagList != nullptr);

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

    _resetElementCaches();

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

i32 NbtPathStringNode::set(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier, bool overwriteSame) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return 0;
    }

    auto* compound = dynamic_cast<nbt::tags::compound_tag*>(tag);
    if (compound == nullptr || valueSupplier == nullptr) {
        return 0;
    }

    // 与 MC CompoundChildNode.setTag 一致：不比较新旧值，调用即视为一次修改
    auto value = valueSupplier();
    if (value == nullptr) {
        return 0;
    }

    MC_UNUSED(overwriteSame);
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
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator, bool overwriteSame) const
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
    } else if (creator && !overwriteSame) {
        auto value = creator();
        if (value != nullptr) {
            result.push_back(value.get());
            compound->value[m_name] = std::move(value);
        }
    }

    return result;
}

std::unique_ptr<nbt::tags::tag> NbtPathStringNode::createPreferredParentTag() const
{
    // 按名字访问的复合键缺失时，原版把它创建为复合标签
    return std::make_unique<nbt::tags::compound_tag>();
}

bool NbtPathStringNode::heavyList(nbt::tags::tag* tag) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return false;
    }

    auto* compound = static_cast<nbt::tags::compound_tag*>(tag);
    auto it = compound->value.find(m_name);
    if (it == compound->value.end() || it->second == nullptr || it->second->id() != nbt::TagId::List) {
        return false;
    }

    auto& list = static_cast<nbt::tags::list_tag&>(*it->second);
    if (dynamic_cast<nbt::tags::tag_list_tag*>(&list) != nullptr) {
        return false;
    }

    heavyListSlot(it->second, list);
    return true;
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

    // 负数索引表示从末尾倒数，例如 -1 指向最后一个元素
    const i32 index = m_index < 0 ? static_cast<i32>(list->size()) + m_index : m_index;
    if (index < 0 || index >= static_cast<i32>(list->size())) {
        return result;
    }

    result.push_back(elementAt(*list, static_cast<size_t>(index), m_elementCache));
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

    // 负数索引表示从末尾倒数，例如 -1 指向最后一个元素
    const i32 index = m_index < 0 ? static_cast<i32>(list->size()) + m_index : m_index;
    if (index < 0 || index >= static_cast<i32>(list->size())) {
        return result;
    }

    result.push_back(elementAt(*list, static_cast<size_t>(index), m_elementCache));
    return result;
}

i32 NbtPathIndexNode::set(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier, bool overwriteSame) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::List || valueSupplier == nullptr) {
        return 0;
    }

    auto* list = dynamic_cast<nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return 0;
    }

    const i32 index = m_index < 0 ? static_cast<i32>(list->size()) + m_index : m_index;
    if (index < 0 || index >= static_cast<i32>(list->size())) {
        return 0;
    }

    // 与 MC IndexedElementNode.setTag 一致：新旧值相等时不计为修改
    auto existing = (*list)[static_cast<size_t>(index)];
    auto value = valueSupplier();
    if (value == nullptr || value->equals(*existing)) {
        return 0;
    }

    MC_UNUSED(overwriteSame);
    auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(list);
    if (tagList == nullptr) {
        // 裸值/复合/嵌套列表的元素类型固定，无法原地放入任意类型的标签；
        // TODO: NbtPath::set 可通过 heavyList 预先把列表转换为 tag_list_tag 来支持按下标写入，
        //   该节点自身只持有元素路径，无法定位并替换列表所在槽位
        return 0;
    }

    tagList->value[static_cast<size_t>(index)] = std::move(value);
    return 1;
}

i32 NbtPathIndexNode::remove(nbt::tags::tag* tag) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return 0;
    }

    auto* list = dynamic_cast<nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return 0;
    }

    // 与 MC IndexedElementNode.removeTag 一致：删除不计入迭代时的下标回退
    const i32 index = m_index < 0 ? static_cast<i32>(list->size()) + m_index : m_index;
    if (index < 0 || index >= static_cast<i32>(list->size())) {
        return 0;
    }

    auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(list);
    if (tagList == nullptr || tagList->element_id() == nbt::TagId::List) {
        // 元素以裸值/值类型/嵌套列表存储，无法从排序后的节点路径中安全定位父槽位进行原地删除；
        // TODO: 为 list_tag 增加 removeAt(index) 虚方法，让各子类自行按具体容器删除元素
        return 0;
    }

    tagList->value.erase(tagList->value.begin() + index);
    return 1;
}

std::vector<nbt::tags::tag*> NbtPathIndexNode::getOrCreate(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator, bool overwriteSame) const
{
    // 索引节点不支持创建
    MC_UNUSED(creator);
    MC_UNUSED(overwriteSame);
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
        result.push_back(elementAt(*list, i, m_elementCache));
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
        result.push_back(elementAt(*list, i, m_elementCache));
    }

    return result;
}

i32 NbtPathAllElementsNode::set(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier, bool overwriteSame) const
{
    // 与 MC AllElementsNode.setTag 一致：空列表写入一个值；非空列表把所有元素替换为同一个值，
    // 返回被替换掉的（与新值不相等的）元素数量
    if (tag == nullptr || tag->id() != nbt::TagId::List || valueSupplier == nullptr) {
        return 0;
    }

    auto* list = dynamic_cast<nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return 0;
    }

    MC_UNUSED(overwriteSame);
    const size_t size = list->size();
    if (size == 0) {
        auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(list);
        if (tagList == nullptr) {
            // 空列表按定义是 end_list_tag，不能容纳任何元素
            return 0;
        }
        auto value = valueSupplier();
        if (value == nullptr) {
            return 0;
        }
        tagList->value.push_back(std::move(value));
        return 1;
    }

    auto value = valueSupplier();
    if (value == nullptr) {
        return 0;
    }

    i32 changed = 0;
    for (size_t i = 0; i < size; ++i) {
        auto element = (*list)[i];
        if (!value->equals(*element)) {
            changed++;
        }
    }
    if (changed == 0) {
        return 0;
    }

    auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(list);
    if (tagList == nullptr) {
        // TODO: 为 list_tag 增加 setAll 虚方法，让各子类按具体容器把全部元素替换为给定值
        return 0;
    }

    tagList->value.clear();
    tagList->value.push_back(value->copy());
    for (size_t i = 1; i < size; ++i) {
        tagList->value.push_back(value->copy());
    }
    return changed;
}

i32 NbtPathAllElementsNode::remove(nbt::tags::tag* tag) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return 0;
    }

    auto* list = dynamic_cast<nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return 0;
    }

    i32 count = static_cast<i32>(list->size());
    if (count == 0) {
        return 0;
    }

    // 删除全部元素，返回值与 MC AllElementsNode.removeTag 一致（原列表长度）
    if (auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(list)) {
        tagList->value.clear();
    } else if (auto* listList = dynamic_cast<nbt::tags::list_list_tag*>(list)) {
        listList->value.clear();
    } else if (auto* compoundList = dynamic_cast<nbt::tags::compound_list_tag*>(list)) {
        compoundList->value.clear();
    } else if (auto* stringList = dynamic_cast<nbt::tags::string_list_tag*>(list)) {
        stringList->value.clear();
    } else if (auto* byteList = dynamic_cast<nbt::tags::byte_list_tag*>(list)) {
        byteList->value.clear();
    } else if (auto* shortList = dynamic_cast<nbt::tags::short_list_tag*>(list)) {
        shortList->value.clear();
    } else if (auto* intList = dynamic_cast<nbt::tags::int_list_tag*>(list)) {
        intList->value.clear();
    } else if (auto* longList = dynamic_cast<nbt::tags::long_list_tag*>(list)) {
        longList->value.clear();
    } else if (auto* floatList = dynamic_cast<nbt::tags::float_list_tag*>(list)) {
        floatList->value.clear();
    } else if (auto* doubleList = dynamic_cast<nbt::tags::double_list_tag*>(list)) {
        doubleList->value.clear();
    } else if (auto* byteArrayList = dynamic_cast<nbt::tags::bytearray_list_tag*>(list)) {
        byteArrayList->value.clear();
    } else if (auto* intArrayList = dynamic_cast<nbt::tags::intarray_list_tag*>(list)) {
        intArrayList->value.clear();
    } else if (auto* longArrayList = dynamic_cast<nbt::tags::longarray_list_tag*>(list)) {
        longArrayList->value.clear();
    } else {
        // TODO: 新增列表容器类型时需在此补充清空逻辑
        return 0;
    }

    return count;
}

std::vector<nbt::tags::tag*> NbtPathAllElementsNode::getOrCreate(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator, bool overwriteSame) const
{
    // 与 MC AllElementsNode.getOrCreateTag 一致：空列表插入创建出的值
    MC_UNUSED(overwriteSame);
    if (tag == nullptr || tag->id() != nbt::TagId::List || !creator) {
        return get(tag);
    }

    auto* list = dynamic_cast<nbt::tags::list_tag*>(tag);
    if (list == nullptr || list->size() != 0) {
        return get(tag);
    }

    auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(list);
    if (tagList == nullptr) {
        return get(tag);
    }

    auto value = creator();
    if (value == nullptr) {
        return {};
    }
    tagList->value.push_back(std::move(value));
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

    // 与 MC NbtUtils.compareNbt(filter, tag, true) 一致：过滤器是候选值的子集模式，
    // 复合标签按键子集匹配、列表按"每个模式元素都能找到匹配元素"匹配，其余标签按值相等
    return compareNbt(m_filter.get(), &tag);
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
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier, bool overwriteSame) const
{
    // 与 MC MatchRootObjectNode.setTag 一致：只有当前标签匹配过滤器时才替换整个标签；
    // 匹配根对象意味着路径会修改传入的复合标签本体，调用方拿到的并不是可替换的槽位
    MC_UNUSED(overwriteSame);
    if (tag == nullptr || tag->id() != nbt::TagId::Compound || valueSupplier == nullptr) {
        return 0;
    }

    if (!_matches(*static_cast<nbt::tags::compound_tag*>(tag))) {
        return 0;
    }

    if (auto value = valueSupplier()) {
        return value->equals(*tag) ? 0 : 1;
    }
    return 0;
}

i32 NbtPathCompoundFilterNode::remove(nbt::tags::tag* tag) const
{
    // 过滤器节点不支持删除
    MC_UNUSED(tag);
    return 0;
}

std::vector<nbt::tags::tag*> NbtPathCompoundFilterNode::getOrCreate(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator, bool overwriteSame) const
{
    MC_UNUSED(creator);
    MC_UNUSED(overwriteSame);
    return get(tag);
}

std::unique_ptr<nbt::tags::tag> NbtPathCompoundFilterNode::createPreferredParentTag() const
{
    // 与 MC MatchRootObjectNode.createPreferredParentTag 一致：中间缺失时按过滤器模式创建
    return m_filter ? m_filter->copy() : std::make_unique<nbt::tags::compound_tag>();
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
    // 与 MC NbtUtils.compareNbt(filter, tag, true) 一致（同 NbtPathCompoundFilterNode::_matches）
    return compareNbt(m_filter.get(), &tag);
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

    // 元素以裸值存储的列表不可能有复合元素，物化出的元素也不会进入结果集，
    // 因此这里的物化缓存使用局部变量即可。
    std::vector<std::unique_ptr<nbt::tags::tag>> materialized;
    for (size_t i = 0; i < list->size(); ++i) {
        nbt::tags::tag* element = elementAt(*list, i, materialized);
        if (element->id() == nbt::TagId::Compound) {
            auto* compound = dynamic_cast<nbt::tags::compound_tag*>(element);
            if (compound != nullptr && _matches(*compound)) {
                result.push_back(element);
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

    // 见非 const 版本：裸值列表的元素不会进入结果集，局部物化缓存即可
    std::vector<std::unique_ptr<nbt::tags::tag>> materialized;
    for (size_t i = 0; i < list->size(); ++i) {
        const nbt::tags::tag* element = elementAt(*list, i, materialized);
        if (element->id() == nbt::TagId::Compound) {
            const auto* compound = dynamic_cast<const nbt::tags::compound_tag*>(element);
            if (compound != nullptr && _matches(*compound)) {
                result.push_back(element);
            }
        }
    }

    return result;
}

i32 NbtPathListFilterNode::set(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier, bool overwriteSame) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::List || valueSupplier == nullptr) {
        return 0;
    }

    auto* list = dynamic_cast<nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return 0;
    }

    MC_UNUSED(overwriteSame);
    // 与 MC MatchElementNode.setTag 一致：空列表写入一个值；否则替换所有匹配元素，
    // 新旧值相等的不计入返回值
    const size_t size = list->size();
    if (size == 0) {
        auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(list);
        if (tagList == nullptr) {
            // 空列表按定义是 end_list_tag，不能容纳任何元素
            return 0;
        }
        if (auto value = valueSupplier()) {
            tagList->value.push_back(std::move(value));
            return 1;
        }
        return 0;
    }

    // 匹配到的元素需要原地替换：复合列表与嵌套列表的表层结构可以就地替换元素对象，
    // 裸值/字符串/数组列表不存在复合元素因而不会命中
    auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(list);
    if (tagList != nullptr) {
        i32 count = 0;
        for (auto& element : tagList->value) {
            if (element && element->id() == nbt::TagId::Compound &&
                _matches(*static_cast<const nbt::tags::compound_tag*>(element.get()))) {
                auto newValue = valueSupplier();
                if (newValue && !newValue->equals(*element)) {
                    element = std::move(newValue);
                    count++;
                }
            }
        }
        return count;
    }

    // 嵌套列表的元素是列表本身，不可能以复合标签形态被过滤器命中（同 get）

    if (auto* compoundList = dynamic_cast<nbt::tags::compound_list_tag*>(list)) {
        i32 count = 0;
        for (auto& element : compoundList->value) {
            if (!_matches(element)) {
                continue;
            }
            auto newValue = valueSupplier();
            auto* newCompound = newValue ? dynamic_cast<nbt::tags::compound_tag*>(newValue.get()) : nullptr;
            if (newCompound != nullptr && !newCompound->equals(element)) {
                element.value.clear();
                for (const auto& [key, value] : newCompound->value) {
                    element.value.emplace(key, value->copy());
                }
                count++;
            }
        }
        return count;
    }

    return 0;
}

i32 NbtPathListFilterNode::remove(nbt::tags::tag* tag) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return 0;
    }

    auto* list = dynamic_cast<nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return 0;
    }

    // 与 MC MatchElementNode.removeTag 一致：从后向前删除所有匹配元素
    if (auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(list)) {
        i32 count = 0;
        for (auto it = tagList->value.begin(); it != tagList->value.end();) {
            if (*it && (*it)->id() == nbt::TagId::Compound &&
                _matches(*static_cast<const nbt::tags::compound_tag*>(it->get()))) {
                it = tagList->value.erase(it);
                count++;
                continue;
            }
            ++it;
        }
        return count;
    }

    // 裸值/字符串/数组列表不存在复合元素，不可能匹配；复合列表可以按值直接删除
    if (auto* compoundList = dynamic_cast<nbt::tags::compound_list_tag*>(list)) {
        i32 count = 0;
        for (auto it = compoundList->value.begin(); it != compoundList->value.end();) {
            if (_matches(*it)) {
                it = compoundList->value.erase(it);
                count++;
                continue;
            }
            ++it;
        }
        return count;
    }

    // 嵌套列表的元素是列表本身，无法以复合标签形态被过滤器命中
    return 0;
}

std::vector<nbt::tags::tag*> NbtPathListFilterNode::getOrCreate(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator, bool overwriteSame) const
{
    MC_UNUSED(overwriteSame);
    if (tag == nullptr || tag->id() != nbt::TagId::List) {
        return get(tag);
    }

    auto* list = dynamic_cast<nbt::tags::list_tag*>(tag);
    if (list == nullptr) {
        return get(tag);
    }

    // 与 MC MatchElementNode.getOrCreateTag 一致：没有任何匹配元素时按过滤器模式追加一个
    auto matches = get(tag);
    if (!matches.empty()) {
        return matches;
    }

    auto* tagList = dynamic_cast<nbt::tags::tag_list_tag*>(list);
    auto pattern = m_filter
        ? std::unique_ptr<nbt::tags::compound_tag>(static_cast<nbt::tags::compound_tag*>(m_filter->copy().release()))
        : std::make_unique<nbt::tags::compound_tag>();
    if (tagList == nullptr) {
        // TODO: 复合列表（compound_list_tag）追加过滤器模式需要一个可复制的复合标签槽位，
        //   当前无法从已执行完的节点路径中取得父槽位，待 list_tag 增加 append 虚方法后补全
        MC_UNUSED(pattern);
        return {};
    }

    tagList->value.push_back(std::move(pattern));
    return get(tag);
}

std::unique_ptr<nbt::tags::tag> NbtPathListFilterNode::createPreferredParentTag() const
{
    return m_filter ? std::unique_ptr<nbt::tags::tag>(static_cast<nbt::tags::compound_tag*>(m_filter->copy().release()))
                    : std::make_unique<nbt::tags::compound_tag>();
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

    // 与 MC MatchObjectNode 一致：过滤器按子集语义比较键对应的值
    if (!compareNbt(m_filter.get(), it->second.get())) {
        return result;
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

    if (!compareNbt(m_filter.get(), it->second.get())) {
        return result;
    }

    result.push_back(it->second.get());
    return result;
}

i32 NbtPathKeyFilterNode::set(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> valueSupplier, bool overwriteSame) const
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

    // 与 MC MatchObjectNode.setTag 一致：只有现有值匹配过滤器时才写入，且新旧值相等不计入
    MC_UNUSED(overwriteSame);
    if (!compareNbt(m_filter.get(), it->second.get())) {
        return 0;
    }

    auto value = valueSupplier();
    if (value == nullptr || value->equals(*it->second)) {
        return 0;
    }

    it->second = std::move(value);
    return 1;
}

i32 NbtPathKeyFilterNode::remove(nbt::tags::tag* tag) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
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

    // 与 MC MatchObjectNode.removeTag 一致：现有值匹配过滤器时删除该键
    if (!compareNbt(m_filter.get(), it->second.get())) {
        return 0;
    }

    compound->value.erase(it);
    return 1;
}

std::vector<nbt::tags::tag*> NbtPathKeyFilterNode::getOrCreate(
    nbt::tags::tag* tag, std::function<std::unique_ptr<nbt::tags::tag>()> creator, bool overwriteSame) const
{
    MC_UNUSED(creator);
    MC_UNUSED(overwriteSame);
    return get(tag);
}

std::unique_ptr<nbt::tags::tag> NbtPathKeyFilterNode::createPreferredParentTag() const
{
    // 与 MC MatchObjectNode.createPreferredParentTag 一致：缺失时按过滤器模式创建
    return m_filter ? std::unique_ptr<nbt::tags::tag>(static_cast<nbt::tags::compound_tag*>(m_filter->copy().release()))
                    : std::make_unique<nbt::tags::compound_tag>();
}

bool NbtPathKeyFilterNode::heavyList(nbt::tags::tag* tag) const
{
    if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
        return false;
    }

    auto* compound = dynamic_cast<nbt::tags::compound_tag*>(tag);
    if (compound == nullptr) {
        return false;
    }

    auto it = compound->value.find(m_name);
    if (it == compound->value.end() || it->second == nullptr || it->second->id() != nbt::TagId::List) {
        return false;
    }

    auto& list = static_cast<nbt::tags::list_tag&>(*it->second);
    if (dynamic_cast<nbt::tags::tag_list_tag*>(&list) != nullptr) {
        return false;
    }

    heavyListSlot(it->second, list);
    return true;
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
