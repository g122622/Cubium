# 测试指南

## 目录结构

```text
test/
├── UNIT_TEST.md                       # 单元测试与 CTest
├── INTEGRATED_TEST.md                  # GameTest 运行、期限与报告
├── INTEGRATED_TEST_MIGRATION_TO_TS.md   # 集成脚本迁移说明
├── STRUCTURES.md                       # 测试结构资源
├── E2E_BOT_TEST.md                      # 网络客户端端到端测试
├── COMMAND_DIFF_TEST.md                  # 指令端到端 diff（Cubium vs vanilla）
├── STRESS_TEST.md                       # 机器人压力测试、内存与 tick 观测
└── FUZZING.md                          # 协议模糊测试
```

## 内部关系与外部依赖

测试指南说明 tests 与 scripts/test 的运行方式，CI.md 汇总自动化流程。
单元测试依赖 GoogleTest，集成测试依赖原生服务端与行为包，端到端测试依赖 bot 客户端；
指令 diff 额外依赖 vanilla 服务端 jar，日常回归可用冻结基线免启动 vanilla。

## 容易踩的坑

- 不把 job 成功当作所有用例通过；检查 JUnit 的 failure、error 和 skipped。
- GameTest 的 tick 是游戏进度，实际时间期限由独立看门狗约束。
- 本地只跑针对性用例，避免运行完整巨量测试集合。
