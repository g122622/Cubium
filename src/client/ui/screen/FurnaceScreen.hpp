#pragma once

#include "client/ui/screen/AbstractContainerScreen.hpp"
#include "common/world/blockentity/processing/FurnaceInventory.hpp"
#include "common/entity/inventory/container/FurnaceContainer.hpp"
#include "core/Types.hpp"

namespace mc {
class PlayerInventory;
}

namespace mc::client {

/**
 * @brief 熔炉屏幕
 */
class FurnaceScreen : public AbstractContainerScreen<mc::blockentity::FurnaceContainer> {
public:
    /**
     * @brief 构造函数
     * @param containerId 容器ID
     * @param playerInventory 玩家背包
     * @param clickSender 点击发送器
     * @param closeSender 关闭发送器
     */
    FurnaceScreen(ContainerId containerId,
                  mc::PlayerInventory* playerInventory,
                  ContainerClickSender clickSender = {},
                  ContainerCloseSender closeSender = {});

    /**
     * @brief 获取屏幕标题
     */
    [[nodiscard]] std::string getTitle() const override {
        return "Furnace";
    }

protected:
    void onInit() override;
    void renderContainerBackground() override;
    void renderContainerForeground(i32 mouseX, i32 mouseY) override;
    void renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY) override;
    void renderTooltip(i32 mouseX, i32 mouseY) override;

private:
    static constexpr i32 GUI_WIDTH = 176;
    static constexpr i32 GUI_HEIGHT = 166;
    static constexpr i32 TITLE_X = 8;
    static constexpr i32 TITLE_Y = 6;
};

} // namespace mc::client
