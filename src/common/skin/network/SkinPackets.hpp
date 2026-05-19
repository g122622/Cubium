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
#include "common/network/packet/Packet.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include "common/util/text/ITextComponentFwd.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace mc::skin {

/**
 * @brief 玩家列表操作类型
 *
 * 参考 MC 1.16.5 SPlayerListItemPacket.Action
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
 * 存储单个玩家的信息，用于 PlayerListItemPacket。
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

    /**
     * @brief 创建添加玩家条目
     * @param profile 玩家档案
     * @param gameMode 游戏模式
     * @param ping 延迟
     */
    static PlayerListEntry createAdd(const GameProfile& profile, GameMode gameMode, i32 ping);

    /**
     * @brief 创建移除玩家条目
     * @param uuid 玩家UUID
     */
    static PlayerListEntry createRemove(const std::array<u8, 16>& uuid);

    /**
     * @brief 创建更新延迟条目
     * @param uuid 玩家UUID
     * @param ping 新延迟
     */
    static PlayerListEntry createUpdateLatency(const std::array<u8, 16>& uuid, i32 ping);

    /**
     * @brief 创建更新游戏模式条目
     * @param uuid 玩家UUID
     * @param gameMode 新游戏模式
     */
    static PlayerListEntry createUpdateGameMode(const std::array<u8, 16>& uuid, GameMode gameMode);

    /**
     * @brief 创建更新显示名条目
     * @param uuid 玩家UUID
     * @param displayName 显示名（JSON格式的ITextComponent），std::nullopt表示清除显示名
     */
    static PlayerListEntry createUpdateDisplayName(
        const std::array<u8, 16>& uuid, const std::optional<std::string>& displayName);

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

    // 序列化辅助方法
    void serialize(network::PacketSerializer& ser, PlayerListAction action) const;
    static Result<PlayerListEntry> deserialize(network::PacketDeserializer& deser, PlayerListAction action);
};

/**
 * @brief 玩家列表包
 *
 * 参考 MC 1.16.5 SPlayerListItemPacket
 *
 * 用于：
 * - 添加玩家（包含皮肤属性）
 * - 更新玩家信息（游戏模式、延迟、显示名）
 * - 移除玩家
 *
 * 网络格式（MC 1.16.5）：
 * - action: VarInt
 * - count: VarInt
 * - entries: [entry data based on action]
 *
 * AddPlayer 条目格式：
 * - UUID: 16 bytes
 * - Name: std::string (max 16)
 * - Properties count: VarInt
 * - Properties: [name, value, hasSignature, signature?]
 * - GameMode: VarInt
 * - Ping: VarInt
 * - HasDisplayName: Boolean
 * - DisplayName?: Chat (optional)
 */
class PlayerListItemPacket : public network::Packet {
public:
    PlayerListItemPacket();
    explicit PlayerListItemPacket(PlayerListAction action);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // 访问器
    [[nodiscard]] PlayerListAction action() const { return m_action; }
    void setAction(PlayerListAction action) { m_action = action; }

    [[nodiscard]] const std::vector<PlayerListEntry>& entries() const { return m_entries; }
    std::vector<PlayerListEntry>& entries() { return m_entries; }

    void addEntry(const PlayerListEntry& entry) { m_entries.push_back(entry); }
    void addEntry(PlayerListEntry&& entry) { m_entries.push_back(std::move(entry)); }
    void clearEntries() { m_entries.clear(); }

private:
    PlayerListAction m_action = PlayerListAction::AddPlayer;
    std::vector<PlayerListEntry> m_entries;
};

} // namespace mc::skin
