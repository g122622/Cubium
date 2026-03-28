#pragma once

#include "world/blockentity/BlockEntity.hpp"
#include <array>
#include <string>

namespace mc {

class IWorld;
class Player;

namespace blockentity {

/**
 * @brief 告示牌方块实体
 *
 * 告示牌用于显示文本，特点：
 * - 4行文本，每行最多15个字符
 * - 支持彩色文本（使用§代码）
 * - 可编辑（右键点击）
 * - 木告示牌和苂石告示牌
 *
 * 参考: net.minecraft.tileentity.SignTileEntity
 */
class SignEntity : public BlockEntity {
public:
    /// 最大行数
    static constexpr i32 LINE_COUNT = 4;
    /// 每行最大字符数
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
     * @brief 获取指定行的文本
     * @param line 行号 (0-3)
     * @return 文本内容
     */
    [[nodiscard]] const String& getLine(i32 line) const;

    /**
     * @brief 设置指定行的文本
     * @param line 行号 (0-3)
     * @param text 文本内容
     * @return 如果设置成功返回true
     */
    bool setLine(i32 line, const String& text);

    /**
     * @brief 获取所有行文本
     * @return 文本数组
     */
    [[nodiscard]] const std::array<String, LINE_COUNT>& getLines() const { return m_lines; }

    /**
     * @brief 设置所有行文本
     * @param lines 文本数组
     */
    void setLines(const std::array<String, LINE_COUNT>& lines);

    /**
     * @brief 清空所有文本
     */
    void clearLines();

    // ========== 编辑状态 ==========

    /**
     * @brief 检查是否可编辑
     * @return 如果可编辑返回true
     */
    [[nodiscard]] bool isEditable() const { return m_editable; }

    /**
     * @brief 设置可编辑状态
     * @param editable 可编辑状态
     */
    void setEditable(bool editable);

    /**
     * @brief 获取编辑者
     * @return 编辑者玩家指针，如果没有返回nullptr
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
     * @return 文本颜色
     */
    [[nodiscard]] i32 getTextColor() const { return m_textColor; }

    /**
     * @brief 设置文本颜色
     * @param color 文本颜色
     */
    void setTextColor(i32 color);

    // ========== 光照 ==========

    /**
     * @brief 检查是否发光
     * @return 如果发光返回true
     */
    [[nodiscard]] bool isGlowing() const { return m_glowing; }

    /**
     * @brief 设置发光状态
     * @param glowing 发光状态
     */
    void setGlowing(bool glowing);

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const override { return false; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    /**
     * @brief 验证文本有效性
     * @param text 待验证文本
     * @return 如果有效返回true
     */
    [[nodiscard]] static bool validateText(const String& text);

    /**
     * @brief 截断文本到最大长度
     * @param text 输入文本
     * @return 截断后的文本
     */
    [[nodiscard]] static String truncateText(const String& text);

    std::array<String, LINE_COUNT> m_lines;  ///< 4行文本
    bool m_editable = true;                   ///< 是否可编辑
    Player* m_editor = nullptr;               ///< 当前编辑者
    i32 m_textColor = 0;                      ///< 文本颜色（黑色）
    bool m_glowing = false;                   ///< 是否发光
};

} // namespace blockentity
} // namespace mc
