#pragma once

#include "server/test/runner/reporter/TestReporter.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace mc::test {

/**
 * @brief JUnit XML 报告器。
 *
 * 对齐 Java `JUnitLikeTestReporter`：把测试结果写入 JUnit XML（`<testsuites><testsuite><testcase>`）。
 * - testcase `name=testId, classname=structure, time=runTime/1000.0`（tick 数 / 20 ticks/秒）。
 * - required 失败 → `<failure message="(blockPos) error">`；optional 失败 → `<skipped>`。
 *
 * 写入路径由构造参数指定（每实例须唯一，`-j16` 下用 `TempDirHelper` 各自临时目录）。
 */
class JUnitTestReporter final : public TestReporter {
public:
    explicit JUnitTestReporter(std::filesystem::path reportPath);

    void onTestPassed(const BaseGameTestInstance& test) override;
    void onTestFailed(const BaseGameTestInstance& test) override;
    void onBatchFinished(const MultipleTestTracker& tracker) override;
    void onAllFinished(const MultipleTestTracker& tracker) override;

    /**
     * @brief 是否在报告过程中遇到 IO 错误（写文件失败）。
     */
    [[nodiscard]] bool hasIoError() const noexcept { return m_ioError; }

private:
    struct _CaseRecord {
        std::string name;
        std::string classname;
        double timeSeconds = 0.0;
        bool passed = true;
        bool required = true;
        std::string failureMessage; // 空=通过/跳过；非空+required=failure；非空+!required=skipped
    };

    void _writeXml();
    static void _xmlEscape(std::string& s);

    std::filesystem::path m_reportPath;
    std::vector<_CaseRecord> m_records;
    bool m_ioError = false;
};

} // namespace mc::test
