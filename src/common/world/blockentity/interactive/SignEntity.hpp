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

#include "util/text/ITextComponent.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <array>
#include <memory>
#include <string>

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
 * - 可编辑（右键点击）
 * - 木告示牌和荧光告示牌
 *
 * 参考: net.minecraft.tileentity.SignTileEntity
 */
class SignEntity : public BlockEntity {
public:
    /// 最大行数
    static constexpr i32 LINE_COUNT = 4;
    /// 每行最大字符数（纯文本）
    static constexpr i32 MAX_LINE_LENGTH = 15;

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
     * @brief 获取编辑者
     * @return 编辑者玩家指针，如果没有返回 nullptr
     */
    [[nodiscard]] Player* getEditor() const { return m_editor; }

    /**
     * @brief 设置编辑者
     * @param player 玩家
     */
    void setEditor(Player* player);

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
     * MC 1.16.5: 当玩家右键点击告示牌时，如果文本中包含
     * 点击事件（如 run_command），则执行该命令。
     *
     * 参考: SignTileEntity.executeCommand()
     *
     * @param world 世界引用
     * @param player 执行命令的玩家
     * @return 如果成功执行返回 true
     */
    bool executeCommand(IWorld& world, Player& player);

    /**
     * @brief 检查是否只有 OP 可以设置 NBT
     *
     * MC 1.16.5: 告示牌的 NBT 数据只能由 OP 级玩家修改。
     * 参考: SignTileEntity.onlyOpsCanSetNbt()
     *
     * @return 始终返回 true
     */
    [[nodiscard]] bool onlyOpsCanSetNbt() const { return true; }

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const override { return false; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    /**
     * @brief 验证文本组件有效性
     * @param text 文本组件
     * @return 如果有效返回 true
     */
    [[nodiscard]] static bool validateText(const text::ITextComponent& text);

    /**
     * @brief 截断文本组件到最大长度
     * @param text 文本组件
     * @return 截断后的文本组件
     */
    [[nodiscard]] static std::unique_ptr<text::ITextComponent> truncateText(std::unique_ptr<text::ITextComponent> text);

    std::array<std::unique_ptr<text::ITextComponent>, LINE_COUNT> m_lines; ///< 4行富文本
    bool m_editable = true;                                                ///< 是否可编辑
    Player* m_editor = nullptr;                                            ///< 当前编辑者
    i32 m_textColor = 0;                                                   ///< 文本颜色（DyeColor 值）
    bool m_glowing = false;                                                ///< 是否发光
};

} // namespace blockentity
} // namespace mc
