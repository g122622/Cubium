---
name: mc-fix-ci-error
description: 用gh工具运行nightly-ci，逐个认领并收敛ci中遇到的各个问题，直到ci完全成功（不含任何错误）
---

## 任务简介

用gh工具读取上一次最新nightly-ci结果，或者运行一个新的nightly-ci，逐个认领并收敛ci中遇到的各个问题，直到ci完全成功（不含任何错误）。

先阅读 docs\CI.md

- 当你遇到单元测试问题，请执行 /mc-fix-test-error 这个skill
- 当你遇到其他类型的测试问题，请参考 /mc-fix-test-error 这个skill的思路，结合E:\dev\minecraft-reborn-branch-1\docs\test 下的相关文档解决

解决一个问题之后，直接提交并推送到main，不要走pr流程；不要本地编译运行，而是直接取消之前的旧ci并跑一个新的ci，然后观察结果。