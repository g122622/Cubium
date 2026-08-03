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

#include "common/core/Types.hpp"
#include "common/util/property/Properties.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace mc {
namespace entity {

/**
 * @brief 铜傀儡氧化等级（与方块的 WeatheringCopper.WeatherState 对应）
 *
 * 对应 MC 1.21.11: net.minecraft.world.level.block.WeatheringCopper.WeatherState
 * 铜傀儡实体持有此状态以决定外观纹理与音效变种。
 */
enum class CopperGolemWeatherState : u8 {
    Unaffected = 0, ///< 未氧化（基础）
    Exposed = 1,    ///< 斑驳
    Weathered = 2,  ///< 锈蚀
    Oxidized = 3    ///< 氧化
};

/**
 * @brief 铜傀儡行为状态（动画状态机标识）
 *
 * 对应 MC 1.21.11: net.minecraft.world.entity.animal.golem.CopperGolemState
 * 由 MC Brain 系统在物品运输行为中切换，客户端用于触发动画。
 */
enum class CopperGolemState : u8 {
    Idle = 0,          ///< 空闲
    GettingItem = 1,   ///< 拾取到物品
    GettingNoItem = 2, ///< 尝试拾取但无物品
    DroppingItem = 3,  ///< 放下物品
    DroppingNoItem = 4 ///< 尝试放下但无物品
};

/**
 * @brief 铜傀儡氧化等级工具集
 *
 * 提供 WeatherState 的下一个/上一个等级查询、字符串序列化、
 * 以及与方块 OxidationLevel 之间的互转。
 *
 * 对应 MC 1.21.11: net.minecraft.world.entity.animal.golem.CopperGolemOxidationLevels
 */
class CopperGolemOxidationUtils {
public:
    /**
     * @brief 获取下一氧化等级
     * @param current 当前等级
     * @return 下一等级，如果已是最高返回 std::nullopt
     */
    [[nodiscard]] static std::optional<CopperGolemWeatherState> next(CopperGolemWeatherState current) noexcept
    {
        switch (current) {
            case CopperGolemWeatherState::Unaffected:
                return CopperGolemWeatherState::Exposed;
            case CopperGolemWeatherState::Exposed:
                return CopperGolemWeatherState::Weathered;
            case CopperGolemWeatherState::Weathered:
                return CopperGolemWeatherState::Oxidized;
            case CopperGolemWeatherState::Oxidized:
                return std::nullopt;
        }
        return std::nullopt;
    }

    /**
     * @brief 获取上一氧化等级
     * @param current 当前等级
     * @return 上一等级，如果已是最低返回 std::nullopt
     */
    [[nodiscard]] static std::optional<CopperGolemWeatherState> previous(CopperGolemWeatherState current) noexcept
    {
        switch (current) {
            case CopperGolemWeatherState::Unaffected:
                return std::nullopt;
            case CopperGolemWeatherState::Exposed:
                return CopperGolemWeatherState::Unaffected;
            case CopperGolemWeatherState::Weathered:
                return CopperGolemWeatherState::Exposed;
            case CopperGolemWeatherState::Oxidized:
                return CopperGolemWeatherState::Weathered;
        }
        return std::nullopt;
    }

    /**
     * @brief 从方块 OxidationLevel 转换
     */
    [[nodiscard]] static CopperGolemWeatherState fromBlockOxidation(BlockStateProperties::OxidationLevel level) noexcept
    {
        switch (level) {
            case BlockStateProperties::OxidationLevel::Unaffected:
                return CopperGolemWeatherState::Unaffected;
            case BlockStateProperties::OxidationLevel::Exposed:
                return CopperGolemWeatherState::Exposed;
            case BlockStateProperties::OxidationLevel::Weathered:
                return CopperGolemWeatherState::Weathered;
            case BlockStateProperties::OxidationLevel::Oxidized:
                return CopperGolemWeatherState::Oxidized;
        }
        return CopperGolemWeatherState::Unaffected;
    }

    /**
     * @brief 转换为方块 OxidationLevel
     */
    [[nodiscard]] static BlockStateProperties::OxidationLevel toBlockOxidation(CopperGolemWeatherState state) noexcept
    {
        switch (state) {
            case CopperGolemWeatherState::Unaffected:
                return BlockStateProperties::OxidationLevel::Unaffected;
            case CopperGolemWeatherState::Exposed:
                return BlockStateProperties::OxidationLevel::Exposed;
            case CopperGolemWeatherState::Weathered:
                return BlockStateProperties::OxidationLevel::Weathered;
            case CopperGolemWeatherState::Oxidized:
                return BlockStateProperties::OxidationLevel::Oxidized;
        }
        return BlockStateProperties::OxidationLevel::Unaffected;
    }

    /**
     * @brief 序列化为字符串（用于 NBT 持久化）
     */
    [[nodiscard]] static std::string toString(CopperGolemWeatherState state)
    {
        switch (state) {
            case CopperGolemWeatherState::Unaffected:
                return "unaffected";
            case CopperGolemWeatherState::Exposed:
                return "exposed";
            case CopperGolemWeatherState::Weathered:
                return "weathered";
            case CopperGolemWeatherState::Oxidized:
                return "oxidized";
        }
        return "unaffected";
    }

    /**
     * @brief 从字符串反序列化
     */
    [[nodiscard]] static CopperGolemWeatherState fromString(std::string_view str)
    {
        if (str == "exposed") return CopperGolemWeatherState::Exposed;
        if (str == "weathered") return CopperGolemWeatherState::Weathered;
        if (str == "oxidized") return CopperGolemWeatherState::Oxidized;
        return CopperGolemWeatherState::Unaffected;
    }

    /**
     * @brief 序列化 CopperGolemState 为字符串
     */
    [[nodiscard]] static std::string stateToString(CopperGolemState state)
    {
        switch (state) {
            case CopperGolemState::Idle:
                return "idle";
            case CopperGolemState::GettingItem:
                return "getting_item";
            case CopperGolemState::GettingNoItem:
                return "getting_no_item";
            case CopperGolemState::DroppingItem:
                return "dropping_item";
            case CopperGolemState::DroppingNoItem:
                return "dropping_no_item";
        }
        return "idle";
    }

    /**
     * @brief 从字符串反序列化 CopperGolemState
     */
    [[nodiscard]] static CopperGolemState stateFromString(std::string_view str)
    {
        if (str == "getting_item") return CopperGolemState::GettingItem;
        if (str == "getting_no_item") return CopperGolemState::GettingNoItem;
        if (str == "dropping_item") return CopperGolemState::DroppingItem;
        if (str == "dropping_no_item") return CopperGolemState::DroppingNoItem;
        return CopperGolemState::Idle;
    }
};

} // namespace entity
} // namespace mc
