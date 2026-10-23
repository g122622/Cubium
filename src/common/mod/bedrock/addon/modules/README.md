# Minecraft 脚本模块绑定

## 目录结构树

```text
modules/
├── MinecraftModuleFactory.hpp/cpp # 注册 Minecraft 原生脚本模块及游戏对象
├── ScriptCustomComponentBinding.hpp/cpp # startup 提供的自定义组件注册入口
├── ScriptEventBinding.hpp/cpp # 事件信号订阅、取消和脚本消息
├── system/ # System 调度、生成器任务和脚本消息方法
├── entity/ # Entity 声明属性与包作用域动态属性
├── properties/ # 世界和实体共用的动态属性绑定及值转换
└── types/ # 向量、颜色转换和世界访问抽象
```

## 内部模块关系

模块工厂创建类原型和模块导出，按职责调用 System、Entity 和事件绑定。Player 从 Entity 继承，游戏对象统一经 binding 层包装。

## 上下游外部依赖关系

由脚本生命周期管理器注册；服务端通过 ScriptWorldAccessor 注入世界访问回调。绑定依赖公共世界、实体、物品、事件和调度设施，不依赖服务端实现。

## 容易踩的坑

- 类和原型从当前上下文的 classRegistry 查找，不可跨插件复用。
- 跨 tick 保存参数必须持有独立值句柄，不能保存临时参数包装地址。
- 自定义组件注册表通过 startup 事件提供，每个插件只持有本上下文入口。
- Entity 属性延迟应用，动态属性即时写入；动态属性的作用域来自清单 UUID。
- getComponent 的返回类型和写回行为需要同时成立，普通属性快照无法替代可写组件。
