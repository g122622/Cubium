# 服务端脚本事件桥接

## 目录结构树

```text
bridge/
├── EventBridge.hpp/cpp       # 游戏事件快照、取消回传及订阅生命周期
└── ServerEventSignals.hpp/cpp # 官方脚本事件名与游戏事件类型关联
```

## 内部模块关系

EventBridge 订阅游戏事件，发布前置快照或排队后置快照；信号定义用于向脚本暴露对应名称。

## 上下游外部依赖关系

由 ServerScriptManager 初始化和关闭；依赖 ServerEventBus、公共 ScriptEventBus、ScriptEventData 与游戏对象包装。

## 容易踩的坑

- 前置分发返回取消结果后，必须回写原生事件的取消标志。
- 延迟载荷中的物品、字符串及伤害来源字段必须在原生事件有效期间转为快照。
- 玩家首次加入与重生使用统一 playerSpawn 脚本信号，通过 initialSpawn 区分。
- 关闭桥接时注销所有原生订阅，随后再销毁脚本上下文。
