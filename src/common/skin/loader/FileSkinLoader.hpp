#pragma once

#include "SkinLoader.hpp"
#include "common/resource/IResourcePack.hpp"
#include <filesystem>

namespace mc::skin {

/**
 * @brief 本地文件皮肤加载器
 *
 * 从本地文件系统或资源包加载皮肤。
 *
 * 支持的路径格式：
 * - 绝对路径：/path/to/skin.png
 * - 相对路径：skins/player.png
 * - 资源位置：minecraft:textures/entity/steve.png
 */
class FileSkinLoader : public ISkinLoader {
public:
    /**
     * @brief 构造文件加载器
     * @param resourcePack 资源包（可选，用于加载内置皮肤）
     */
    explicit FileSkinLoader(IResourcePack* resourcePack = nullptr);

    ~FileSkinLoader() override = default;

    Result<void> initialize() override;
    void shutdown() override;

    [[nodiscard]] bool supportsUrl(const std::string& url) const override;
    Result<SkinLoadResult> load(const std::string& url) override;
    void loadAsync(const std::string& url, std::function<void(Result<SkinLoadResult>)> callback) override;
    void cancel(const std::string& url) override;
    void cancelAll() override;

    [[nodiscard]] std::string name() const override { return "FileSkinLoader"; }

private:
    /**
     * @brief 从文件系统加载
     */
    Result<SkinLoadResult> loadFromFilesystem(const std::string& path);

    /**
     * @brief 从资源包加载
     */
    Result<SkinLoadResult> loadFromResourcePack(const ResourceLocation& location);

    /**
     * @brief 验证皮肤 PNG 数据
     *
     * 检查是否为有效的 64x64 或 64x32 PNG。
     * 如果是 64x32，自动转换为 64x64。
     */
    Result<std::vector<u8>> validateAndConvertSkin(const std::vector<u8>& pngData);

    /**
     * @brief 计算 SHA1 哈希
     */
    std::string calculateHash(const std::vector<u8>& data);

    IResourcePack* m_resourcePack = nullptr;
    bool m_initialized = false;
};

} // namespace mc::skin
