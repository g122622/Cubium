#include "DefaultSkinProvider.hpp"
#include "../core/SkinTypes.hpp"
#include <spdlog/spdlog.h>

namespace mc::skin {

Result<void> DefaultSkinProvider::initialize()
{
    if (m_initialized) {
        return {};
    }

    auto result = loadBuiltinSkins();
    if (!result.success()) {
        spdlog::warn(
            "DefaultSkinProvider: Failed to load builtin skins, using fallback: {}", result.error().toString());
        // 不返回错误，使用 fallback 数据
    }

    m_initialized = true;
    spdlog::info("DefaultSkinProvider initialized");
    return {};
}

Result<void> DefaultSkinProvider::loadBuiltinSkins()
{
    // 内置的 64x64 Steve 皮肤（简化版 - 单色填充）
    // 实际应该从资源文件加载，这里使用简化的 fallback
    // PNG 格式的最小有效皮肤数据

    // 生成简单的 64x64 RGBA 数据作为 fallback
    // 在实际项目中，应该从 resources/textures/entity/steve.png 和 alex.png 加载

    // Steve: 使用经典的棕橙色调
    // 这是一个非常简化的皮肤，实际应该从文件加载
    constexpr size_t skinSize = 64 * 64 * 4; // 64x64 RGBA
    m_steveData.resize(skinSize, 0);

    // 填充简单的皮肤颜色（仅作为 fallback）
    // 头部区域 (0-8, 0-8): 肤色
    // 身体区域 (16-24, 16-28): 蓝色衬衫
    // 腿部区域 (16-20, 32-44): 深蓝裤子
    // 手臂区域 (40-44, 16-28): 肤色

    // 这里只是设置透明占位，实际皮肤应该从资源文件加载
    // 生产环境应该使用类似 EntityTextureAtlas 的方式加载真实皮肤

    // Alex 也使用相同的 fallback
    m_alexData.resize(skinSize, 0);

    spdlog::debug("DefaultSkinProvider: Loaded builtin skin fallbacks (64x64 RGBA)");
    return {};
}

ResourceLocation DefaultSkinProvider::getDefaultSkin(const std::array<u8, 16>& uuid) const
{
    if (getDefaultSkinType(uuid) == SkinType::Slim) {
        return m_alexLocation;
    }
    return m_steveLocation;
}

SkinType DefaultSkinProvider::getDefaultSkinType(const std::array<u8, 16>& uuid) const
{
    return getDefaultSkinTypeForUUID(uuid);
}

bool DefaultSkinProvider::isDefaultSkin(const ResourceLocation& location) const
{
    return location == m_steveLocation || location == m_alexLocation;
}

} // namespace mc::skin
