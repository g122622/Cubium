#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <deque>

namespace mc::client {

/**
 * @brief 待确认的玩家输入
 *
 * 记录客户端发送给服务端的输入，用于服务端确认后的位置校正。
 */
struct PendingInput {
    u32 sequence;       ///< 输入序列号
    Vector3 delta;      ///< 移动增量
    f32 yaw;            ///< 偏航角
    f32 pitch;          ///< 俯仰角
    bool jumping;       ///< 是否跳跃
    bool sneaking;      ///< 是否潜行
};

/**
 * @brief 客户端玩家预测器
 *
 * 处理本地玩家的客户端预测：
 * - 移动预测：立即响应用户输入
 * - 位置校正：服务端确认后校正偏差
 * - 平滑插值：避免校正时的跳变
 *
 * ## 设计原则
 *
 * 客户端预测允许玩家操作获得即时反馈，同时保持与服务端的状态同步。
 * 核心流程：
 * 1. 玩家输入 → 本地预测移动 → 发送输入给服务端
 * 2. 服务端处理 → 返回确认位置
 * 3. 客户端收到确认 → 与预测位置比较 → 校正偏差
 *
 * ## 使用场景
 *
 * ```cpp
 * // 处理玩家输入
 * predictor.handleMovementInput(forward, strafe, jumping, sneaking);
 *
 * // 每帧更新预测位置
 * predictor.tick(deltaTime);
 *
 * // 接收服务端确认
 * predictor.receiveServerPosition(position, yaw, pitch);
 *
 * // 获取预测位置用于渲染
 * Vector3 renderPos = predictor.predictedPosition();
 * ```
 *
 * ## 线程安全
 *
 * 此类不是线程安全的。调用者需要确保在正确的线程访问。
 */
class ClientPlayerPredictor {
public:
    /**
     * @brief 构造函数
     */
    ClientPlayerPredictor();

    /**
     * @brief 析构函数
     */
    ~ClientPlayerPredictor() = default;

    // 禁止拷贝
    ClientPlayerPredictor(const ClientPlayerPredictor&) = delete;
    ClientPlayerPredictor& operator=(const ClientPlayerPredictor&) = delete;

    // 允许移动
    ClientPlayerPredictor(ClientPlayerPredictor&&) = default;
    ClientPlayerPredictor& operator=(ClientPlayerPredictor&&) = default;

    // ========== 输入处理 ==========

    /**
     * @brief 处理玩家移动输入
     *
     * 立即更新预测位置，并将输入加入待确认队列。
     *
     * @param forward 前后移动 (-1.0 到 1.0)
     * @param strafe 左右移动 (-1.0 到 1.0)
     * @param jumping 是否跳跃
     * @param sneaking 是否潜行
     */
    void handleMovementInput(f32 forward, f32 strafe, bool jumping, bool sneaking);

    /**
     * @brief 处理视角旋转
     *
     * @param yaw 偏航角变化
     * @param pitch 俯仰角变化
     */
    void handleRotationInput(f32 deltaYaw, f32 deltaPitch);

    // ========== 服务端同步 ==========

    /**
     * @brief 接收服务端位置确认
     *
     * 当服务端确认玩家位置时调用。会比较预测位置与服务端位置，
     * 如果偏差超过阈值则进行校正。
     *
     * @param position 服务端确认的位置
     * @param yaw 服务端确认的偏航角
     * @param pitch 服务端确认的俯仰角
     */
    void receiveServerPosition(const Vector3& position, f32 yaw, f32 pitch);

    /**
     * @brief 设置服务端确认的输入序列号
     *
     * 服务端处理完输入后会返回最后处理的序列号，
     * 用于清理已确认的待确认输入队列。
     *
     * @param lastAckSequence 服务端最后确认的序列号
     */
    void acknowledgeInput(u32 lastAckSequence);

    // ========== 更新 ==========

    /**
     * @brief 每帧更新
     *
     * 更新预测位置和平滑校正。
     *
     * @param deltaTime 帧时间（秒）
     */
    void tick(f32 deltaTime);

    // ========== 状态查询 ==========

    /**
     * @brief 获取预测位置（用于渲染）
     * @return 预测位置
     */
    [[nodiscard]] Vector3 predictedPosition() const;

    /**
     * @brief 获取预测旋转（用于渲染）
     * @return pair<yaw, pitch>
     */
    [[nodiscard]] std::pair<f32, f32> predictedRotation() const;

    /**
     * @brief 获取服务端位置
     * @return 服务端确认的位置
     */
    [[nodiscard]] Vector3 serverPosition() const;

    /**
     * @brief 获取服务端旋转
     * @return pair<yaw, pitch>
     */
    [[nodiscard]] std::pair<f32, f32> serverRotation() const;

    /**
     * @brief 检查是否有服务端位置确认
     * @return true 如果已收到服务端位置
     */
    [[nodiscard]] bool hasServerPosition() const;

    /**
     * @brief 重置预测器状态
     *
     * 在玩家传送、死亡重生等情况下调用。
     *
     * @param position 新位置
     * @param yaw 新偏航角
     * @param pitch 新俯仰角
     */
    void reset(const Vector3& position, f32 yaw, f32 pitch);

    /**
     * @brief 清除所有待确认输入
     */
    void clearPendingInputs();

    /**
     * @brief 获取当前输入序列号
     * @return 当前序列号
     */
    [[nodiscard]] u32 currentSequence() const;

    /**
     * @brief 设置移动速度
     * @param speed 移动速度（方块/秒）
     */
    void setMovementSpeed(f32 speed);

    /**
     * @brief 设置位置校正阈值
     * @param threshold 校正阈值（方块距离）
     */
    void setCorrectionThreshold(f32 threshold);

private:
    /**
     * @brief 应用位置校正
     *
     * 当预测位置与服务端位置偏差过大时，进行平滑校正。
     */
    void applyCorrection();

    /**
     * @brief 计算下一帧的预测位置
     * @param deltaTime 帧时间
     */
    void updatePrediction(f32 deltaTime);

    /**
     * @brief 清理已确认的待确认输入
     * @param lastAckSequence 最后确认的序列号
     */
    void prunePendingInputs(u32 lastAckSequence);

    // ========== 成员变量 ==========

    // 预测位置（客户端预测）
    Vector3 m_predictedPosition;
    f32 m_predictedYaw = 0.0f;
    f32 m_predictedPitch = 0.0f;

    // 服务端确认位置
    Vector3 m_serverPosition;
    f32 m_serverYaw = 0.0f;
    f32 m_serverPitch = 0.0f;
    bool m_hasServerPosition = false;

    // 当前输入状态
    f32 m_inputForward = 0.0f;
    f32 m_inputStrafe = 0.0f;
    bool m_inputJumping = false;
    bool m_inputSneaking = false;

    // 待确认的输入队列
    std::deque<PendingInput> m_pendingInputs;
    u32 m_inputSequence = 0;

    // 移动参数
    f32 m_movementSpeed = 4.317f;  // MC 默认玩家移动速度（方块/秒）
    f32 m_correctionThreshold = 0.1f;  // 校正阈值（方块）
    f32 m_correctionRate = 0.2f;  // 校正速率（每秒校正的比例）

    // 校正状态
    bool m_isCorrecting = false;
    Vector3 m_correctionStart;
    Vector3 m_correctionTarget;
    f32 m_correctionProgress = 0.0f;
};

} // namespace mc::client
