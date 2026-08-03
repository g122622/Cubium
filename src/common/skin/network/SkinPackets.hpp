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
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include "common/util/text/ITextComponentFwd.hpp"
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc::skin {

/**
 * @brief 玩家列表操作类型
 */
enum class PlayerListAction : u8 {
    AddPlayer = 0,         // 添加玩家
    UpdateGameMode = 1,    // 更新游戏模式
    UpdateLatency = 2,     // 更新延迟
    UpdateDisplayName = 3, // 更新显示名
    RemovePlayer = 4       // 移除玩家
};

/**
 * @brief 玩家列表条目
 *
 * 存储单个玩家的信息，作为 IR ir::play::PlayerInfoUpdate / PlayerInfoRemove 的逻辑载荷。
 * （旧 PlayerListItemPacket 类已随 Phase8 删除。）
 *
 * displayName 字段说明：
 * - 存储 JSON 格式的 ITextComponent 序列化结果
 * - 例如：{"text":"PlayerName","color":"red","bold":true}
 * - 使用 setDisplayName() 方法从 ITextComponent 设置
 * - 使用 getDisplayNameAsText() 方法解析为 ITextComponent
 */
struct PlayerListEntry {
    std::array<u8, 16> uuid;                     // 玩家UUID
    std::string name;                            // 玩家名称
    std::vector<GameProfileProperty> properties; // 档案属性（皮肤等）
    GameMode gameMode = GameMode::Survival;      // 游戏模式
    i32 ping = 0;                                // 延迟（毫秒）
    std::optional<std::string> displayName;      // 显示名（JSON格式的ITextComponent）

    PlayerListEntry() = default;

    // 移动构造和赋值
    PlayerListEntry(PlayerListEntry&&) = default;
    PlayerListEntry& operator=(PlayerListEntry&&) = default;
    PlayerListEntry(const PlayerListEntry&) = default;
    PlayerListEntry& operator=(const PlayerListEntry&) = default;

    /**
     * @brief 创建添加玩家条目
     * @param profile 玩家档案
     * @param gameMode 游戏模式
     * @param ping 延迟
     */
    static PlayerListEntry createAdd(const GameProfile& profile, GameMode gameMode, i32 ping) noexcept;

    /**
     * @brief 创建移除玩家条目
     * @param uuid 玩家UUID
     */
    static PlayerListEntry createRemove(const std::array<u8, 16>& uuid) noexcept;

    /**
     * @brief 创建更新延迟条目
     * @param uuid 玩家UUID
     * @param ping 新延迟
     */
    static PlayerListEntry createUpdateLatency(const std::array<u8, 16>& uuid, i32 ping) noexcept;

    /**
     * @brief 创建更新游戏模式条目
     * @param uuid 玩家UUID
     * @param gameMode 新游戏模式
     */
    static PlayerListEntry createUpdateGameMode(const std::array<u8, 16>& uuid, GameMode gameMode) noexcept;

    /**
     * @brief 创建更新显示名条目
     * @param uuid 玩家UUID
     * @param displayName 显示名（JSON格式的ITextComponent），std::nullopt表示清除显示名
     */
    static PlayerListEntry createUpdateDisplayName(
        const std::array<u8, 16>& uuid, const std::optional<std::string>& displayName) noexcept;

    /**
     * @brief 从ITextComponent设置显示名
     * @param text 文本组件
     *
     * 将文本组件序列化为JSON字符串存储到displayName字段。
     */
    void setDisplayName(const text::ITextComponent& text);

    /**
     * @brief 获取显示名作为ITextComponent
     * @return 文本组件，如果displayName为空或解析失败则返回nullptr
     */
    [[nodiscard]] std::unique_ptr<text::ITextComponent> getDisplayNameAsText() const;

    /**
     * @brief 将ITextComponent序列化为JSON字符串
     * @param text 文本组件
     * @return JSON字符串
     */
    static std::string serializeText(const text::ITextComponent& text);
};

} // namespace mc::skin
