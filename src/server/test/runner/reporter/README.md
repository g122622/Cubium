# GameTest 报告

## 目录结构

```text
reporter/
├── TestReporter.hpp          # 用例开始、结果和批次生命周期接口
├── GlobalTestReporter.*      # 全局事件分发
├── LogTestReporter.*         # 带实际耗时的控制台结果
├── JUnitTestReporter.*       # 完整清单与原子更新的 JUnit 快照
└── FailedTestCollector.*     # 隔离重跑所需的失败名称
```

## 内部关系与外部依赖

runner 的实例监听器向 GlobalTestReporter 广播，具体报告器分别输出日志、文件和名称列表。
依赖 framework 的实例和函数、tracker 计数器以及 spdlog；GameTestServer 安装和移除报告器，
外层 Node 协调者与 CI Python 汇总器消费 JUnit。

## 容易踩的坑

- 选中清单须在执行前保存；未开始是 skipped，开始但未结束是 error。
- 开始事件先于同步测试体执行，避免先通过、再被开始事件改成中止。
- `time` 是实际耗时，包含共享主线程等待，不能当作独占 CPU 时间。
- 临时文件完整关闭后 rename；写文件失败必须使运行报错。
