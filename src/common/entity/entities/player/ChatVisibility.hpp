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
