# QuickJS 引擎后端

## 目录结构

```text
quickjs/
├── QuickJSEngine.*          # 脚本引擎工厂与启动关闭
├── QuickJSRuntime.*         # 运行时和上下文管理
├── QuickJSContext.*         # 模块加载与脚本执行
├── QuickJSBindingContext.*  # 值句柄、类属性和原生调用
└── QuickJSModuleLoader.*    # 原生及源码模块解析
```

## 内部模块关系

引擎拥有运行时，运行时管理上下文；绑定上下文封装 QuickJS 值和类操作，模块加载器连接原生及行为包代码。

## 上下游外部依赖关系

依赖 QuickJS 与公共脚本接口；公共模块和服务端绑定只经抽象接口使用本后端。

## 容易踩的坑

值句柄须成对持有和释放。实体拥有的子对象按 `entityOwner` 注销，opaque 可以是背包地址，不能按 opaque 地址推断实体生命周期。所有 QuickJS 操作在所属脚本线程执行。
