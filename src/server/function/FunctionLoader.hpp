#pragma once

#include "MacroFunction.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/resource/repository/PackRepository.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc {
namespace function {

class FunctionManager;

/**
 * @brief 函数加载器
 *
 * 从数据包加载 .mcfunction 文件和函数标签 JSON 文件，并注册到 FunctionManager。
 *
 * 函数文件路径映射遵循数据包规范：
 *   data/<namespace>/functions/<path>.mcfunction -> <namespace>:<path>
 *
 * 函数标签路径映射遵循数据包规范：
 *   data/<namespace>/tags/functions/<path>.json -> <namespace>:<path>
 *
 * 标签 JSON 格式：
 *   {
 *     "replace": false,
 *     "values": [
 *       "namespace:path",        // 直接引用函数
 *       "#namespace:tag"         // 引用另一个标签
 *     ]
 *   }
 *
 * 函数文件格式：
 *   - 每行一条命令（不含 / 前缀）
 *   - # 开头的行为注释，被忽略
 *   - \ 结尾的行与下一行连接
 *   - $ 开头的行为宏函数行（$(var) 占位符语法），整个文件若有至少一个宏行
 *     将被注册为 MacroFunction，否则注册为 CommandFunction
 *   - 空行被忽略
 *   - 命令长度上限为 2,000,000 字符
 */
class FunctionLoader {
public:
    /**
     * @brief 加载结果
     */
    struct LoadResult {
        Size successCount = 0;
        Size failedCount = 0;
        Size skippedCount = 0;       ///< 跳过的宏函数行数（保留字段，新实现不再跳过宏行而是注册为 MacroFunction）
        Size tagCount = 0;           ///< 加载的函数标签数量
        Size macroFunctionCount = 0; ///< 注册为 MacroFunction 的函数数量
        std::vector<std::string> errors;
    };

    /**
     * @brief 解析结果
     *
     * 解析 .mcfunction 文件后产生的中间结果。
     * 若 macroEntries 非空，则应注册为 MacroFunction；否则注册为 CommandFunction。
     */
    struct ParseResult {
        /// 普通命令列表（仅当 macroEntries 为空时使用，即文件不含 $ 宏行）
        std::vector<std::string> commands;

        /// 宏函数条目列表（仅当文件含 $ 宏行时填充）
        /// 此时 commands 留空，所有普通命令已被转成 PlainText entry 放入 macroEntries
        std::vector<MacroFunctionEntry> macroEntries;

        /// 形参名列表（仅当 macroEntries 非空时有效，按首次出现顺序去重）
        std::vector<std::string> macroParameters;

        /// 是否为宏函数（macroEntries 非空）
        [[nodiscard]] bool isMacro() const noexcept { return !macroEntries.empty(); }
    };

    /**
     * @brief 标签条目类型
     */
    enum class TagEntryType : u8 {
        Function, ///< 直接引用函数
        Tag,      ///< 引用另一个标签（# 前缀）
    };

    /**
     * @brief 标签条目
     *
     * 表示 values 数组中的一个条目，可以是直接函数引用或标签引用。
     * 对应 MC Java 的 TagEntry，支持 required 语义：
     * - required=true（默认）：引用的目标必须存在，不存在时标签构建失败
     * - required=false：引用的目标不存在时静默跳过
     */
    struct TagEntry {
        ResourceLocation id;  ///< 条目 ID
        TagEntryType type;    ///< 条目类型（函数或标签引用）
        bool required = true; ///< 是否必须存在（默认 true）

        /**
         * @brief 便捷构造：函数引用条目
         */
        static TagEntry functionEntry(ResourceLocation id, bool required = true)
        {
            return {std::move(id), TagEntryType::Function, required};
        }

        /**
         * @brief 便捷构造：标签引用条目
         */
        static TagEntry tagEntry(ResourceLocation id, bool required = true)
        {
            return {std::move(id), TagEntryType::Tag, required};
        }
    };

    /**
     * @brief 标签解析结果
     *
     * 从 JSON 文件解析出的函数标签数据，包含标签 ID、
     * 条目列表和替换标志。
     */
    struct TagData {
        ResourceLocation id;           ///< 标签 ID
        bool replace = false;          ///< 是否替换已有标签内容
        std::vector<TagEntry> entries; ///< 条目列表（函数引用和标签引用统一存储）
    };

