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

#include "../../core/Result.hpp"
#include "../IResourcePack.hpp"
#include "../ResourceLocation.hpp"
#include "PackFormat.hpp"
#include <memory>
#include <string_view>
#include <vector>

namespace mc {
namespace resource {
namespace compat {

/**
 * @brief 资源路径/名称转换的抽象接口
 *
 * 不同 Minecraft 版本使用不同的资源命名约定（纹理、模型、方块状态）。
 * 此接口提供了一种统一的方式在不同版本之间进行转换。
 *
 * 转换遵循类似于编译器的"前端"模式:
 * - 输入: 特定版本的资源路径 (例如 MC 1.12 的纹理路径)
 * - 输出: 统一的资源路径 (MC 1.13+ 风格)
 *
 * 参考: net.minecraft.client.resources.LegacyResourcePackWrapper
 */
class ResourceMapper {
public:
    virtual ~ResourceMapper() = default;

    // -------------------------------------------------------------------------
    // 纹理路径转换
    // -------------------------------------------------------------------------

    /**
     * @brief 将纹理路径转换为统一（现代）格式
     *
     * 将特定版本的纹理路径转换为规范格式:
     * - MC 1.12: textures/blocks/stone.png -> textures/block/stone.png
     * - MC 1.12: textures/blocks/log_jungle.png -> textures/block/jungle_log.png
     *
     * @param path 原始纹理路径
     * @return 统一的纹理路径 (MC 1.13+ 风格)
     */
    [[nodiscard]] virtual std::string toUnifiedTexturePath(std::string_view path) const = 0;

    /**
     * @brief 获取统一纹理路径的所有可能路径变体
     *
     * 加载纹理时，我们可能需要尝试多个路径，因为:
     * 1. 资源包版本在检测之前是未知的
     * 2. 纹理名称在不同版本之间可能不同
     *
     * @param unifiedPath 统一纹理路径
     * @return 可能的文件路径向量（按优先级排序）
     */
    [[nodiscard]] virtual std::vector<std::string> getTexturePathVariants(std::string_view unifiedPath) const = 0;

    /**
     * @brief 将纹理名称从旧版格式转换为现代格式
     *
     * 处理命名差异，例如:
     * - log_jungle -> jungle_log
     * - wool_colored_white -> white_wool
     * - stone_granite -> granite
     *
     * @param name 纹理基本名称（不含路径和扩展名）
     * @return 现代纹理名称，如果不存在映射则返回原名称
     */
    [[nodiscard]] virtual std::string toModernTextureName(std::string_view name) const = 0;

    /**
     * @brief 将纹理名称从现代格式转换为旧版格式
     *
     * @param name 现代纹理名称
     * @return 旧版纹理名称，如果不存在映射则返回原名称
     */
    [[nodiscard]] virtual std::string toLegacyTextureName(std::string_view name) const = 0;

    // -------------------------------------------------------------------------
    // 模型路径转换
    // -------------------------------------------------------------------------

    /**
     * @brief 将模型路径转换为统一格式
     *
     * 模型路径在不同版本之间通常是一致的，
     * 但 block/item 路径在 1.13 中有变化:
     * - MC 1.12: models/block/ -> models/block/ (不变)
     * - MC 1.12: models/item/ -> models/item/ (不变)
     *
     * @param path 原始模型路径
     * @return 统一的模型路径
     */
    [[nodiscard]] virtual std::string toUnifiedModelPath(std::string_view path) const = 0;

    /**
     * @brief 获取统一模型路径的所有可能路径变体
     *
     * @param unifiedPath 统一模型路径
     * @return 可能的文件路径向量
     */
    [[nodiscard]] virtual std::vector<std::string> getModelPathVariants(std::string_view unifiedPath) const = 0;

    // -------------------------------------------------------------------------
    // 方块状态路径转换
    // -------------------------------------------------------------------------

    /**
     * @brief 将方块状态路径转换为统一格式
     *
     * 方块状态路径在 1.13 中有变化:
     * - MC 1.12: blockstates/stone.json -> blockstates/stone.json (不变)
     * - MC 1.13+: blockstates/stone.json -> blockstates/stone.json (不变)
     *
     * 注意: 方块 ID 在 1.13 中有变化（扁平化），但路径保持相似。
     *
     * @param path 原始方块状态路径
     * @return 统一的方块状态路径
     */
    [[nodiscard]] virtual std::string toUnifiedBlockStatePath(std::string_view path) const = 0;

    // -------------------------------------------------------------------------
    // 工具方法
    // -------------------------------------------------------------------------

    /**
     * @brief 获取此映射器处理的包格式
     *
     * @return 目标包格式版本
     */
    [[nodiscard]] virtual PackFormat getTargetFormat() const = 0;

    /**
     * @brief 检查资源是否存在于任何变体路径
     *
     * 按顺序尝试所有可能的路径变体，直到找到为止。
     *
     * @param pack 要检查的资源包
     * @param unifiedPath 统一资源路径
     * @return 如果资源可以找到则返回 true
     */
    [[nodiscard]] virtual bool hasResourceVariant(const IResourcePack& pack, std::string_view unifiedPath) const = 0;

    /**
     * @brief 从第一个匹配的变体路径读取资源
     *
     * 按顺序尝试所有可能的路径变体，返回第一个匹配的结果。
     *
     * @param pack 要读取的资源包
     * @param unifiedPath 统一资源路径
     * @return 资源数据，如果未找到则返回错误
     */
    [[nodiscard]] virtual Result<std::vector<u8>> readResourceVariant(
        const IResourcePack& pack, std::string_view unifiedPath) const = 0;

    // -------------------------------------------------------------------------
    // 工厂方法
    // -------------------------------------------------------------------------

    /**
     * @brief 为包格式创建适当的映射器
     *
     * @param format 包格式版本
     * @return 映射器实例（永不为空）
     */
    static std::unique_ptr<ResourceMapper> create(PackFormat format);
};

/**
 * @brief 带有公共功能的基础资源映射器
 *
 * 提供路径操作和资源变体查找的共享实现。
 */
class BaseResourceMapper : public ResourceMapper {
public:
    bool hasResourceVariant(const IResourcePack& pack, std::string_view unifiedPath) const override;

    Result<std::vector<u8>> readResourceVariant(const IResourcePack& pack, std::string_view unifiedPath) const override;

protected:
    /**
     * @brief 尝试从多个路径读取资源
     *
     * @param pack 资源包
     * @param paths 要尝试的路径（按顺序）
     * @return 资源数据或错误
     */
    Result<std::vector<u8>> tryReadFromPaths(const IResourcePack& pack, const std::vector<std::string>& paths) const;
};

} // namespace compat
} // namespace resource
} // namespace mc
