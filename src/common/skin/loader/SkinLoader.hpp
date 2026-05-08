#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include <vector>
#include <functional>

namespace mc::skin {

/**
 * @brief 皮肤加载结果
 */
struct SkinLoadResult {
    std::vector<u8> pngData;    // PNG 数据
    SkinTextures textures;       // 解析出的纹理信息
    std::string hash;                 // 文件哈希（用于缓存）
};

/**
 * @brief 皮肤加载器接口
 *
 * 定义皮肤加载的统一接口，支持从不同来源加载皮肤：
 * - 本地文件
 * - HTTP 下载
 * - 资源包
 *
 * 所有加载操作是异步的，通过回调返回结果。
 */
class ISkinLoader {
public:
    virtual ~ISkinLoader() = default;

    /**
     * @brief 初始化加载器
     * @return 成功或错误
     */
    virtual Result<void> initialize() = 0;

    /**
     * @brief 关闭加载器
     */
    virtual void shutdown() = 0;

    /**
     * @brief 检查是否支持指定 URL
     * @param url 皮肤 URL
     * @return 是否支持
     */
    [[nodiscard]] virtual bool supportsUrl(const std::string& url) const = 0;

    /**
     * @brief 同步加载皮肤
     * @param url 皮肤 URL 或路径
     * @return 加载结果
     */
    virtual Result<SkinLoadResult> load(const std::string& url) = 0;

    /**
     * @brief 异步加载皮肤
     * @param url 皮肤 URL 或路径
     * @param callback 完成回调
     */
    virtual void loadAsync(const std::string& url,
                          std::function<void(Result<SkinLoadResult>)> callback) = 0;

    /**
     * @brief 取消加载
     * @param url 要取消的 URL
     */
    virtual void cancel(const std::string& url) = 0;

    /**
     * @brief 取消所有加载
     */
    virtual void cancelAll() = 0;

    /**
     * @brief 获取加载器名称
     */
    [[nodiscard]] virtual std::string name() const = 0;
};

} // namespace mc::skin
