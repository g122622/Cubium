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

#include "client/sound/resource/SoundDefinition.hpp"
#include "client/sound/resource/SoundRegistry.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/PackRepository.hpp"
#include "common/sound/SoundTypes.hpp"
#include "common/util/math/random/Random.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc::client::sound {

// 从 mc::sound 引入类型
using ::mc::sound::AttenuationType;
using ::mc::sound::DEFAULT_ATTENUATION_DISTANCE;
using ::mc::sound::SoundInstanceId;

/**
 * @brief 声音资源加载进度回调
 */
struct SoundLoadProgress {
    size_t totalPacks = 0;       ///< 总资源包数量
    size_t currentPack = 0;      ///< 当前处理的资源包索引
    std::string currentPackName; ///< 当前资源包名称
    size_t totalEvents = 0;      ///< 总声音事件数量
    size_t loadedEvents = 0;     ///< 已加载的声音事件数量
};

/**
 * @brief 声音资源加载器
 *
 * 负责加载和解析 sounds.json 文件，管理声音事件定义。
 * 支持从多个资源包加载声音定义，后加载的资源包可以覆盖或追加声音。
 *
 * 加载流程：
 * 1. 遍历所有启用的资源包
 * 2. 查找每个命名空间下的 sounds.json
 * 3. 解析 JSON 并注册声音事件
 * 4. 合并/覆盖同名声音事件
 *
 * 使用示例:
 * @code
 * PackRepository packs;
 * // ... 添加资源包 ...
 *
 * SoundHandler handler(packs);
 * handler.setProgressCallback([](const SoundLoadProgress& progress) {
 *     spdlog::info("Loading sounds: {}/{} packs, {}/{} events",
 *         progress.currentPack, progress.totalPacks,
 *         progress.loadedEvents, progress.totalEvents);
 * });
 *
 * auto result = handler.reload();
 * if (!result.success()) {
 *     spdlog::error("Failed to load sounds: {}", result.error().message);
 * }
 *
 * // 获取声音定义
 * const SoundEventDefinition* def = handler.getSoundEvent(
 *     ResourceLocation("minecraft:block.stone.break")
 * );
 * @endcode
 */
class SoundHandler {
public:
    /**
     * @brief 进度回调类型
     */
    using ProgressCallback = std::function<void(const SoundLoadProgress&)>;

    /**
     * @brief 构造声音加载器
     *
     * @param resourcePacks 资源包列表引用
     */
    explicit SoundHandler(PackRepository& resourcePacks);

    /**
     * @brief 析构函数
     */
    ~SoundHandler() = default;

    // 禁止拷贝
    SoundHandler(const SoundHandler&) = delete;
    SoundHandler& operator=(const SoundHandler&) = delete;

    // 允许移动
    SoundHandler(SoundHandler&&) noexcept = default;
    SoundHandler& operator=(SoundHandler&&) noexcept = default;

    // ========================================================================
    // 资源加载
    // ========================================================================

    /**
     * @brief 重载声音资源
     *
     * 遍历所有启用的资源包，加载所有命名空间下的 sounds.json。
     * 后加载的资源包可以覆盖或追加同名声音事件。
     *
     * 加载顺序：
     * 1. 按优先级从低到高遍历资源包
     * 2. 对每个资源包，遍历所有命名空间
     * 3. 加载 assets/<namespace>/sounds.json
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> reload();

    /**
     * @brief 清空所有加载的声音
     */
    void clear();

    // ========================================================================
    // 声音事件访问
    // ========================================================================

    /**
     * @brief 获取声音事件定义
     *
     * @param id 声音事件ID
     * @return 声音事件定义，不存在返回 nullptr
     */
    [[nodiscard]] const SoundEventDefinition* getSoundEvent(const ResourceLocation& id) const;

    /**
     * @brief 检查声音事件是否存在
     *
     * @param id 声音事件ID
     * @return true 如果存在
     */
    [[nodiscard]] bool hasSoundEvent(const ResourceLocation& id) const;

    /**
     * @brief 获取随机声音
     *
     * 从声音事件的多个声音中，根据权重随机选择一个。
     *
     * @param id 声音事件ID
     * @param rng 随机数生成器
     * @return 选中的声音定义，不存在返回 nullptr
     */
    [[nodiscard]] const SoundDefinition* getRandomSound(const ResourceLocation& id, mc::math::Random& rng) const;

    /**
     * @brief 获取所有已注册的声音事件ID
     */
    [[nodiscard]] std::vector<ResourceLocation> getAllSoundEventIds() const;

    /**
     * @brief 获取声音事件数量
     */
    [[nodiscard]] size_t getSoundEventCount() const;

    // ========================================================================
    // 预加载
    // ========================================================================

    /**
     * @brief 获取需要预加载的声音列表
     *
     * @return 所有标记为 preload=true 的声音文件路径
     */
    [[nodiscard]] std::vector<ResourceLocation> getPreloadSounds() const;

    // ========================================================================
    // 进度回调
    // ========================================================================

    /**
     * @brief 设置加载进度回调
     *
     * @param callback 进度回调函数
     */
    void setProgressCallback(ProgressCallback callback) { m_progressCallback = std::move(callback); }

    // ========================================================================
    // 统计信息
    // ========================================================================

    /**
     * @brief 获取上次加载的错误数量
     */
    [[nodiscard]] size_t getErrorCount() const { return m_errorCount; }

    /**
     * @brief 获取上次加载的警告数量
     */
    [[nodiscard]] size_t getWarningCount() const { return m_warningCount; }

    // ========================================================================
    // 访问器
    // ========================================================================

    /**
     * @brief 获取资源包列表
     */
    [[nodiscard]] PackRepository& getResourcePacks() noexcept { return m_resourcePacks; }

    /**
     * @brief 获取资源包列表（const版本）
     */
    [[nodiscard]] const PackRepository& getResourcePacks() const noexcept { return m_resourcePacks; }

    /**
     * @brief 获取声音注册表
     */
    [[nodiscard]] SoundRegistry& getRegistry() noexcept { return m_registry; }

    /**
     * @brief 获取声音注册表（const版本）
     */
    [[nodiscard]] const SoundRegistry& getRegistry() const noexcept { return m_registry; }

private:
    /**
     * @brief 加载单个 sounds.json 文件
     *
     * @param pack 资源包
     * @param namespace 命名空间
     * @return 加载的声音事件数量，或错误
     */
    [[nodiscard]] Result<size_t> _loadSoundsJson(const IResourcePack& pack, std::string_view namespace_);

    /**
     * @brief 解析 sounds.json 内容
     *
     * @param content JSON 内容
     * @param namespace 命名空间
     * @return 解析的声音事件数量，或错误
     */
    [[nodiscard]] Result<size_t> _parseSoundsJson(std::string_view content, std::string_view namespace_);

    /**
     * @brief 通知进度
     */
    void _notifyProgress(const SoundLoadProgress& progress);

    PackRepository& m_resourcePacks;
    SoundRegistry m_registry;
    ProgressCallback m_progressCallback;
    size_t m_errorCount = 0;
    size_t m_warningCount = 0;
};

} // namespace mc::client::sound
