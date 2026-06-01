#pragma once

#include "common/core/Types.hpp"

#include <bitset>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本引擎能力标志
 *
 * 控制脚本运行时可执行的操作
 */
enum class Capability : u8 {
    AllowEval = 0,      // 允许使用 eval()
    ScriptOnly = 1,     // 仅脚本模式，禁止原生模块
};

/**
 * @brief 脚本能力集合
 */
class Capabilities {
public:
    Capabilities() = default;

    void setCapability(Capability cap, bool enabled) {
        m_capabilities[static_cast<size_t>(cap)] = enabled;
    }

    [[nodiscard]] bool hasCapability(Capability cap) const {
        return m_capabilities.test(static_cast<size_t>(cap));
    }

private:
    std::bitset<2> m_capabilities;
};

} // namespace mc::mod::bedrock::addon
