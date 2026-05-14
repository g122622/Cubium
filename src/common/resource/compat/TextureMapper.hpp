#pragma once

#include "../../core/Types.hpp"
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mc {
namespace resource {
namespace compat {

/**
 * @brief 纹理名称映射数据库
 *
 * 包含 MC 1.12 和 1.13+ 命名约定之间的 854+ 个双向映射。
 * 从 MC Java 的 LegacyResourcePackWrapper 生成。
 *
 * 主要映射包括:
 * - 原木: log_jungle <-> jungle_log
 * - 树叶: leaves_oak <-> oak_leaves
 * - 木板: planks_oak <-> oak_planks
 * - 羊毛: wool_colored_white <-> white_wool
 * - 石头变种: stone_granite <-> granite
 * - 花: flower_rose <-> poppy
 *
 * 参考: net.minecraft.client.resources.LegacyResourcePackWrapper
 */
class TextureMapper {
public:
    /**
     * @brief 获取单例实例
     *
     * 线程安全的初始化。
     */
    static const TextureMapper& instance();

    // -------------------------------------------------------------------------
    // 名称映射
    // -------------------------------------------------------------------------

    /**
     * @brief 获取 1.13+（现代）纹理名称对应的 1.12（旧版）名称
     *
     * @param modernName 现代纹理名称（例如 "jungle_log"）
     * @return 旧版名称（例如 "log_jungle"），如果未找到则返回空
     */
    [[nodiscard]] std::string getLegacyName(std::string_view modernName) const;

    /**
     * @brief 获取 1.12（旧版）纹理名称对应的 1.13+（现代）名称
     *
     * @param legacyName 旧版纹理名称（例如 "log_jungle"）
     * @return 现代名称（例如 "jungle_log"），如果未找到则返回空
     */
    [[nodiscard]] std::string getModernName(std::string_view legacyName) const;

    /**
     * @brief 获取纹理的所有名称变体
     *
     * 如果存在映射则返回现代和旧版名称，
     * 如果不存在映射则只返回原名称。
     *
     * @param name 纹理名称（现代或旧版）
     * @return 可能的名称向量（先现代，后旧版）
     */
    [[nodiscard]] std::vector<std::string> getNameVariants(std::string_view name) const;

    /**
     * @brief 检查名称是否存在映射
     *
     * @param name 要检查的纹理名称
     * @return 如果此名称存在映射则返回 true
     */
    [[nodiscard]] bool hasMapping(std::string_view name) const;

    // -------------------------------------------------------------------------
    // 路径转换
    // -------------------------------------------------------------------------

    /**
     * @brief 将纹理路径从旧版格式转换为现代格式
     *
     * 处理目录变化和名称变化:
     * - textures/blocks/log_jungle.png -> textures/block/jungle_log.png
     * - textures/blocks/stone_granite.png -> textures/block/granite.png
     *
     * @param legacyPath 旧版路径（MC 1.12 格式）
     * @return 现代路径（MC 1.13+ 格式）
     */
    [[nodiscard]] std::string toModernPath(std::string_view legacyPath) const;

    /**
     * @brief 将纹理路径从现代格式转换为旧版格式
     *
     * @param modernPath 现代路径（MC 1.13+ 格式）
     * @return 旧版路径（MC 1.12 格式）
     */
    [[nodiscard]] std::string toLegacyPath(std::string_view modernPath) const;

    /**
     * @brief 获取纹理路径的所有路径变体
     *
     * 生成加载纹理时要尝试的所有可能路径:
     * - 现代路径配现代名称
     * - 现代路径配旧版名称
     * - 旧版路径配现代名称
     * - 旧版路径配旧版名称
     *
     * @param path 纹理路径（现代或旧版）
     * @return 可能的路径向量
     */
    [[nodiscard]] std::vector<std::string> getPathVariants(std::string_view path) const;

    // -------------------------------------------------------------------------
    // 统计信息
    // -------------------------------------------------------------------------

    /**
     * @brief 获取纹理名称映射数量
     */
    [[nodiscard]] size_t getMappingCount() const noexcept { return m_modernToLegacy.size(); }

private:
    TextureMapper();

    // 初始化所有纹理映射
    void initializeMappings();

    // 双向映射: 现代 <-> 旧版
    std::unordered_map<std::string, std::string> m_modernToLegacy;
    std::unordered_map<std::string, std::string> m_legacyToModern;

    // 线程安全初始化的互斥锁
    static std::mutex s_mutex;
    static TextureMapper* s_instance;
};

} // namespace compat
} // namespace resource
} // namespace mc
