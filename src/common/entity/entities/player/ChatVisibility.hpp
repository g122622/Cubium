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
 * @brief 玩家聊天可见性设置
 *
 * 对齐 MC 1.16.5 `net.minecraft.entity.player.ChatVisibility`。
 */
enum class ChatVisibility : i32 {
    Full = 0,
    System = 1,
    Hidden = 2,
};

[[nodiscard]] constexpr i32 getChatVisibilityId(ChatVisibility visibility) noexcept
{
    return static_cast<i32>(visibility);
}

[[nodiscard]] constexpr const char* getChatVisibilityTranslationKey(ChatVisibility visibility) noexcept
{
    switch (visibility) {
        case ChatVisibility::Full:
            return "options.chat.visibility.full";
        case ChatVisibility::System:
            return "options.chat.visibility.system";
        case ChatVisibility::Hidden:
            return "options.chat.visibility.hidden";
    }

    return "options.chat.visibility.full";
}

[[nodiscard]] constexpr ChatVisibility getChatVisibilityById(i32 id) noexcept
{
    constexpr i32 count = 3;
    const i32 normalized = ((id % count) + count) % count;
    return static_cast<ChatVisibility>(normalized);
}

} // namespace mc
