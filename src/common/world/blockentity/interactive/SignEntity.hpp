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
#include "common/world/block/BlockPos.hpp"
#include "util/nbt/Nbt.hpp"
#include "util/text/ITextComponent.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <array>
#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;
class Player;

namespace blockentity {

/**
 * @brief 告示牌方块实体
 *
 * 告示牌用于显示富文本，特点：
 * - 4行文本，每行最多15个字符
 * - 支持富文本（颜色、样式、点击事件等）
 * - 可编辑（右键点击），涂蜡后不可编辑
 * - 木告示牌和荧光告示牌
 */
class SignEntity : public BlockEntity {
public:
    /// 最大行数
    static constexpr i32 LINE_COUNT = 4;
    /// 每行最大字符数（纯文本）
    static constexpr i32 MAX_LINE_LENGTH = 15;
    /// 编辑者最大交互距离（MC Java 默认交互距离 + 4.0 格余量）
    static constexpr f32 MAX_EDIT_DISTANCE = 4.0f + 4.0f;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit SignEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~SignEntity() override;

    // ========== 文本接口 ==========

    /**
     * @brief 获取指定行的文本组件
     * @param line 行号 (0-3)
     * @return 文本组件指针，如果无效返回 nullptr
     */
    [[nodiscard]] const text::ITextComponent* getLine(i32 line) const;

    /**
     * @brief 设置指定行的文本组件
     * @param line 行号 (0-3)
     * @param text 文本组件（所有权转移）
     * @return 如果设置成功返回 true
     */
    bool setLine(i32 line, std::unique_ptr<text::ITextComponent> text);

    /**
     * @brief 设置指定行的纯文本（向后兼容）
     * @param line 行号 (0-3)
     * @param text 纯文本内容
     * @return 如果设置成功返回 true
     */
    bool setLineFromLegacy(i32 line, const std::string& text);

    /**
     * @brief 获取指定行的纯文本内容
     * @param line 行号 (0-3)
     * @return 纯文本内容
     */
    [[nodiscard]] std::string getLineText(i32 line) const;

    /**
     * @brief 获取指定行的格式化文本（§ 代码格式）
     * @param line 行号 (0-3)
     * @return 格式化文本
     */
    [[nodiscard]] std::string getLineFormatted(i32 line) const;

    /**
     * @brief 设置所有行文本
     * @param lines 文本组件数组
     */
    void setLines(std::array<std::unique_ptr<text::ITextComponent>, LINE_COUNT> lines);

    /**
     * @brief 清空所有文本
     */
    void clearLines();

    // ========== 编辑状态 ==========

    /**
     * @brief 检查是否可编辑
     * @return 如果可编辑返回 true
     */
    [[nodiscard]] bool isEditable() const { return m_editable; }

    /**
     * @brief 设置可编辑状态
     * @param editable 可编辑状态
     */
    void setEditable(bool editable);

    /**
     * @brief 检查告示牌文本是否可编辑
     *
     * 对应 MC Java 的 SignBlockEntity.hasEditableText()。
     * 仅当告示牌未涂蜡时返回 true（涂蜡后文本永久不可编辑）。
     * 此方法用于 SignBlock 在玩家右键交互时判断是否应打开编辑器。
     *
     * @return 如果文本可编辑返回 true
     */
    [[nodiscard]] bool hasEditableText() const { return !m_waxed; }

    // ========== 编辑者追踪 ==========
    // 参考 MC Java 的 SignBlockEntity.playerWhoMayEdit 机制：
    // 当玩家打开告示牌编辑器时，设置其 UUID 为允许编辑者；
    // 其他玩家在此期间无法涂蜡或编辑该告示牌。
    // tick() 中定期检查编辑者是否距离过远或已离线，自动清除编辑锁。

    /**
     * @brief 检查是否有其他玩家正在编辑此告示牌
     *
     * 对应 MC Java 的 SignBlock.otherPlayerIsEditingSign()。
     * 当 playerWhoMayEdit 已设置且与当前交互玩家不同时返回 true。
     *
     * @param player 当前交互的玩家
     * @return 如果另一玩家正在编辑返回 true
     */
    [[nodiscard]] bool otherPlayerIsEditing(const Player& player) const;

    /**
     * @brief 获取当前允许编辑的玩家 UUID
     *
     * 对应 MC Java 的 SignBlockEntity.getPlayerWhoMayEdit()。
     * 返回空字符串表示没有玩家正在编辑。
     *
     * @return 编辑者 UUID，空字符串表示无编辑者
     */
    [[nodiscard]] const std::string& getPlayerWhoMayEdit() const { return m_playerWhoMayEdit; }

    /**
     * @brief 设置允许编辑的玩家
     *
     * 对应 MC Java 的 SignBlockEntity.setAllowedPlayerEditor()。
     * 当玩家打开告示牌编辑器时调用，将 UUID 记录为当前编辑者。
     * 传入空字符串清除编辑锁。
     *
     * @param uuid 玩家 UUID，空字符串表示清除编辑者
     */
    void setAllowedPlayerEditor(const std::string& uuid);

