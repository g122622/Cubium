# 物品脚本绑定

## 目录结构树

```text
item/
└── ScriptItemPropertiesBinding.hpp/cpp # ItemStack 动态属性与 clone 方法
```

## 内部模块关系

物品属性绑定复用通用方法，读取物品当前快照，成功修改后提交到物品 custom_data。clone 使用统一物品包装入口创建独立副本。

## 上下游外部依赖关系

由 MinecraftModuleFactory 注册，依赖公共物品属性门面、脚本对象注册表与通用动态属性绑定。

## 容易踩的坑

- 参数 getter 可以重入脚本，完成转换后才读取物品快照，避免覆盖内层已经提交的数据。
- Container.getItem 返回副本，修改后需显式 setItem 才写回容器。
- 物品非拥有句柄必须由其持有方保证寿命，绑定每次调用均检查接收者类型。
