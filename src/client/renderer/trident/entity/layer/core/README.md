# 核心层渲染器

本目录包含层渲染器系统的核心类。

## 文件说明

| 文件 | 描述 |
|------|------|
| `LayerRenderer.hpp` | 层渲染器基类模板 |

## LayerRenderer 基类

```cpp
template<typename TEntity>
class LayerRenderer {
public:
    virtual ~LayerRenderer() = default;
    
    virtual void render(
        TEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale
    ) = 0;
    
    virtual bool shouldRender(const TEntity& entity) const;
};
```

## 参考

- MC 1.16.5 LayerRenderer
