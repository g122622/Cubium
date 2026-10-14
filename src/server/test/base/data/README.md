# 测试元数据与运行参数

## 目录结构

```text
data/
├── TestData.hpp/cpp  # 注册期测试元数据及 JSON 序列化
├── RetryOptions.hpp # 重试次数和停止条件
└── TestParameters.hpp # 运行期测试选择、旋转与网格参数
```

## 内部模块关系

TestData 描述单个测试；RetryOptions 表达重试策略；TestParameters 描述一次执行的选择和布局。

## 上下游外部依赖关系

上游为注册门面、脚本绑定及运行器；下游依赖基础坐标、方向和 JSON 类型。

## 容易踩的坑

- 注册期元数据与运行期参数须分别传递，结构旋转与本轮额外旋转不能混用。
- 无头 runner 按单个用例创建子批次，保留原批次回调和环境；测试并行由独立进程分片提供。
