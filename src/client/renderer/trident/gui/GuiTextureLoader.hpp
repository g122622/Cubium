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
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {} // namespace mc

namespace mc::client::renderer::trident::gui {

class GuiTextureAtlas;
class GuiSpriteAtlas;

/**
 * @brief 精灵ID到资源路径的映射条目
 *
 * 用于 MC 1.21+ 独立精灵格式的图集构建。
 * first 为旧版精灵ID（如 "heart_full"），
 * second 为新格式资源路径（如 ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/full")）。
 */
using SpriteMapping = std::pair<std::string, ResourceLocation>;

/**
 * @brief GUI纹理加载器
 *
 * 从资源包加载GUI纹理并构建纹理图集。
 * 支持从多个资源包加载，后加载的资源包优先级更高。
 *
 * 支持两种图集类型：
 * - GuiSpriteAtlas: 推荐使用，整合精灵管理和Vulkan纹理
 * - GuiTextureAtlas: 传统纹理图集
 *
 * 使用示例：
 * @code
 * GuiTextureLoader loader;
 * loader.addResourcePack(resourcePack);
 *
 * // 使用GuiSpriteAtlas（推荐）
 * loader.loadGuiTexture(spriteAtlas, "minecraft:textures/gui/widgets.png");
 *
 * // 或使用GuiTextureAtlas
 * loader.loadGuiTexture(textureAtlas, "minecraft:textures/gui/widgets.png");
 * @endcode
 */
class GuiTextureLoader {
public:
    GuiTextureLoader();
    ~GuiTextureLoader();

    // 禁止拷贝
    GuiTextureLoader(const GuiTextureLoader&) = delete;
    GuiTextureLoader& operator=(const GuiTextureLoader&) = delete;

    // 允许移动
    GuiTextureLoader(GuiTextureLoader&&) noexcept = default;
    GuiTextureLoader& operator=(GuiTextureLoader&&) noexcept = default;

    /**
     * @brief 添加资源包
     * @param resourcePack 资源包指针
     */
    void addResourcePack(std::shared_ptr<IResourcePack> resourcePack);

    /**
     * @brief 清除所有资源包
     */
    void clearResourcePacks();

    /**
     * @brief 获取资源包数量
     */
    [[nodiscard]] size_t resourcePackCount() const { return m_resourcePacks.size(); }

    // ==================== GuiSpriteAtlas 重载（推荐）====================

    /**
     * @brief 从资源包加载GUI纹理到GuiSpriteAtlas
     *
     * 按资源包优先级（后添加的优先）搜索并加载纹理。
     * 加载后会将纹理数据上传到GuiSpriteAtlas。
     *
     * @param atlas 目标精灵图集
     * @param location 纹理资源位置（如 "minecraft:textures/gui/widgets.png"）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadGuiTexture(GuiSpriteAtlas& atlas, const std::string& location);

    /**
     * @brief 从资源包加载GUI纹理到GuiSpriteAtlas（带尺寸指定）
     *
     * @param atlas 目标精灵图集
     * @param location 纹理资源位置
     * @param atlasWidth 图集宽度（用于UV计算）
     * @param atlasHeight 图集高度（用于UV计算）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadGuiTexture(
        GuiSpriteAtlas& atlas, const std::string& location, i32 atlasWidth, i32 atlasHeight);

    /**
     * @brief 加载所有GUI资源到GuiSpriteAtlas
     *
     * 加载指定纹理并注册默认精灵。
     *
     * @param atlas 目标精灵图集
     * @param textureLocation 纹理资源位置（如 "minecraft:textures/gui/icons.png"）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadAllToSpriteAtlas(GuiSpriteAtlas& atlas, const std::string& textureLocation);

    /**
     * @brief 从独立精灵文件构建精灵图集（MC 1.21+ 格式）
     *
     * MC 1.21+ 资源包将 icons.png / widgets.png 拆分为独立精灵 PNG 文件，
     * 此方法将指定映射表中的精灵文件逐一加载，拼合成纹理图集，
     * 并根据拼合结果自动注册精灵到 GuiSpriteAtlas。
     *
     * 如果映射表中的某个精灵文件不存在，跳过该精灵并继续加载其余。
     * 如果没有任何精灵加载成功，返回错误。
     *
     * @param atlas 目标精灵图集
     * @param spriteMappings 精灵ID到资源路径的映射表
     * @return 成功加载的精灵数量
     */
    [[nodiscard]] Result<u32> buildSpriteAtlas(GuiSpriteAtlas& atlas, const std::vector<SpriteMapping>& spriteMappings);

    // ==================== GuiTextureAtlas 重载（传统）====================

    /**
     * @brief 从资源包加载GUI纹理到GuiTextureAtlas
     *
     * 按资源包优先级（后添加的优先）搜索并加载纹理。
     * 加载后会将纹理数据上传到GuiTextureAtlas。
     *
     * @param atlas 目标纹理图集
     * @param location 纹理资源位置（如 "minecraft:textures/gui/widgets.png"）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadGuiTexture(GuiTextureAtlas& atlas, const std::string& location);

    /**
     * @brief 从JSON加载精灵定义
     *
     * 解析JSON文件并注册精灵到图集。
     *
     * @param atlas 目标纹理图集
     * @param jsonPath JSON文件路径（如 "minecraft:gui/sprites/widgets.json"）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadSpritesFromJson(GuiTextureAtlas& atlas, const std::string& jsonPath);

    /**
     * @brief 加载默认GUI纹理
     *
     * 加载内置的默认GUI纹理：
     * - widgets.png（按钮、快捷栏等）
     * - icons.png（心形、饥饿等）
     *
     * 如果资源包中没有这些纹理，使用硬编码的默认值。
     *
     * @param atlas 目标纹理图集
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadDefaultTextures(GuiTextureAtlas& atlas);

    /**
     * @brief 加载所有GUI资源
     *
     * 按顺序加载：
     * 1. widgets.png 和对应精灵定义
     * 2. icons.png 和对应精灵定义
     * 3. 注册默认精灵（如果资源包未提供）
     *
     * @param atlas 目标纹理图集
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadAll(GuiTextureAtlas& atlas);

    // ==================== 工具方法 ====================

    /**
     * @brief 解码PNG纹理数据
     * @param data PNG文件内容
     * @param outWidth 输出宽度
     * @param outHeight 输出高度
     * @param outPixels 输出像素数据（RGBA8）
     * @return 成功或错误
     */
    [[nodiscard]] static Result<void> decodePng(
        const std::vector<u8>& data, i32& outWidth, i32& outHeight, std::vector<u8>& outPixels);

    /**
     * @brief 从文件加载PNG纹理
     * @param filePath 文件路径
     * @param outWidth 输出宽度
     * @param outHeight 输出高度
     * @param outPixels 输出像素数据（RGBA8）
     * @return 成功或错误
     */
    [[nodiscard]] static Result<void> loadPngFromFile(
        const std::string& filePath, i32& outWidth, i32& outHeight, std::vector<u8>& outPixels);

private:
    /**
     * @brief 在资源包中查找纹理
     * @param location 纹理资源位置
     * @param outData 输出纹理数据
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> _findTexture(const std::string& location, std::vector<u8>& outData);

    /**
     * @brief 构建资源路径
     * @param location 资源位置
     * @return 文件路径
     */
    [[nodiscard]] static std::string _buildResourcePath(const std::string& location);

    std::vector<std::shared_ptr<IResourcePack>> m_resourcePacks;
};

} // namespace mc::client::renderer::trident::gui
