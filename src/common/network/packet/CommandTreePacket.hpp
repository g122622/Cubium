#pragma once

#include "Packet.hpp"

namespace mc::network {

/**
 * @brief 命令树同步包
 *
 * 服务端在登录完成后发送给客户端，携带当前可用命令树的 JSON 快照。
 * 该包仅序列化包体，外层网络头由 ConnectionManager::encapsulatePacket 统一添加。
 */
class CommandTreePacket : public Packet {
public:
    CommandTreePacket()
        : Packet(PacketType::CommandTree) {}

    explicit CommandTreePacket(String treeJson)
        : Packet(PacketType::CommandTree)
        , m_treeJson(std::move(treeJson)) {}

    /**
        * @brief 获取命令树 JSON 包体
     */
    [[nodiscard]] const String& treeJson() const noexcept { return m_treeJson; }

    /**
        * @brief 设置命令树 JSON 包体
     */
    void setTreeJson(String treeJson) { m_treeJson = std::move(treeJson); }

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

private:
    String m_treeJson;
};

} // namespace mc::network
