/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
 *
 * 异步加载通过注入的 UniversalWorkerPool 实现，回调在 worker 线程触发。
 * 若未注入线程池，loadAsync 降级为同步执行后立即回调。
 */
class FileSkinLoader : public ISkinLoader {
public:
    /**
     * @brief 构造文件加载器
     *
     * 资源包列表通过 setResourcePacks 注入（用于加载内置皮肤）。
     */
    FileSkinLoader();

    ~FileSkinLoader() override;

    Result<void> initialize() override;
    void shutdown() override;

    [[nodiscard]] bool supportsUrl(const std::string& url) const override;
    Result<SkinLoadResult> load(const std::string& url) override;
    void loadAsync(const std::string& url, std::function<void(Result<SkinLoadResult>)> callback) override;
    void cancel(const std::string& url) override;
    void cancelAll() override;

    [[nodiscard]] std::string name() const override { return "FileSkinLoader"; }

    /**
     * @brief 注入工作线程池用于异步加载
     *
     * 线程池由调用方拥有，必须保证生命周期长于本加载器（或在 shutdown 后释放）。
     * 传入 nullptr 切换回同步降级模式。
     *
     * @param workerPool 工作线程池指针（非所有权）
     */
    void setWorkerPool(util::UniversalWorkerPool* workerPool) { m_workerPool = workerPool; }

    /**
     * @brief 设置资源包列表（用于从资源包加载皮肤）
     *
     * 资源包由调用方拥有，必须保证生命周期长于本加载器。
     * 列表顺序为添加顺序（低→高优先级），查找时反向遍历（后添加的优先），
     * 与 ResourceManager 的纹理加载惯例一致。
     *
     * @param resourcePacks 资源包指针列表（非所有权）
     */
    void setResourcePacks(std::vector<IResourcePack*> resourcePacks) { m_resourcePacks = std::move(resourcePacks); }

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

    /**
     * @brief 在途任务计数增加（loadAsync 提交时调用）
     */
    void _incrementPending();

    /**
     * @brief 在途任务计数减少（回调完成时调用），通知 shutdown 等待者
     */
    void _decrementPending();

    std::vector<IResourcePack*> m_resourcePacks;
    bool m_initialized = false;

    // 异步加载基础设施
    util::UniversalWorkerPool* m_workerPool = nullptr;

    // 在途任务管理：url → 取消信号
    std::mutex m_pendingMutex;
    std::unordered_map<std::string, std::shared_ptr<std::atomic<bool>>> m_pendingLoads;

    // shutdown 同步：等待所有在途回调完成
    std::atomic<size_t> m_pendingCount{0};
    std::mutex m_shutdownMutex;
    std::condition_variable m_shutdownCondition;
};

} // namespace mc::skin
