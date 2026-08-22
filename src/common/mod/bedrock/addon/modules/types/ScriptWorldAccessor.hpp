#pragma once

#include "common/core/Types.hpp"

#include <functional>
#include <string>
#include <vector>

namespace mc {

class IWorld; // 前向声明：getDimension 返回 IWorld*，仅需不完整类型

namespace scoreboard {
class Scoreboard; // 前向声明：getScoreboard 返回 Scoreboard*，仅需不完整类型
} // namespace scoreboard

namespace mod::bedrock::addon {

/**
 * @brief BossBar 只读快照（值类型，供脚本侧 world.bossbar 读取）
 *
 * BossBar 的 C++ 类型（CustomServerBossInfo/Manager）在 server 层，common 层无法直接持有其指针。
 * 故 ScriptWorldAccessor 以值快照方式桥接：server 层回调从 CustomServerBossInfo 填充本结构，
 * common 层脚本绑定据此构造 JS 对象。每次属性访问重新取快照以保证实时性（对齐 set value/max/color
 * 后 JS 立即可见）。exists=false 表示该 id 的 BossBar 已被 remove 或不存在。
 */
struct BossBarView {
    bool exists = false;
    std::string id;
    std::string name; // BossBar 显示名纯文本（经 ITextComponent::getUnformattedText 提取）
    i32 value = 0;
    i32 max = 0;
    std::string color;   // "pink"/"blue"/"red"/"green"/"yellow"/"purple"/"white"
    std::string overlay; // "progress"/"notched_6"/"notched_10"/"notched_12"/"notched_20"
    bool visible = true;
    std::vector<std::string> players; // 玩家名列表
};

/**
 * @brief 世界出生点只读快照（值类型，供脚本侧 world.getDefaultSpawn 读取）
 *
 * ServerWorld 在 server 层，common 层无法直接持有其指针。故 ScriptWorldAccessor 以值快照方式桥接：
 * server 层回调从 ServerWorld::worldSpawnPoint/spawnAngle 填充本结构，common 层脚本绑定据此构造 JS 对象。
 * 每次访问重新取快照保证 /setworldspawn 后 JS 立即可见。
 */
struct WorldSpawnView {
    bool exists = false; // 回调是否已注册（server 层注入）。未注册时 false，脚本侧返 undefined。
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    float angle = 0.0f; // 出生点朝向（度）
};

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
     * @brief 获取服务器记分板（ServerScoreboard，向上转为 Scoreboard 基类指针）。
     *
     * 供 world.scoreboard 脚本属性读取记分板，使 GameTest JS 能查询 objective/score 做断言。
     * GameTest 单服务器场景下记分板全局唯一。
     * @return 记分板指针；回调未注册返回 nullptr
     */
    [[nodiscard]] mc::scoreboard::Scoreboard* getScoreboard();

    /**
     * @brief 获取所有自定义 BossBar 的 id 列表（server 层回调遍历 CustomServerBossInfoManager::getIds）。
     *
     * 供 world.bossbar.getAll() 脚本方法。BossBar 类型在 server 层，common 层以值（id 字符串列表）桥接。
     * @return id 列表；回调未注册返回空
     */
    [[nodiscard]] std::vector<std::string> getBossBarIds();

    /**
     * @brief 按 id 取 BossBar 只读快照（server 层回调从 CustomServerBossInfo 填充 BossBarView）。
     *
     * 供 world.bossbar.get(id) 脚本方法及 BossBar JS 对象属性访问。每次访问重新取快照保证实时性。
     * @param id BossBar id（ResourceLocation 字符串，如 "mybar"）
     * @return 快照；BossBar 不存在时 exists=false
     */
    [[nodiscard]] BossBarView getBossBar(const std::string& id);

    /**
     * @brief 取世界出生点只读快照（server 层回调从 ServerWorld::worldSpawnPoint/spawnAngle 填充）。
     *
     * 供 world.getDefaultSpawn 脚本属性。ServerWorld 在 server 层，common 层以 WorldSpawnView 值桥接。
     * 每次访问重新取快照保证 /setworldspawn 后 JS 立即可见。
     * @return 快照；回调未注册时 exists=false
     */
    [[nodiscard]] WorldSpawnView getWorldSpawn();

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

    /**
     * @brief 设置获取记分板回调
     * @param callback 回调函数（返回服务器 Scoreboard*，无则 nullptr）
     */
    void setGetScoreboardCallback(std::function<mc::scoreboard::Scoreboard*()> callback);

    /**
     * @brief 设置获取 BossBar id 列表回调
     * @param callback 回调函数（返回所有自定义 BossBar 的 id 字符串列表）
     */
    void setGetBossBarIdsCallback(std::function<std::vector<std::string>()> callback);

    /**
     * @brief 设置按 id 取 BossBar 快照回调
     * @param callback 回调函数（按 id 返回 BossBarView 快照，不存在时 exists=false）
     */
    void setGetBossBarCallback(std::function<BossBarView(const std::string&)> callback);

    /**
     * @brief 设置取世界出生点快照回调
     * @param callback 回调函数（返回 WorldSpawnView 快照，未注册时 exists=false）
     */
    void setGetWorldSpawnCallback(std::function<WorldSpawnView()> callback);

private:
    ScriptWorldAccessor() noexcept = default;

    std::function<void(const std::string&)> m_messageCallback;
    std::function<std::vector<std::string>()> m_getPlayerNamesCallback;
    std::function<u64()> m_currentTickCallback;
    std::function<mc::IWorld*(const std::string&)> m_getDimensionCallback;
    std::function<mc::scoreboard::Scoreboard*()> m_getScoreboardCallback;
    std::function<std::vector<std::string>()> m_getBossBarIdsCallback;
    std::function<BossBarView(const std::string&)> m_getBossBarCallback;
    std::function<WorldSpawnView()> m_getWorldSpawnCallback;
};

} // namespace mod::bedrock::addon
} // namespace mc
