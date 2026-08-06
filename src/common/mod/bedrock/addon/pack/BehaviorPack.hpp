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
#include "common/mod/bedrock/addon/pack/AddonManifest.hpp"

#include <memory>
#include <string>

namespace mc::mod::bedrock::addon {

/**
 * @brief 行为包装器
 *
 * 管理单个行为包的加载和状态
 */
class BehaviorPack {
public:
    /**
     * @brief 构造函数
     * @param path 行为包目录路径
     * @param manifest 清单对象
     */
    BehaviorPack(std::string path, AddonManifest manifest);

    // 禁止拷贝
    BehaviorPack(const BehaviorPack&) = delete;
    BehaviorPack& operator=(const BehaviorPack&) = delete;

    // 允许移动
    BehaviorPack(BehaviorPack&&) noexcept = default;
    BehaviorPack& operator=(BehaviorPack&&) noexcept = default;

    ~BehaviorPack() = default;

    /**
     * @brief 获取行为包路径
     * @return 行为包目录路径
     */
    [[nodiscard]] const std::string& path() const;

    /**
     * @brief 获取清单
     * @return 清单对象
     */
    [[nodiscard]] const AddonManifest& manifest() const;

    /**
     * @brief 获取UUID
     * @return 行为包UUID
     */
    [[nodiscard]] const std::string& uuid() const;

    /**
     * @brief 获取名称
     * @return 行为包名称
     */
    [[nodiscard]] const std::string& name() const;

    /**
     * @brief 检查是否启用
     * @return 是否启用
     */
    [[nodiscard]] bool isEnabled() const;

    /**
     * @brief 设置启用状态
     * @param enabled 是否启用
     */
    void setEnabled(bool enabled);

    /**
     * @brief 获取加载优先级
     * @return 优先级（数值越大越先加载）
     */
    [[nodiscard]] i32 priority() const;

    /**
     * @brief 设置加载优先级
     * @param priority 优先级
     */
    void setPriority(i32 priority);

    /**
     * @brief 读取脚本文件
     * @param relativePath 相对于行为包目录的路径
     * @return 文件内容
     */
    [[nodiscard]] Result<std::string> readScriptFile(const std::string& relativePath) const;

    /**
     * @brief 读取行为包内任意二进制资源
     *
     * 用于读取 scripts/ 之外的资源（如 structures 目录下的 .mcstructure）。
     * @param relativePath 相对于行为包目录的路径
     * @return 文件字节内容
     */
    [[nodiscard]] Result<std::vector<u8>> readResource(const std::string& relativePath) const;

private:
    std::string m_path;
    AddonManifest m_manifest;
    bool m_enabled = true;
    i32 m_priority = 0;
};

} // namespace mc::mod::bedrock::addon
