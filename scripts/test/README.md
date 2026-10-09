# 测试协调脚本

## 目录结构

```text
test/
├── run-gametests.ts          # 全量 GameTest、失败隔离重跑和 JUnit 聚合
├── gametest-process.ts       # 按绝对期限启动和终止服务端进程
├── tests/                   # 脚本回归检查
├── run_diff.ts              # 原版与项目行为差异检查
├── setup.ts                 # 测试环境准备
├── _bedrock_single.ts       # 单项基岩行为验证
├── _fix_structure_origin.mjs # 结构原点修正工具
└── _rebuild_creeper_pit.ts   # 测试结构重建工具
```

## 内部关系与外部依赖

GameTest 协调者调用进程期限工具，读取服务端 JUnit，聚合各轮结果。
nightly 和本地开发调用这些脚本；依赖 Node.js 标准库以及构建后的服务端。
CI 汇总脚本读取输出目录和日志。

## 容易踩的坑

- 全量运行与隔离重跑共享 30 分钟期限；流水线错误返回 2，普通用例失败返回 1。
- 缺失或不完整的进程报告必须报错；skipped 和 error 不能转换为通过。
- `--shards` 当前重复执行同一个 filter，尚未提供互不重叠的分片。
- Unix 超时终止进程组；Windows 直接终止原生服务端进程，避免启动 taskkill 的延迟。
