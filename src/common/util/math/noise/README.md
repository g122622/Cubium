# 共享的确定性数学噪声

## 目录结构树

```text
noise/
├── PerlinSimplexNoise.cpp  # 模块接口与实现
├── PerlinSimplexNoise.hpp  # 多倍频 Simplex 噪声
├── SimplexNoise.cpp  # 模块接口与实现
└── SimplexNoise.hpp  # 2D Simplex 噪声
```

## 内部模块关系

SimplexNoise 提供基础采样，PerlinSimplexNoise 组合多个倍频；随机初始化使用已有 IRandom/JavaLegacyRandom，噪声对象构造后只读。

## 上下游外部依赖关系

生物群系温度与颜色、气候采样和服务端生成算法消费本目录；数学模块不依赖世界生成器实现。

## 容易踩的坑

- 目录按领域职责组织；跨领域协作通过明确入口和基础类型。
- 新增或迁移源文件时同步更新所有消费目标的 CMake 清单。
