# 脚本绑定基础设施

## 目录结构树

```text
binding/
├── IModuleBindingFactory.hpp     # 原生模块工厂接口
├── IScriptBindingContext.hpp     # 引擎无关值操作与上下文清理入口
├── ScriptClassBinding.hpp/cpp    # 对象包装、类注册与模块导出
├── ScriptClassRegistry.hpp/cpp   # 每个上下文独立的类名及原型索引
├── ScriptHandleRegistry.hpp/cpp  # 实体拥有的非拥有句柄失效管理
├── ScriptEntityReference.hpp/cpp # 排队事件的非拥有实体引用
├── ScriptGameObjects.hpp/cpp     # 实体、维度和物品的统一脚本包装
├── ScriptDamageCause.hpp/cpp     # 游戏伤害类型与脚本原因映射
├── ScriptBlockRef.hpp            # 方块位置及状态包装
├── ScriptValueHandle.hpp         # 持久脚本值的独立引用与上下文失效
└── ScriptCallbackHolder.hpp      # 回调函数的持有与调用
```

## 内部模块关系

绑定上下文拥有类注册表与资源清理回调。模块通过 NativeModuleBuilder 导出类，ScriptObjectRegistry 包装游戏对象。ScriptValueHandle 在上下文销毁前释放其值，ScriptCallbackHolder 在其上提供函数调用。

## 上下游外部依赖关系

公共模块、服务端插件和 GameTest 绑定消费本目录接口；QuickJS 后端实现绑定抽象。本目录依赖公共基础类型，不依赖服务端。

## 容易踩的坑

- 创建值和 getProperty 返回拥有句柄；setProperty 和 setArrayElement 复制值引用，不消耗调用方句柄，挂载后仍需释放。
- createClassProto 返回上下文拥有的借用句柄，调用方不得释放。
- retainValue 不复制句柄地址，不能延长原生参数包装句柄的寿命；跨调用保存值使用 ScriptValueHandle。
- 类名与原型仅在所属上下文内查找，不能把另一个插件的 classId 或原型用于当前上下文。
- 游戏实体与脚本对象生命周期独立。非拥有实体及其子对象按实际实体地址统一失效，实体编号不是进程级身份。
- 引擎后端必须在释放脚本上下文或值之前执行 disposeBindings。已经完成的资源应移除清理登记，避免长期积累。
