#pragma once

#include "CommandFunction.hpp"
#include "IFunction.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {
namespace command {
class ServerCommandSource;
}

namespace function {

// 前向声明
class MacroFunction;

/**
 * @brief 函数管理器
 *
 * 管理 .mcfunction 文件的注册、查找和执行。
 *
 * 职责：
 * 1. 存储已注册的 IFunction（CommandFunction 和 MacroFunction，按 ResourceLocation 索引）
 * 2. 存储函数标签（按 ResourceLocation 索引的函数 ID 列表）
 * 3. 提供函数查找接口（按 ID 和标签）
 * 4. 提供函数执行接口（通过 IFunction::execute → CommandRegistry 逐行执行）
 * 5. 管理 tick 和 load 函数标签的每 tick 执行
 * 6. 支持函数递归调用深度限制（防止无限递归）
 */
class FunctionManager {
public:
    /// 最大递归调用深度
    static constexpr Size MAX_CALL_DEPTH = 100;

    /**
     * @brief 函数执行结果
     *
     * @note 等价于 FunctionExecuteResult，保留此 typedef 仅为向后兼容。
     *       外部代码（PlayerAdvancements、MinecraftServer、FunctionCommand 等）已大量使用
     *       FunctionManager::ExecuteResult，重命名会引发不必要的连锁修改。
     */
    using ExecuteResult = FunctionExecuteResult;

    FunctionManager() = default;
    ~FunctionManager() = default;

    FunctionManager(const FunctionManager&) = delete;
    FunctionManager& operator=(const FunctionManager&) = delete;
    FunctionManager(FunctionManager&&) = default;
    FunctionManager& operator=(FunctionManager&&) = default;

    // ========== 函数注册 ==========

    /**
     * @brief 注册普通函数（命令列表形式）
     * @param id 函数 ID
     * @param commands 命令列表
     */
    void registerFunction(const ResourceLocation& id, std::vector<std::string> commands);

    /**
     * @brief 注册宏函数
     *
     * 在 FunctionLoader 解析 .mcfunction 文件时，若文件包含 $ 宏行，
     * 调用此接口注册 MacroFunction 而非 CommandFunction。
     *
     * @param id 函数 ID
     * @param function 宏函数对象（已构建 entries 与 parameters）
     */
    void registerMacroFunction(const ResourceLocation& id, std::unique_ptr<MacroFunction> function);

    /**
     * @brief 注册函数标签
     * @param tagId 标签 ID（如 minecraft:tick）
     * @param functionIds 标签包含的函数 ID 列表
     */
    void registerTag(const ResourceLocation& tagId, std::vector<ResourceLocation> functionIds);

    /**
     * @brief 清空所有已注册的函数和标签
     */
    void clear();

    // ========== 函数查找 ==========

    /**
     * @brief 按函数 ID 查找函数对象（统一接口）
     *
     * 返回的 IFunction 指针可用于判断 isMacro() 或调用 execute。
     *
     * @param id 函数 ID
     * @return 函数指针（未找到返回 nullptr）
     */
    [[nodiscard]] const IFunction* getFunctionAny(const ResourceLocation& id) const;

    /**
     * @brief 按函数 ID 查找普通函数
     *
     * 仅当函数是 CommandFunction（非宏函数）时返回非空指针。
     * 历史接口，保留是为了 FunctionCommand 等仍能区分类型；新代码应优先使用 getFunctionAny。
     *
     * @param id 函数 ID
     * @return 函数指针（未找到或为宏函数时返回 nullptr）
     */
    [[nodiscard]] const CommandFunction* getFunction(const ResourceLocation& id) const;

    /**
     * @brief 按标签查找函数列表
     * @param tagId 标签 ID
     * @return 函数 ID 列表（未找到返回空列表）
     */
    [[nodiscard]] const std::vector<ResourceLocation>& getTag(const ResourceLocation& tagId) const;

    /**
     * @brief 检查函数是否存在（含普通函数和宏函数）
     */
    [[nodiscard]] bool hasFunction(const ResourceLocation& id) const;

    /**
     * @brief 检查标签是否存在
     */
    [[nodiscard]] bool hasTag(const ResourceLocation& tagId) const;

    /**
     * @brief 获取所有已注册函数的 ID
     */
    [[nodiscard]] std::vector<ResourceLocation> getAllFunctionIds() const;

    /**
     * @brief 获取所有已注册标签的 ID
     */
    [[nodiscard]] std::vector<ResourceLocation> getAllTagIds() const;

    /**
     * @brief 获取已注册函数数量
     */
    [[nodiscard]] Size functionCount() const noexcept { return m_functions.size(); }

    /**
     * @brief 获取已注册标签数量
     */
    [[nodiscard]] Size tagCount() const noexcept { return m_tags.size(); }

    // ========== 函数执行 ==========

    /**
     * @brief 执行函数（无参数）
     *
     * 普通函数直接逐行执行；宏函数将抛 FunctionInstantiationException（缺参数），
     * 失败计入 failureCount 而非抛出异常。
     *
     * @param id 函数 ID
     * @param source 命令源
     * @return 执行结果
     */
    ExecuteResult execute(const ResourceLocation& id, command::ServerCommandSource& source);

    /**
     * @brief 执行函数（带参数）
     *
     * 用于宏函数的 /function <name> <arguments> 形式。
     * 普通函数忽略 arguments；宏函数用 arguments 实例化 $(var) 占位符。
     *
     * @param id 函数 ID
     * @param source 命令源
     * @param arguments 实参 CompoundTag（可为 nullptr）
     * @return 执行结果
     */
    ExecuteResult execute(
        const ResourceLocation& id, command::ServerCommandSource& source, const nbt::tags::compound_tag* arguments);

    /**
     * @brief 执行已加载的 CommandFunction 对象
     *
     * 历史接口，新代码应使用 execute(id, source, arguments) 或 IFunction::execute。
     */
    ExecuteResult execute(const CommandFunction& function, command::ServerCommandSource& source);

    // ========== Tick / Load 标签 ==========

    /**
     * @brief 每个服务器 tick 调用
     *
     * 执行 minecraft:tick 标签中的函数。
     * 在首次重载后执行 minecraft:load 标签中的函数。
     *
     * @param source 命令源（使用 gamemaster 权限）
     */
    void tick(command::ServerCommandSource& source);

    /**
     * @brief 通知函数库已重载
     *
     * 下一次 tick() 时将执行 minecraft:load 标签中的函数。
     */
    void notifyReload();

private:
    /**
     * @brief 执行标签中的所有函数
     */
    void executeTagFunctions(const ResourceLocation& tagId, command::ServerCommandSource& source);

    /// 存储函数（按 ResourceLocation 索引，CommandFunction 和 MacroFunction 都通过 IFunction 多态管理）
    std::unordered_map<ResourceLocation, std::unique_ptr<IFunction>> m_functions;

    /// 存储函数标签（标签 ID -> 函数 ID 列表）
    std::unordered_map<ResourceLocation, std::vector<ResourceLocation>> m_tags;

    /// 预定义的标签常量
    static const ResourceLocation TICK_TAG;
    static const ResourceLocation LOAD_TAG;

    /// 是否需要在下次 tick 时执行 load 函数
    bool m_postReload = false;

    /// 空标签列表（用于 getTag 找不到标签时返回）
    static const std::vector<ResourceLocation> EMPTY_TAG;
};

} // namespace function
} // namespace mc
