#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"

#include <optional>
#include <string_view>

namespace mc::sound {

/**
 * @brief 声音类别枚举
 *
 * 用于音量控制和分类播放。每个类别有独立的音量设置。
 * 参考: net.minecraft.util.SoundCategory
 *
 * 使用示例:
 * @code
 * SoundCategory category = SoundCategory::Blocks;
 * StringView name = getSoundCategoryName(category); // "block"
 *
 * auto parsed = parseSoundCategory("music");
 * if (parsed) {
 *     // parsed.value() == SoundCategory::Music
 * }
 * @endcode
 */
enum class SoundCategory : u8 {
    Master,   ///< 主音量（影响所有声音）
    Music,    ///< 背景音乐
    Records,  ///< 唱片机音乐
    Weather,  ///< 天气音效（雨、雷）
    Blocks,   ///< 方块音效
    Hostile,  ///< 敌对生物音效
    Neutral,  ///< 中立生物音效
    Players,  ///< 玩家音效
    Ambient,  ///< 环境音效
    Voice,    ///< 语音/字幕

    Count     ///< 类别总数（用于数组大小）
};

/**
 * @brief 获取声音类别的内部名称
 *
 * 返回与 MC Java 一致的名称字符串。
 *
 * @param category 声音类别
 * @return 类别名称字符串，如 "master", "music", "block" 等
 *
 * @note 名称与 MC Java 的 sounds.json 中使用的名称一致
 *
 * 映射关系:
 * - Master  -> "master"
 * - Music   -> "music"
 * - Records -> "record"
 * - Weather -> "weather"
 * - Blocks  -> "block"
 * - Hostile -> "hostile"
 * - Neutral -> "neutral"
 * - Players -> "player"
 * - Ambient -> "ambient"
 * - Voice   -> "voice"
 */
[[nodiscard]] StringView getSoundCategoryName(SoundCategory category) noexcept;

/**
 * @brief 从名称解析声音类别
 *
 * 将名称字符串转换为对应的 SoundCategory 枚举值。
 * 支持与 MC Java 一致的名称格式。
 *
 * @param name 类别名称（不区分大小写）
 * @return 解析结果，无效名称返回 nullopt
 *
 * 支持的名称:
 * - "master", "MASTER" -> SoundCategory::Master
 * - "music", "MUSIC" -> SoundCategory::Music
 * - "record", "RECORD", "records", "RECORDS" -> SoundCategory::Records
 * - "weather", "WEATHER" -> SoundCategory::Weather
 * - "block", "BLOCK", "blocks", "BLOCKS" -> SoundCategory::Blocks
 * - "hostile", "HOSTILE" -> SoundCategory::Hostile
 * - "neutral", "NEUTRAL" -> SoundCategory::Neutral
 * - "player", "PLAYER", "players", "PLAYERS" -> SoundCategory::Players
 * - "ambient", "AMBIENT" -> SoundCategory::Ambient
 * - "voice", "VOICE" -> SoundCategory::Voice
 */
[[nodiscard]] std::optional<SoundCategory> parseSoundCategory(StringView name) noexcept;

/**
 * @brief 检查声音类别是否有效（非 Count）
 *
 * @param category 要检查的类别
 * @return true 如果是有效的声音类别
 */
[[nodiscard]] constexpr bool isValidSoundCategory(SoundCategory category) noexcept {
    return category >= SoundCategory::Master && category < SoundCategory::Count;
}

} // namespace mc::sound