    /**
     * @brief 清除当前编辑者
     *
     * 便捷方法，等价于 setAllowedPlayerEditor("")。
     */
    void clearAllowedPlayerEditor() { setAllowedPlayerEditor(""); }

    /**
     * @brief 检查编辑者是否距离过远或已离线
     *
     * 对应 MC Java 的 SignBlockEntity.playerIsTooFarAwayToEdit()。
     * 当编辑者不在交互范围内（标准交互距离 + 4.0 格）或已离线时返回 true。
     *
     * @param world 世界引用
     * @param uuid 编辑者 UUID
     * @return 如果编辑者无法继续编辑返回 true
     */
    [[nodiscard]] bool playerIsTooFarAwayToEdit(IWorld& world, const std::string& uuid) const;

    // ========== 颜色 ==========

    /**
     * @brief 获取文本颜色
     * @return 文本颜色（DyeColor 值）
     */
    [[nodiscard]] i32 getTextColor() const { return m_textColor; }

    /**
     * @brief 设置文本颜色
     * @param color 文本颜色（DyeColor 值）
     */
    void setTextColor(i32 color);

    // ========== 涂蜡状态 ==========

    /**
     * @brief 检查告示牌是否已涂蜡
     *
     * 涂蜡后的告示牌文字不可编辑，玩家尝试编辑时播放失败音效。
     *
     * @return 如果已涂蜡返回 true
     */
    [[nodiscard]] bool isWaxed() const { return m_waxed; }

    /**
     * @brief 设置告示牌的涂蜡状态
     *
     * @param waxed 涂蜡状态
     * @return 如果状态发生了变化返回 true（即之前未涂蜡→涂蜡，或涂蜡→未涂蜡）
     */
    bool setWaxed(bool waxed);

    // ========== 光照 ==========

    /**
     * @brief 检查是否发光
     * @return 如果发光返回 true
     */
    [[nodiscard]] bool isGlowing() const { return m_glowing; }

    /**
     * @brief 设置发光状态
     * @param glowing 发光状态
     */
    void setGlowing(bool glowing);

    // ========== 命令执行 ==========

    /**
     * @brief 执行告示牌上的命令
     *
     * 当玩家右键点击告示牌时，如果文本中包含点击事件（如 run_command），
     * 则执行该命令。
     *
     * @param world 世界引用
     * @param player 执行命令的玩家
     * @return 如果成功执行返回 true
     */
    bool executeCommand(IWorld& world, Player& player);

    /**
     * @brief 检查是否只有 OP 可以设置 NBT
     *
     * 告示牌的 NBT 数据只能由 OP 级玩家修改。
     *
     * @return 始终返回 true
     */
    [[nodiscard]] bool onlyOpsCanSetNbt() const noexcept override { return true; }

    // ========== Tick 更新 ==========

    /**
     * @brief 每刻更新
     *
     * 检查当前编辑者是否距离过远或已离线，如果是则清除编辑锁。
     * 对应 MC Java 的 SignBlockEntity.tick()。
     *
     * @param world 世界引用
     */
    void tick(IWorld& world) override;

    /**
     * @brief 是否需要 tick 更新
     *
     * 仅当有玩家正在编辑时需要 tick，以便定期检查编辑者是否离线或走远。
     * 无编辑者时不需要 tick，避免性能开销。
     *
     * @return 如果有编辑者返回 true
     */
    [[nodiscard]] bool needsTick() const noexcept override { return !m_playerWhoMayEdit.empty(); }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    bool loadFromNBT(const nbt::CompoundTag& tag) override;
    void saveToNBT(nbt::CompoundTag& tag) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    /**
     * @brief 验证文本组件有效性
     * @param text 文本组件
     * @return 如果有效返回 true
     */
    [[nodiscard]] static bool _validateText(const text::ITextComponent& text);

    /**
     * @brief 截断文本组件到最大长度
     * @param text 文本组件
     * @return 截断后的文本组件
     */
    [[nodiscard]] static std::unique_ptr<text::ITextComponent> _truncateText(
        std::unique_ptr<text::ITextComponent> text);

    std::array<std::unique_ptr<text::ITextComponent>, LINE_COUNT> m_lines; ///< 4行富文本
    bool m_editable = true;                                                ///< 是否可编辑
    std::string m_playerWhoMayEdit; ///< 当前允许编辑的玩家 UUID（空字符串表示无编辑者）
    i32 m_textColor = 0;            ///< 文本颜色（DyeColor 值）
    bool m_glowing = false;         ///< 是否发光
    bool m_waxed = false;           ///< 是否已涂蜡（涂蜡后不可编辑）
};

} // namespace blockentity
} // namespace mc
