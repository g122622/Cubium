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

private:
    /** @brief 获取已登录、已完成加载的真实玩家实体。 */
    [[nodiscard]] Player* _player(PlayerId playerId) const;

    /** @brief 获取当前菜单，包含无外部容器时的玩家背包菜单，并校验有效性。 */
    [[nodiscard]] AbstractContainerMenu* _menu(PlayerId playerId, Player& player) const;
};

} // namespace mc::server::net
