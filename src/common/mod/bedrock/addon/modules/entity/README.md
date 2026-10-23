# 实体脚本绑定

## 目录结构树

```text
entity/
└── ScriptEntityPropertiesBinding.hpp/cpp # Entity 声明属性及动态属性方法
```

## 内部模块关系

模块工厂在 Entity 原型上安装属性方法，绑定统一解析实体、转换脚本值并调用实体属性组件。

## 上下游外部依赖关系

MinecraftModuleFactory 调用本目录，Player 通过 Entity 原型继承这些方法。绑定依赖引擎无关上下文、对象包装和实体属性组件。

## 容易踩的坑

- 方法每次调用均通过当前上下文解析实体，销毁后的包装对象不可继续访问。
- 动态属性以当前上下文的包 UUID 隔离，不能用脚本文件名作为作用域。
- null、undefined 和省略的动态属性值均表示删除。
