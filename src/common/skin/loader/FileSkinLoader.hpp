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
    Result<SkinLoadResult> _loadFromFilesystem(const std::string& path);

    /**
     * @brief 从资源包加载
     */
    Result<SkinLoadResult> _loadFromResourcePack(const ResourceLocation& location);

    /**
     * @brief 验证皮肤 PNG 数据
     *
     * 检查是否为有效的 64x64 或 64x32 PNG。
     * 如果是 64x32，自动转换为 64x64。
     */
    Result<std::vector<u8>> _validateAndConvertSkin(const std::vector<u8>& pngData);

    /**
     * @brief 计算数据哈希值
     */
    std::string _calculateHash(const std::vector<u8>& data);

    IResourcePack* m_resourcePack = nullptr;
    bool m_initialized = false;
};

} // namespace mc::skin
