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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "IFunction.hpp"
#include "StringTemplate.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace mc {
namespace function {

class FunctionManager;

/**
 * @brief 宏函数条目类型
 *
 * 对应 MC 1.21.11 MacroFunction.Entry 接口的两个具体实现：
 * - MacroEntry: 含 $(var) 占位符的宏行，需用实参实例化
 * - PlainTextEntry: 纯文本命令行，无需替换
 *
 * 实例化时：
 * - MacroEntry 用 StringTemplate::substitute(values) 替换占位符得到命令字符串
 * - PlainTextEntry 直接返回预存的命令字符串
 */
struct MacroFunctionEntry {
    /// 宏行：StringTemplate + 形参索引列表（指向 MacroFunction::parameters()）
    struct Macro {
        StringTemplate template_;
        std::vector<Size> parameterIndices;
    };

    /// 纯文本行：预存的命令字符串
    struct PlainText {
        std::string command;
    };

    std::variant<Macro, PlainText> data;

    /** @brief 构造宏行条目 */
    static MacroFunctionEntry macro(StringTemplate tmpl, std::vector<Size> indices)
    {
        return MacroFunctionEntry{Macro{std::move(tmpl), std::move(indices)}};
    }

    /** @brief 构造纯文本行条目 */
    static MacroFunctionEntry plainText(std::string command)
    {
        return MacroFunctionEntry{PlainText{std::move(command)}};
    }

    /** @brief 是否为宏行 */
    [[nodiscard]] bool isMacro() const noexcept { return std::holds_alternative<Macro>(data); }

    /**
     * @brief 用实参值列表实例化条目
     *
     * 对宏行：用 values 替换占位符得到命令字符串
     * 对纯文本行：直接返回预存命令
     *
     * @param values 实参值列表（按 MacroFunction::parameters() 顺序）
     * @return 命令字符串
     * @throws std::runtime_error 如果替换后超过命令长度上限
     */
    [[nodiscard]] std::string instantiate(const std::vector<std::string>& values) const;
};

/**
 * @brief 宏函数 - 支持 $(var) 占位符替换的函数
 *
 * 对应 MC 1.21.11 的 net.minecraft.commands.functions.MacroFunction。
 *
 * 与 CommandFunction（普通函数）的区别：
 * - 含有以 `$` 开头的行（宏行），宏行中使用 $(var) 引用形参
 * - 执行时需要 CompoundTag 实参，将 $(var) 替换为 NBT 值字符串后再解析命令
 * - 普通行（非 `$` 开头）与宏行混合存储在 entries 中
 *
 * 实例化流程（对应 MacroFunction#instantiate）：
 * 1. 按 parameters 顺序从 CompoundTag 中取出每个形参对应的 NBT tag
 * 2. 将 tag stringify 为字符串（不同类型规则不同）
 * 3. 用字符串列表作为 cache key 查 LRU 缓存，命中则直接返回
 * 4. 未命中则遍历 entries，对每个 MacroEntry 用 StringTemplate::substitute 替换占位符
 *    得到命令字符串；PlainTextEntry 直接返回预存命令
 * 5. 缓存最多保留 MAX_CACHE_ENTRIES 条，超出时淘汰最旧
 *
 * stringify 规则（对应 MacroFunction#stringify）：
 * - FloatTag → "%.15g" 格式化（去尾零）
 * - DoubleTag → "%.15g" 格式化（去尾零）
 * - ByteTag → (int)value 的十进制字符串
 * - ShortTag → (int)value 的十进制字符串
 * - LongTag → value 的十进制字符串
 * - StringTag → 原始字符串值
 * - 其他（Int/Compound/List/ByteArray/IntArray/LongArray）→ SNBT 文本（std::to_string(tag)）
 *
 * LRU 缓存（对应 MacroFunction#cache）：
 * - 使用 std::list + unordered_map 实现 O(1) LRU
 * - 键为 stringified values 列表（与 parameters 一一对应）
 * - 上限 MAX_CACHE_ENTRIES = 8
 */
class MacroFunction : public IFunction {
public:
    using Entry = MacroFunctionEntry;

