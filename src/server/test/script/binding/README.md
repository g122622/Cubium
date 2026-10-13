# GameTest 脚本绑定

## 目录结构

```text
binding/
├── ScriptTestHelper.*               # Test 的世界操作、断言、回调与清理
├── ScriptSimulatedPlayer.*          # 模拟玩家的属性及行为
├── ScriptRegistrationBuilder*      # 注册元数据与链式配置
├── ScriptGameTestFunction*          # 测试体调用及同步/异步结果
├── ScriptSequence*                 # 测试序列及回调桥接
└── ScriptGameTestError*             # 框架错误到脚本错误的转换
```

## 内部模块关系

绑定通过统一脚本上下文和对象注册表转发到 GameTest 门面；测试体、定时任务和清理回调的错误均转换为框架结果。

## 上下游外部依赖关系

依赖公共脚本接口、GameTest 门面及原生模拟玩家；由 `GameTestModuleBinding` 注册到测试宿主。

## 容易踩的坑

模拟玩家使用独立原型，通用实体属性须显式绑定，不能假设继承 `Entity.location`。跨维度迁移只转移实体所有权，句柄保持有效；真实析构前统一清空句柄。`succeedWhen` 用回调错误表示条件尚未满足，无异常返回会完成测试。
