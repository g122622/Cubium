#pragma once

#include "../../core/Types.hpp"
#include <string_view>

namespace mc {
namespace resource {
namespace compat {

/**
 * @brief Minecraft 资源包格式版本
 *
 * pack.mcmeta 中的 pack_format 数值:
 * - 1: MC 1.6.1 - 1.8.9
 * - 2: MC 1.9 - 1.10.2
 * - 3: MC 1.11 - 1.12.2  (关键: 旧版纹理路径 textures/blocks/)
 * - 4: MC 1.13 - 1.14.4  (关键: 扁平化 + 新路径 textures/block/)
 * - 5: MC 1.15 - 1.16.1
 * - 6: MC 1.16.2 - 1.16.5
 * - 7: MC 1.17.x
 * - 8: MC 1.18.x
 * - 9: MC 1.19.x
 *
 * 参考: net.minecraft.resources.ResourcePackInfo
 */
enum class PackFormat : i32 {
    Unknown = 0,
    V1_6_to_1_8 = 1,
    V1_9_to_1_10 = 2,
    V1_11_to_1_12 = 3,   ///< 旧版纹理路径: textures/blocks/
    V1_13_to_1_14 = 4,   ///< 新版纹理路径: textures/block/
    V1_15_to_1_16_1 = 5,
    V1_16_2_to_1_16_5 = 6,
    V1_17 = 7,
    V1_18 = 8,
    V1_19 = 9,
};

// 前向声明
class PackMetadata;

/**
 * @brief 从 pack.mcmeta 元数据检测包格式
 *
 * @param packFormat pack.mcmeta 中的 pack_format 值
 * @return 检测到的包格式枚举值
 */
[[nodiscard]] PackFormat detectPackFormat(i32 packFormat);

/**
 * @brief 检查格式是否使用旧版纹理路径 (textures/blocks/)
 *
 * MC 1.12 及更早版本使用 "textures/blocks/" 目录。
 * MC 1.13+ 使用 "textures/block/" 目录。
 *
 * @param format 包格式版本
 * @return 如果使用旧版路径则返回 true
 */
[[nodiscard]] bool usesOldTexturePaths(PackFormat format);

/**
 * @brief 检查格式是否使用新版纹理路径 (textures/block/)
 *
 * @param format 包格式版本
 * @return 如果使用新版路径则返回 true
 */
[[nodiscard]] bool usesNewTexturePaths(PackFormat format);

/**
 * @brief 检查格式是否使用旧版物品路径 (textures/items/)
 *
 * MC 1.12 及更早版本使用 "textures/items/" 目录。
 * MC 1.13+ 使用 "textures/item/" 目录。
 *
 * @param format 包格式版本
 * @return 如果使用旧版物品路径则返回 true
 */
[[nodiscard]] bool usesOldItemPaths(PackFormat format);

/**
 * @brief 获取包格式的人类可读名称
 *
 * @param format 包格式版本
 * @return 字符串表示 (例如: "1.12.2", "1.16.5")
 */
[[nodiscard]] std::string_view packFormatToString(PackFormat format);

/**
 * @brief 检查格式是否需要纹理名称映射
 *
 * MC 1.12 使用不同的纹理命名约定:
 * - log_jungle vs jungle_log
 * - wool_colored_white vs white_wool
 * - stone_granite vs granite
 *
 * @param format 包格式版本
 * @return 如果需要名称映射到现代名称则返回 true
 */
[[nodiscard]] bool requiresTextureNameMapping(PackFormat format);

} // namespace compat
} // namespace resource
} // namespace mc
