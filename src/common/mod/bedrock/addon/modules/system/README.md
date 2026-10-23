# System 脚本绑定

## 目录结构树

```text
system/
├── ScriptSystemBinding.hpp/cpp # System 调度、取消与生成器任务绑定
└── README.md                   # 依赖与回调生命周期约束
```

## 内部模块关系

绑定将 System 调用交给 ScriptScheduler；回调和生成器通过 ScriptValueHandle 持有独立引用。

## 上下游外部依赖关系

由 MinecraftModuleFactory 注册，依赖绑定抽象、调度器与世界 tick 访问入口。

## 容易踩的坑

- 脚本参数句柄仅在原生调用内存活，异步执行必须复制独立句柄。
- 上下文销毁前取消其任务并归还脚本引用，禁止回调访问已经释放的绑定上下文。
- 生成器采用轮转时间片；取消一个任务不能阻止其他任务本 tick 的执行机会。
