#pragma once

#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "common/core/Types.hpp"
#include <vector>

namespace mc::client::renderer::trident::gui {

class GuiTextureAtlas;
class GuiSpriteManager;
class GuiSpriteAtlas;

/**
 * @brief GUI精灵注册表
 *
 * 提供Minecraft默认GUI精灵定义，并支持从资源包加载自定义精灵。
 *
 * 默认精灵基于Minecraft 1.16.5的纹理布局：
 * - widgets.png (256x256): 按钮、快捷栏、滑动条等
 * - icons.png (256x256): 心形、盔甲、饥饿、经验条等
 *
 * 支持三种目标：
 * - GuiSpriteManager: 纯数据管理，不依赖Vulkan，适合测试和资源加载阶段
 * - GuiTextureAtlas: Vulkan纹理图集，包含GPU资源
 * - GuiSpriteAtlas: 整合类（精灵数据 + Vulkan纹理）
 */
class GuiSpriteRegistry {
public:
    // ==================== GuiSpriteManager 重载（不依赖Vulkan）====================

    /**
     * @brief 注册widgets.png中的默认精灵到 GuiSpriteManager
     */
    static void registerWidgetsSprites(GuiSpriteManager& manager, i32 atlasWidth = 256, i32 atlasHeight = 256);

    /**
     * @brief 注册icons.png中的默认精灵到 GuiSpriteManager
     */
    static void registerIconsSprites(GuiSpriteManager& manager, i32 atlasWidth = 256, i32 atlasHeight = 256);

    /**
     * @brief 注册容器GUI精灵到 GuiSpriteManager
     */
    static void registerContainerSprites(GuiSpriteManager& manager, i32 atlasWidth = 256, i32 atlasHeight = 256);

    /**
     * @brief 注册所有默认精灵到 GuiSpriteManager
     */
    static void registerAllDefaults(GuiSpriteManager& manager, i32 atlasWidth = 256, i32 atlasHeight = 256);

    // ==================== GuiTextureAtlas 重载（依赖Vulkan）====================

    /**
     * @brief 注册widgets.png中的默认精灵到 GuiTextureAtlas
     *
     * 包括：
     * - 按钮（正常、悬停、禁用）
     * - 快捷栏背景和选中高亮
     * - 滑动条
     *
     * @param atlas 目标纹理图集
     * @param atlasWidth 图集宽度（默认256）
     * @param atlasHeight 图集高度（默认256）
     */
    static void registerWidgetsSprites(GuiTextureAtlas& atlas, i32 atlasWidth = 256, i32 atlasHeight = 256);

    /**
     * @brief 注册icons.png中的默认精灵到 GuiTextureAtlas
     *
     * 包括：
     * - 心形图标（满、半、空、吸收、中毒、凋零）
     * - 盔甲图标（满、半、空）
     * - 饥饿图标（满、半、空）
     * - 经验条
     * - 气泡
     * - 准星
     *
     * @param atlas 目标纹理图集
     * @param atlasWidth 图集宽度（默认256）
     * @param atlasHeight 图集高度（默认256）
     */
    static void registerIconsSprites(GuiTextureAtlas& atlas, i32 atlasWidth = 256, i32 atlasHeight = 256);

    /**
     * @brief 注册容器GUI精灵到 GuiTextureAtlas
     *
     * 包括：
     * - 槽位背景
     * - 容器背景（176x166）
     *
     * @param atlas 目标纹理图集
     * @param atlasWidth 图集宽度（默认256）
     * @param atlasHeight 图集高度（默认256）
     */
    static void registerContainerSprites(GuiTextureAtlas& atlas, i32 atlasWidth = 256, i32 atlasHeight = 256);

    /**
     * @brief 注册所有默认精灵到 GuiTextureAtlas
     *
     * 依次调用 registerWidgetsSprites、registerIconsSprites、registerContainerSprites
     *
     * @param atlas 目标纹理图集
     * @param atlasWidth 图集宽度（默认256）
     * @param atlasHeight 图集高度（默认256）
     */
    static void registerAllDefaults(GuiTextureAtlas& atlas, i32 atlasWidth = 256, i32 atlasHeight = 256);

    // ==================== GuiSpriteAtlas 重载（整合类）====================

    /**
     * @brief 注册widgets.png中的默认精灵到 GuiSpriteAtlas
     *
     * @param atlas 目标精灵图集
     * @param atlasWidth 图集宽度（0=自动使用图集实际尺寸）
     * @param atlasHeight 图集高度（0=自动使用图集实际尺寸）
     */
    static void registerWidgetsSprites(GuiSpriteAtlas& atlas, i32 atlasWidth = 0, i32 atlasHeight = 0);

    /**
     * @brief 注册icons.png中的默认精灵到 GuiSpriteAtlas
     *
     * @param atlas 目标精灵图集
     * @param atlasWidth 图集宽度（0=自动使用图集实际尺寸）
     * @param atlasHeight 图集高度（0=自动使用图集实际尺寸）
     */
    static void registerIconsSprites(GuiSpriteAtlas& atlas, i32 atlasWidth = 0, i32 atlasHeight = 0);

    /**
     * @brief 注册容器GUI精灵到 GuiSpriteAtlas
     *
     * @param atlas 目标精灵图集
     * @param atlasWidth 图集宽度（0=自动使用图集实际尺寸）
     * @param atlasHeight 图集高度（0=自动使用图集实际尺寸）
     */
    static void registerContainerSprites(GuiSpriteAtlas& atlas, i32 atlasWidth = 0, i32 atlasHeight = 0);

    /**
     * @brief 注册所有默认精灵到 GuiSpriteAtlas
     *
     * @param atlas 目标精灵图集
     * @param atlasWidth 图集宽度（0=自动使用图集实际尺寸）
     * @param atlasHeight 图集高度（0=自动使用图集实际尺寸）
     */
    static void registerAllDefaults(GuiSpriteAtlas& atlas, i32 atlasWidth = 0, i32 atlasHeight = 0);

    // ==================== 精灵列表获取（用于调试）====================

    /**
     * @brief 获取widgets精灵列表（用于调试）
     */
    [[nodiscard]] static std::vector<GuiSprite> getWidgetsSpriteList(i32 atlasWidth = 256, i32 atlasHeight = 256);

    /**
     * @brief 获取icons精灵列表（用于调试）
     */
    [[nodiscard]] static std::vector<GuiSprite> getIconsSpriteList(i32 atlasWidth = 256, i32 atlasHeight = 256);
};

} // namespace mc::client::renderer::trident::gui
