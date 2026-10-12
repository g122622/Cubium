# 报告绘图环境

## 目录结构

```text
setup-report-plots/
├── action.yml # 安装 Ubuntu 的 Matplotlib 与 Noto 中文字体
└── README.md  # 绘图环境依赖与字体约束
```

## 内部关系与外部依赖

benchmark 和 report job 通过此 action 安装绘图环境，`scripts/ci/report_plots.py` 使用无窗口后端生成 PNG 与 SVG。

## 容易踩的坑

- 图表包含中文标签，只有 Matplotlib 而没有中文字体会造成缺字。
- 图表渲染只在基准对比和汇总任务执行，不需要在编译 worker 安装绘图依赖。
