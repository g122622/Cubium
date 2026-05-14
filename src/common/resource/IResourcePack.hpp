#pragma once

#include "../core/Result.hpp"
#include "../core/Types.hpp"
#include "PackMetadata.hpp"
#include <memory>
#include <vector>

namespace mc {

/**
 * @brief 资源包抽象接口
 *
 * 提供统一的资源读取接口，支持文件夹和ZIP资源包
 */
class IResourcePack {
public:
    virtual ~IResourcePack() = default;

    // 初始化资源包
    [[nodiscard]] virtual Result<void> initialize() = 0;

    // 获取元数据
    [[nodiscard]] virtual const PackMetadata& metadata() const = 0;

    // 检查资源是否存在
    [[nodiscard]] virtual bool hasResource(std::string_view resourcePath) const = 0;

    // 读取资源内容
    [[nodiscard]] virtual Result<std::vector<u8>> readResource(std::string_view resourcePath) const = 0;

    // 读取文本资源
    [[nodiscard]] virtual Result<std::string> readTextResource(std::string_view resourcePath) const;

    // 列出目录下的所有资源
    [[nodiscard]] virtual Result<std::vector<std::string>> listResources(
        std::string_view directory, std::string_view extension = "") const = 0;

    // 获取资源包路径/名称
    [[nodiscard]] virtual std::string name() const = 0;
};

using ResourcePackPtr = std::shared_ptr<IResourcePack>;

} // namespace mc
