#pragma once

namespace mc::test {

class GameTestBatch;

/**
 * @brief 批次级监听器。
 *
 * 对齐 Java 1.21.11 `GameTestBatchListener`：批次开始/结束时回调，与实例级 `IGameTestListener` 区分
 *（后者反应单测试状态，前者反应批次级生命周期）。由 runner 实现（如批次进度报告）。
 */
class GameTestBatchListener {
public:
    virtual ~GameTestBatchListener() = default;

    virtual void onBatchStarting(GameTestBatch& batch) = 0;
    virtual void onBatchFinished(GameTestBatch& batch) = 0;
};

} // namespace mc::test