    /**
     * @brief 加载进度回调
     * @param current 当前已处理文件数
     * @param total 总文件数
     * @param currentId 当前正在处理的函数ID
     */
    using ProgressCallback = std::function<void(Size current, Size total, const std::string& currentId)>;

    /**
     * @brief 构造加载器
     * @param manager 函数管理器引用
     */
    explicit FunctionLoader(FunctionManager& manager);

    /**
     * @brief 从数据包列表加载所有函数和函数标签
     *
     * 使用 DataPackRepository 的 PackType::ServerData 限定接口从数据包加载函数和标签。
     * 按数据包优先级从低到高加载，同名函数由高优先级数据包覆盖。
     * 标签按 MC Java 语义合并：默认追加，replace=true 时替换之前数据包的条目。
     *
     * @param dataPacks 数据包列表
     * @param callback 进度回调（可选）
     * @return 加载结果
     */
    Result<LoadResult> loadFromDataPackRepository(
        const mc::resource::DataPackRepository& dataPacks, ProgressCallback callback = nullptr);

    /**
     * @brief 设置加载前是否清空已有函数
     * @param clear 是否清空（默认为 true）
     */
    void setClearBeforeLoad(bool clear) { m_clearBeforeLoad = clear; }

    /**
     * @brief 将文件路径转换为函数 ID
     *
     * 遵循 MC 数据包规范：
     *   data/minecraft/functions/foo/bar.mcfunction -> minecraft:foo/bar
     *   data/mod_id/functions/test.mcfunction -> mod_id:test
     *
     * @param filePath 文件路径
     * @return 函数 ID
     */
    [[nodiscard]] std::string pathToFunctionId(const std::string& filePath) const;

    /**
     * @brief 将标签文件路径转换为标签 ID
     *
     * 遵循 MC 数据包规范：
     *   data/minecraft/tags/functions/tick.json -> minecraft:tick
     *   data/mod_id/tags/functions/foo/bar.json -> mod_id:foo/bar
     *
     * @param filePath 文件路径
     * @return 标签 ID
     */
    [[nodiscard]] std::string pathToTagId(const std::string& filePath) const;

    /**
     * @brief 解析 .mcfunction 文件内容
     *
     * 公开接口，便于单元测试直接调用。
     *
     * 解析规则：
     * - 普通行（非 $ 开头）转为 PlainText entry 或 commands 列表（视是否含宏行而定）
     * - $ 开头的行（去掉 $ 后）用 StringTemplate::fromString 解析为 Macro entry
     * - 文件含至少一个宏行时，整体注册为 MacroFunction；否则为 CommandFunction
     *
     * @param id 函数 ID（用于错误消息）
     * @param content 文件内容
     * @return 解析结果（成功）或错误
     */
    Result<ParseResult> parseFunctionContent(const std::string& id, const std::string& content);

    /**
     * @brief 解析函数标签 JSON 文件内容
     *
     * 公开接口，便于单元测试直接调用。
     *
     * @param tagId 标签 ID
     * @param jsonContent JSON 文件内容
     * @return 标签解析结果（成功）或错误
     */
    Result<TagData> parseTagJson(const ResourceLocation& tagId, const std::string& jsonContent);

private:
    FunctionManager& m_manager;
    bool m_clearBeforeLoad = true;

    /**
     * @brief 从数据包加载函数标签
     *
     * 遍历所有命名空间下的 tags/functions/ 目录，加载标签 JSON 文件。
     * 处理多数据包标签合并（replace/append 语义）和标签引用（# 前缀）。
     *
     * @param dataPacks 数据包列表
     * @param result 加载结果（用于累加错误信息）
     * @return 加载的标签数量
     */
    Size loadFunctionTags(const mc::resource::DataPackRepository& dataPacks, LoadResult& result);

    /**
     * @brief 解析标签值条目
     *
     * @param entry 值条目字符串（如 "minecraft:foo" 或 "#minecraft:bar"）
     * @param required 是否必须存在
     * @param[out] entries 输出条目列表
     */
    static void resolveTagEntry(const std::string& entry, bool required, std::vector<TagEntry>& entries);
};

} // namespace function
} // namespace mc
