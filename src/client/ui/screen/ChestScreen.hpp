#pragma once

#include "client/ui/screen/AbstractContainerScreen.hpp"
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "core/Types.hpp"

namespace mc {
class PlayerInventory;
}

namespace mc::client {

/**
 * @brief 箱子屏幕
 */
class ChestScreen : public AbstractContainerScreen<mc::blockentity::ChestContainer> {
public:
    /**
     * @brief 构造函数
     * @param containerId 容器ID
     * @param playerInventory 玩家背包
     * @param rows 箱子行数
     * @param clickSender 点击发送器
     * @param closeSender 关闭发送器
     */
    ChestScreen(ContainerId containerId,
        mc::PlayerInventory* playerInventory,
        i32 rows,
        ContainerClickSender clickSender = {},
        ContainerCloseSender closeSender = {});

    /**
     * @brief 获取屏幕标题
     */
    [[nodiscard]] std::string getTitle() const override { return "Chest"; }

protected:
    void onInit() override;
    void renderContainerBackground() override;
    void renderContainerForeground(i32 mouseX, i32 mouseY) override;
    void renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY) override;
    void renderTooltip(i32 mouseX, i32 mouseY) override;

private:
    static constexpr i32 GUI_WIDTH = 176;
    static constexpr i32 BASE_GUI_HEIGHT = 114;
    static constexpr i32 SLOT_SPACING = 18;
    static constexpr i32 TITLE_X = 8;
    static constexpr i32 TITLE_Y = 6;

    i32 m_rows;
};

} // namespace mc::client
