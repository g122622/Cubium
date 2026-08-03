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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/pack/AddonModule.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"

#include <cstddef>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 行为包列表管理器
 *
 * 管理已加载的行为包集合，提供包的添加、移除、查询和依赖解析功能
 */
class BehaviorPackList {
public:
    BehaviorPackList() = default;
    ~BehaviorPackList() = default;

    // 禁止拷贝
    BehaviorPackList(const BehaviorPackList&) = delete;
    BehaviorPackList& operator=(const BehaviorPackList&) = delete;

    // 允许移动
    BehaviorPackList(BehaviorPackList&&) noexcept = default;
    BehaviorPackList& operator=(BehaviorPackList&&) noexcept = default;

    /**
     * @brief 扫描目录中的所有行为包
     * @param path 行为包目录路径
     * @return 操作结果
     */
    Result<void> scanDirectory(const std::string& path);

    /**
     * @brief 添加单个行为包
     * @param path 行为包目录路径
     * @param enabled 是否启用
     * @param priority 加载优先级
     * @return 操作结果
     */
    Result<void> addPack(const std::string& path, bool enabled = true, i32 priority = 0);

    /**
     * @brief 移除行为包
     * @param uuid 行为包UUID
     */
    void removePack(const std::string& uuid);

    /**
     * @brief 获取所有启用的行为包
     * @return 启用的行为包列表（按优先级排序）
     */
    [[nodiscard]] std::vector<BehaviorPack*> getEnabledPacks();

    /**
     * @brief 获取所有行为包
     * @return 所有行为包列表
     */
    [[nodiscard]] std::vector<const BehaviorPack*> getAllPacks() const;

    /**
     * @brief 根据UUID获取行为包
     * @param uuid 行为包UUID
     * @return 行为包指针，未找到返回nullptr
     */
    [[nodiscard]] BehaviorPack* getPackByUuid(const std::string& uuid);

    /**
     * @brief 根据UUID获取行为包（const版本）
     * @param uuid 行为包UUID
     * @return 行为包指针，未找到返回nullptr
     */
    [[nodiscard]] const BehaviorPack* getPackByUuid(const std::string& uuid) const;

    /**
     * @brief 解析所有行为包的依赖关系
     * @return 解析结果
     */
    Result<void> resolveDependencies();

    /**
     * @brief 获取所有脚本模块
     * @return 脚本模块列表
     */
    [[nodiscard]] std::vector<AddonModule> getScriptModules() const;

    /**
     * @brief 获取行为包数量
     * @return 行为包数量
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    [[nodiscard]] bool empty() const;

    /**
     * @brief 清空所有行为包
     */
    void clear();

private:
    std::vector<std::unique_ptr<BehaviorPack>> m_packs;
    mutable std::shared_mutex m_mutex;
};

} // namespace mc::mod::bedrock::addon
