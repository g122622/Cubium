#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/resource/repository/PackRepository.hpp"
#include <functional>
#include <string>
#include <vector>

namespace mc {
namespace function {

class FunctionManager;

/**
 * @brief 函数加载器
 *
 * 从数据包加载 .mcfunction 文件并注册到 FunctionManager。
 * 路径映射遵循数据包规范：
 *   data/<namespace>/functions/<path>.mcfunction -> <namespace>:<path>
 *
 * 函数文件格式：
 *   - 每行一条命令（不含 / 前缀）
 *   - # 开头的行为注释，被忽略
 *   - \ 结尾的行与下一行连接
 *   - $ 开头的行为宏函数行，当前版本跳过并记录警告
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
        Size skippedCount = 0; ///< 跳过的宏函数行数
        std::vector<std::string> errors;
    };

    /**
     * @brief 解析结果
     */
    struct ParseResult {
        std::vector<std::string> commands; ///< 解析出的命令列表
        Size skippedMacroCount = 0;        ///< 跳过的宏函数行数（$ 开头的行）
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
     * @brief 从数据包列表加载所有函数
     *
     * 使用 DataPackRepository 的 PackType::ServerData 限定接口从数据包加载函数。
     * 按数据包优先级从低到高加载，同名函数由高优先级数据包覆盖。
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
     * @brief 解析 .mcfunction 文件内容
     *
     * 公开接口，便于单元测试直接调用。
     *
     * @param id 函数 ID
     * @param content 文件内容
     * @return 解析结果（成功）或错误
     */
    Result<ParseResult> parseFunctionContent(const std::string& id, const std::string& content);

private:
    FunctionManager& m_manager;
    bool m_clearBeforeLoad = true;
};

} // namespace function
} // namespace mc
