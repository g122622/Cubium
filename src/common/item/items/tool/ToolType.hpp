#pragma once

#include "../../../core/Types.hpp"

namespace mc {
namespace item {
namespace tool {

/**
 * @brief 工具类型枚举
 *
 * 定义工具的类型，用于判断方块是否可以被特定工具有效挖掘。
 * 方块可以设置其需要的工具类型，工具则声明自己的类型。
 *
 * 参考: net.minecraftforge.common.ToolType
 */
enum class ToolType : u8 {
    None = 0,       ///< 无需工具或非工具物品
    Pickaxe = 1,    ///< 镐 - 用于采矿（石头、矿石等）
    Axe = 2,        ///< 斧 - 用于伐木（原木、木板等）
    Shovel = 3,     ///< 锹 - 用于挖掘（泥土、沙子、雪等）
    Hoe = 4,        ///< 锄 - 用于耕作（干草、树叶等）
    Sword = 5,      ///< 剑 - 对蜘蛛网有效
    Shears = 6,     ///< 剪刀 - 用于剪羊毛、树叶等
};

/**
 * @brief 获取工具类型的名称字符串
 * @param type 工具类型
 * @return 类型名称（如 "pickaxe"、"axe" 等）
 */
[[nodiscard]] const char* toString(ToolType type);

} // namespace tool
} // namespace item
} // namespace mc
