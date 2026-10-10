#pragma once

#include "common/network/ir/IrPacket.hpp"
#include "server/network/play/base/PlayHandlerBase.hpp"

namespace mc {
class Player;
class AbstractContainerMenu;
} // namespace mc

namespace mc::server::net {

/** @brief 物品编辑与菜单专用操作的入站处理器。 */
class ContainerActionHandler : public PlayHandlerBase {
public:
    explicit ContainerActionHandler(MinecraftServer& server)
        : PlayHandlerBase(server)
    {}

    /** @brief 将重命名应用到仍有效的铁砧菜单。 */
    void handleRenameItemPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /** @brief 选择有效报价并从玩家背包填充支付槽。 */
    void handleSelectTradePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /** @brief 校验容器 ID、有效性及旁观者权限后分发菜单按钮。 */
    void handleContainerButtonClickPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /** @brief 校验书籍槽位和页数，保存编辑或生成服务端署名的成书。 */
    void handleEditBookPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /** @brief 选择菜单槽位中的收纳袋内容项，保留客户端瞬态选择。 */
    void handleBundleItemSelectedPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /** @brief 验证当前信标菜单后消耗支付并更新效果。 */
    void handleSetBeaconPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

private:
    /** @brief 获取已登录、已完成加载的真实玩家实体。 */
    [[nodiscard]] Player* _player(PlayerId playerId) const;

    /** @brief 获取当前菜单，包含无外部容器时的玩家背包菜单，并校验有效性。 */
    [[nodiscard]] AbstractContainerMenu* _menu(PlayerId playerId, Player& player) const;
};

} // namespace mc::server::net
