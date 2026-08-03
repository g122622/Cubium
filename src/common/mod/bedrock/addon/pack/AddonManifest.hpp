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
#include "common/mod/bedrock/addon/pack/AddonDependency.hpp"
#include "common/mod/bedrock/addon/pack/AddonModule.hpp"
#include "common/mod/bedrock/addon/pack/PackVersion.hpp"

#include <optional>
#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 清单头部信息
 */
struct AddonManifestHeader {
    std::string name;
    std::string description;
    std::string uuid;
    PackVersion version;
    PackVersion minEngineVersion;
};

/**
 * @brief 基岩版清单文件解析器
 *
 * 解析 Bedrock Edition manifest.json format_version 2 格式
 */
struct AddonManifest {
    i32 formatVersion = 2;
    AddonManifestHeader header;
    std::vector<AddonModule> modules;
    std::vector<AddonDependency> dependencies;
    std::vector<std::string> capabilities;

    /**
     * @brief 从JSON字符串解析清单
     * @param json JSON字符串
     * @return 解析结果
     */
    static Result<AddonManifest> parse(const std::string& json);

    /**
     * @brief 从文件加载清单
     * @param path manifest.json文件路径
     * @return 解析结果
     */
    static Result<AddonManifest> loadFromFile(const std::string& path);

    /**
     * @brief 检查是否包含脚本模块
     * @return 是否包含脚本模块
     */
    [[nodiscard]] bool hasScriptModule() const noexcept;

    /**
     * @brief 获取所有脚本模块
     * @return 脚本模块列表
     */
    [[nodiscard]] std::vector<AddonModule> getScriptModules() const;

    /**
     * @brief 检查是否具有指定能力
     * @param cap 能力名称
     * @return 是否具有该能力
     */
    [[nodiscard]] bool hasCapability(const std::string& cap) const noexcept;
};

} // namespace mc::mod::bedrock::addon
