#pragma once

#include "common/core/Types.hpp"

#include <functional>
#include <string>
#include <vector>

namespace mc {

class IWorld; // 前向声明：getDimension 返回 IWorld*，仅需不完整类型

namespace mod::bedrock::addon {

/**
 * @brief 脚本世界访问器接口
 *
 * 提供world全局对象访问服务器状态的抽象接口。
 * 具体实现由服务端注入，避免common层对server层的依赖。
 *
 * 通过ScriptWorldAccessor::instance()全局单例访问，
 * 在ServerScriptManager初始化时设置实现。
 */
class ScriptWorldAccessor {
public:
    /**
     * @brief 获取全局单例
     */
    [[nodiscard]] static ScriptWorldAccessor& instance();

    /**
     * @brief 发送广播消息到所有玩家
     * @param message 消息内容
     */
    void sendMessage(const std::string& message);

    /**
     * @brief 获取所有在线玩家名称
     * @return 玩家名称列表
     */
    [[nodiscard]] std::vector<std::string> getAllPlayerNames();

    /**
     * @brief 获取当前游戏tick
     */
    [[nodiscard]] u64 currentTick() const;

    /**
     * @brief 获取指定维度的 IWorld（经 ServerDimensionManager 解析维度名）。
     * @param dimensionId 维度名字符串（如 "minecraft:overworld"/"minecraft:nether"/"minecraft:the_end"）
     * @return 对应维度的 IWorld*；维度不存在或回调未注册返回 nullptr
     */
    [[nodiscard]] mc::IWorld* getDimension(const std::string& dimensionId);

    /**
     * @brief 设置消息发送回调
     * @param callback 回调函数
     */
    void setMessageCallback(std::function<void(const std::string&)> callback);

    /**
     * @brief 设置获取玩家名称列表回调
     * @param callback 回调函数
     */
    void setGetPlayerNamesCallback(std::function<std::vector<std::string>()> callback);

    /**
     * @brief 设置获取当前tick回调
     * @param callback 回调函数
     */
    void setCurrentTickCallback(std::function<u64()> callback);

    /**
     * @brief 设置获取维度回调
     * @param callback 回调函数（按维度名返回 IWorld*，不存在返回 nullptr）
     */
    void setGetDimensionCallback(std::function<mc::IWorld*(const std::string&)> callback);

private:
    ScriptWorldAccessor() noexcept = default;

    std::function<void(const std::string&)> m_messageCallback;
    std::function<std::vector<std::string>()> m_getPlayerNamesCallback;
    std::function<u64()> m_currentTickCallback;
    std::function<mc::IWorld*(const std::string&)> m_getDimensionCallback;
};

} // namespace mod::bedrock::addon
} // namespace mc
