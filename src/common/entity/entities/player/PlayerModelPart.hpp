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

#include "../../../core/Types.hpp"

namespace mc {

/**
 * @brief 玩家皮肤可切换部件
 *
 * 对齐 MC 1.16.5 `net.minecraft.entity.player.PlayerModelPart`。
 */
enum class PlayerModelPart : u8 {
    Cape = 0,
    Jacket = 1,
    LeftSleeve = 2,
    RightSleeve = 3,
    LeftPantsLeg = 4,
    RightPantsLeg = 5,
    Hat = 6,
};

[[nodiscard]] constexpr u8 getPlayerModelPartId(PlayerModelPart part) noexcept
{
    return static_cast<u8>(part);
}

[[nodiscard]] constexpr u8 getPlayerModelPartMask(PlayerModelPart part) noexcept
{
    return static_cast<u8>(1u << getPlayerModelPartId(part));
}

[[nodiscard]] constexpr const char* getPlayerModelPartName(PlayerModelPart part) noexcept
{
    switch (part) {
        case PlayerModelPart::Cape:
            return "cape";
        case PlayerModelPart::Jacket:
            return "jacket";
        case PlayerModelPart::LeftSleeve:
            return "left_sleeve";
        case PlayerModelPart::RightSleeve:
            return "right_sleeve";
        case PlayerModelPart::LeftPantsLeg:
            return "left_pants_leg";
        case PlayerModelPart::RightPantsLeg:
            return "right_pants_leg";
        case PlayerModelPart::Hat:
            return "hat";
    }

    return "cape";
}

[[nodiscard]] constexpr const char* getPlayerModelPartTranslationKey(PlayerModelPart part) noexcept
{
    switch (part) {
        case PlayerModelPart::Cape:
            return "options.modelPart.cape";
        case PlayerModelPart::Jacket:
            return "options.modelPart.jacket";
        case PlayerModelPart::LeftSleeve:
            return "options.modelPart.left_sleeve";
        case PlayerModelPart::RightSleeve:
            return "options.modelPart.right_sleeve";
        case PlayerModelPart::LeftPantsLeg:
            return "options.modelPart.left_pants_leg";
        case PlayerModelPart::RightPantsLeg:
            return "options.modelPart.right_pants_leg";
        case PlayerModelPart::Hat:
            return "options.modelPart.hat";
    }

    return "options.modelPart.cape";
}

inline constexpr u8 PLAYER_MODEL_PARTS_ALL_MASK = getPlayerModelPartMask(PlayerModelPart::Cape) |
    getPlayerModelPartMask(PlayerModelPart::Jacket) | getPlayerModelPartMask(PlayerModelPart::LeftSleeve) |
    getPlayerModelPartMask(PlayerModelPart::RightSleeve) | getPlayerModelPartMask(PlayerModelPart::LeftPantsLeg) |
    getPlayerModelPartMask(PlayerModelPart::RightPantsLeg) | getPlayerModelPartMask(PlayerModelPart::Hat);

} // namespace mc
