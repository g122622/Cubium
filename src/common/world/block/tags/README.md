# 数据包方块标签

## 目录结构树

```text
tags/
├── BlockTagLoader.cpp  # 方块资源位置池
├── BlockTagLoader.hpp  # 方块标签 JSON 加载器
├── BlockTags.cpp  # 模块接口与实现
└── BlockTags.hpp  # 方块标签
```

## 内部模块关系

Block 定义共享行为入口，BlockState 维护不可变状态；BlockProperties 与方块族描述在注册时绑定。注册表持有方块和状态，原版装配位于 registry/vanilla。

## 上下游外部依赖关系

实体、物品、区块和服务端世界消费方块接口；几何运算使用 common/physics，状态属性使用 common/util/property。

## 容易踩的坑

- **访问器缓存指针会被 `clear()` 打失效**：`BlockTags::CLIMBABLE()` 等把 `BlockTag*` 缓存在 static 局部变量里。`RegistryBootstrap` 每次服务器启动都调 `clear()`，`_isLive()` 因此**绝不能解引用缓存指针**——对象已释放，`tag->getId()` 会读到垃圾字符串并在 `ResourceLocation::hash` 崩溃（同进程连跑多个服务器实例时实测）。当前实现：`clear()` 把清空内容后的对象移入退役区（`_getRetiredTags()`）冻结地址，`_getLiveTags()` 记录当前表内对象地址，守卫只查集合。新增访问器必须照抄该守卫。
- **标签内容 100% 来自数据包**：`initialize()` 不填任何编译期快照；`BlockTagLoader` 在启动期展开 `#tag` 引用后写入，改数据包即改行为。
- **加载顺序**：`BlockTagLoader` 必须在方块注册（`VanillaBlocks::initialize`）之后运行，否则标签中的方块名解析不到，整条标签被拒。