    /**
     * @brief 构造宏函数
     * @param id 函数 ID
     * @param entries 条目列表（MacroEntry 与 PlainTextEntry 混合）
     * @param parameters 形参名列表（已去重，按首次出现顺序）
     */
    MacroFunction(ResourceLocation id, std::vector<Entry> entries, std::vector<std::string> parameters);

    ~MacroFunction() override = default;

    MacroFunction(const MacroFunction&) = delete;
    MacroFunction& operator=(const MacroFunction&) = delete;
    MacroFunction(MacroFunction&&) = default;
    MacroFunction& operator=(MacroFunction&&) = default;

    // ========== IFunction 接口实现 ==========

    [[nodiscard]] const ResourceLocation& id() const noexcept override { return m_id; }

    /**
     * @brief 获取条目数量（宏行 + 纯文本行）
     */
    [[nodiscard]] Size commandCount() const noexcept override { return m_entries.size(); }

    [[nodiscard]] bool isEmpty() const noexcept override { return m_entries.empty(); }

    [[nodiscard]] bool isMacro() const noexcept override { return true; }

    /**
     * @brief 执行宏函数
     *
     * 1. 用 arguments 实例化得到一组命令字符串
     * 2. 逐行通过 CommandRegistry 执行
     *
     * @param manager 函数管理器
     * @param source 命令源
     * @param arguments 实参 CompoundTag（为 nullptr 时抛 FunctionInstantiationException）
     * @return 执行结果
     */
    [[nodiscard]] FunctionExecuteResult execute(FunctionManager& manager,
        command::ServerCommandSource& source,
        const nbt::tags::compound_tag* arguments) const override;

    // ========== 公开工具方法 ==========

    /** @brief 形参名列表（已去重） */
    [[nodiscard]] const std::vector<std::string>& parameters() const noexcept { return m_parameters; }

    /** @brief 条目列表 */
    [[nodiscard]] const std::vector<Entry>& entries() const noexcept { return m_entries; }

    /**
     * @brief 将 NBT 标签转换为宏参数字符串
     *
     * 对应 MC 1.21.11 MacroFunction#stringify。
     * 不同标签类型的转换规则见类注释。
     *
     * @param tag NBT 标签
     * @return 字符串化的值
     */
    [[nodiscard]] static std::string stringify(const nbt::tags::tag& tag);

    /**
     * @brief 实例化宏函数
     *
     * 用 arguments 替换所有 $(var) 占位符，生成命令字符串列表。
     * 使用 LRU 缓存避免相同参数重复解析。
     *
     * @param arguments 实参 CompoundTag（为 nullptr 时抛异常）
     * @return 实例化后的命令字符串列表
     * @throws FunctionInstantiationException 缺少参数或替换后超长
     */
    [[nodiscard]] std::vector<std::string> instantiate(const nbt::tags::compound_tag* arguments) const;

private:
    /// LRU 缓存上限（与 MC 1.21.11 一致）
    static constexpr Size MAX_CACHE_ENTRIES = 8;

    ResourceLocation m_id;
    std::vector<Entry> m_entries;
    std::vector<std::string> m_parameters;

    /**
     * @brief LRU 缓存
     *
     * cache 链表头为最旧，尾为最新（与 MC 的 Object2ObjectLinkedOpenHashMap 语义一致）。
     * 键为 stringify 后的实参值列表，值为已实例化的命令字符串列表。
     */
    struct CacheEntry {
        std::vector<std::string> key;
        std::vector<std::string> commands;
    };
    mutable std::list<CacheEntry> m_cacheList;
    mutable std::unordered_map<std::string, std::list<CacheEntry>::iterator> m_cacheMap;

    /**
     * @brief 查找缓存（命中时移到链表尾部）
     * @param key stringify 后的实参值列表
     * @return 命中则返回命令列表指针，未命中返回 nullptr
     */
    [[nodiscard]] const std::vector<std::string>* _cacheLookup(const std::vector<std::string>& key) const;

    /**
     * @brief 插入缓存条目，超出上限时淘汰最旧
     */
    void _cacheInsert(std::vector<std::string> key, std::vector<std::string> commands) const;

    /**
     * @brief 计算 key 的字符串表示（用于 unordered_map 查找）
     */
    [[nodiscard]] static std::string _cacheKeyToString(const std::vector<std::string>& key);
};

} // namespace function
} // namespace mc
