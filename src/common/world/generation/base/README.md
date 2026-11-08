# 本领域共享的基础类型与定义

## 目录结构树

```text
base/
└── DecorationStage.hpp  # 装饰阶段枚举
```

## 内部模块关系

base/DecorationStage 描述装饰次序；density/DensityFunction 提供生成器与气候采样共享的计算契约。阶段调度和具体生成算法均归服务端。

## 上下游外部依赖关系

公共群系设置与气候采样消费契约；server/world/gen 提供密度函数、结构与特征实现。

## 容易踩的坑

- 目录按领域职责组织；跨领域协作通过明确入口和基础类型。
- 新增或迁移源文件时同步更新所有消费目标的 CMake 清单。
