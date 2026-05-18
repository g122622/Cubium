import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import Rectangle, FancyBboxPatch, FancyArrowPatch
import numpy as np

# 使用新罗马字体
plt.rcParams['font.family'] = 'serif'
plt.rcParams['font.serif'] = ['Times New Roman', 'Times', 'DejaVu Serif']
plt.rcParams['mathtext.fontset'] = 'stix'
plt.rcParams['axes.unicode_minus'] = False
plt.rcParams['font.size'] = 14

fig, axes = plt.subplots(1, 2, figsize=(16, 10))

# 颜色定义
colors = {
    'template': '#7B1FA2',
    'reactive': '#1565C0',
    'widget': '#388E3C',
    'layout': '#FF9800',
}

# ============ 左图：响应式状态管理 ============
ax1 = axes[0]
ax1.set_xlim(0, 16)
ax1.set_ylim(0, 12)
ax1.axis('off')
ax1.set_title('Reactive State Management', fontsize=20, fontweight='bold', pad=15)

# Reactive<T>
ax1.add_patch(FancyBboxPatch((1, 8), 6, 3, boxstyle='round,pad=0.1',
                              facecolor='#E3F2FD', edgecolor='#1565C0', linewidth=2))
ax1.text(4, 10.5, 'Reactive<T>', ha='center', va='center', fontsize=14, fontweight='bold', color='#1565C0')
ax1.text(4, 9.8, 'value: T', ha='center', va='center', fontsize=11, family='monospace', color='#666666')
ax1.text(4, 9.3, 'observers: list<Callback>', ha='center', va='center', fontsize=10, family='monospace', color='#666666')
ax1.text(4, 8.7, 'set(v): notify all', ha='center', va='center', fontsize=10, family='monospace', color='#666666')
ax1.text(4, 8.2, 'get(): return value', ha='center', va='center', fontsize=10, family='monospace', color='#666666')

# Computed<T>
ax1.add_patch(FancyBboxPatch((9, 8), 6, 3, boxstyle='round,pad=0.1',
                              facecolor='#E8F5E9', edgecolor='#388E3C', linewidth=2))
ax1.text(12, 10.5, 'Computed<T>', ha='center', va='center', fontsize=14, fontweight='bold', color='#2E7D32')
ax1.text(12, 9.8, 'dependencies: list<Reactive>', ha='center', va='center', fontsize=10, family='monospace', color='#666666')
ax1.text(12, 9.3, 'compute(): T', ha='center', va='center', fontsize=10, family='monospace', color='#666666')
ax1.text(12, 8.7, 'auto recompute on deps change', ha='center', va='center', fontsize=10, color='#666666')

# StateStore
ax1.add_patch(FancyBboxPatch((3, 3.5), 10, 3.5, boxstyle='round,pad=0.1',
                              facecolor='#FFF3E0', edgecolor='#FF9800', linewidth=2))
ax1.text(8, 6.3, 'StateStore', ha='center', va='center', fontsize=14, fontweight='bold', color='#E65100')
ax1.text(8, 5.6, 'batchUpdate(fn): collect changes', ha='center', va='center', fontsize=11, family='monospace', color='#666666')
ax1.text(8, 5.0, 'notify once after batch', ha='center', va='center', fontsize=11, color='#666666')
ax1.text(8, 4.4, 'Avoid multiple redraws', ha='center', va='center', fontsize=11, color='#666666')

# 箭头：Reactive -> StateStore
ax1.annotate('', xy=(4, 7), xytext=(4, 8),
            arrowprops=dict(arrowstyle='->', color='#1565C0', lw=2))
ax1.annotate('', xy=(12, 7), xytext=(12, 8),
            arrowprops=dict(arrowstyle='->', color='#388E3C', lw=2))

# 使用示例
ax1.add_patch(FancyBboxPatch((2, 0.3), 12, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#F5F5F5', edgecolor='#616161', linewidth=1.5))
ax1.text(8, 2.3, 'Example', ha='center', va='center', fontsize=12, fontweight='bold')
ax1.text(8, 1.7, 'Reactive<int> health(100);', ha='center', va='center', fontsize=10, family='monospace')
ax1.text(8, 1.2, 'health.set(80); // auto notify UI', ha='center', va='center', fontsize=10, family='monospace')
ax1.text(8, 0.7, 'Computed<string> status = health < 50 ? "LOW" : "OK";', ha='center', va='center', fontsize=10, family='monospace')

# ============ 右图：模板编译流程 ============
ax2 = axes[1]
ax2.set_xlim(0, 16)
ax2.set_ylim(0, 12)
ax2.axis('off')
ax2.set_title('Template Compilation Pipeline', fontsize=20, fontweight='bold', pad=15)

# 模板源码
ax2.add_patch(FancyBboxPatch((0.5, 9), 4, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#F3E5F5', edgecolor='#7B1FA2', linewidth=2))
ax2.text(2.5, 10.5, 'Template', ha='center', va='center', fontsize=13, fontweight='bold', color='#7B1FA2')
ax2.text(2.5, 9.8, '<Screen>', ha='center', va='center', fontsize=9, family='monospace')
ax2.text(2.5, 9.4, '  <Button', ha='center', va='center', fontsize=9, family='monospace')
ax2.text(2.5, 9.0, '    text="{{name}}"/>', ha='center', va='center', fontsize=9, family='monospace')

# 箭头
ax2.annotate('', xy=(5, 10.25), xytext=(4.5, 10.25),
            arrowprops=dict(arrowstyle='->', color='#424242', lw=2))

# Lexer
ax2.add_patch(FancyBboxPatch((5, 9.5), 2.5, 1.5, boxstyle='round,pad=0.1',
                              facecolor='#E0E0E0', edgecolor='#616161', linewidth=1.5))
ax2.text(6.25, 10.5, 'Lexer', ha='center', va='center', fontsize=12, fontweight='bold')
ax2.text(6.25, 10.0, 'Tokenize', ha='center', va='center', fontsize=10, color='#666666')

# 箭头
ax2.annotate('', xy=(8, 10.25), xytext=(7.5, 10.25),
            arrowprops=dict(arrowstyle='->', color='#424242', lw=2))

# Parser
ax2.add_patch(FancyBboxPatch((8, 9.5), 2.5, 1.5, boxstyle='round,pad=0.1',
                              facecolor='#E0E0E0', edgecolor='#616161', linewidth=1.5))
ax2.text(9.25, 10.5, 'Parser', ha='center', va='center', fontsize=12, fontweight='bold')
ax2.text(9.25, 10.0, 'Build AST', ha='center', va='center', fontsize=10, color='#666666')

# 箭头
ax2.annotate('', xy=(11, 10.25), xytext=(10.5, 10.25),
            arrowprops=dict(arrowstyle='->', color='#424242', lw=2))

# AST
ax2.add_patch(FancyBboxPatch((11, 9), 4.5, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#FFF8E1', edgecolor='#FFA000', linewidth=2))
ax2.text(13.25, 10.8, 'AST', ha='center', va='center', fontsize=13, fontweight='bold', color='#E65100')
ax2.text(13.25, 10.2, 'Document', ha='center', va='center', fontsize=10, family='monospace')
ax2.text(13.25, 9.7, '  - Screen', ha='center', va='center', fontsize=10, family='monospace')
ax2.text(13.25, 9.3, '    - Button', ha='center', va='center', fontsize=10, family='monospace')

# 编译器
ax2.add_patch(FancyBboxPatch((5, 5.5), 10.5, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#E8F5E9', edgecolor='#388E3C', linewidth=2))
ax2.text(10.25, 7.3, 'TemplateCompiler', ha='center', va='center', fontsize=14, fontweight='bold', color='#2E7D32')
ax2.text(10.25, 6.6, 'Compile AST to C++ widget tree', ha='center', va='center', fontsize=11, color='#666666')
ax2.text(10.25, 6.0, 'Resolve bindings, events, loops', ha='center', va='center', fontsize=11, color='#666666')

# 箭头
ax2.annotate('', xy=(10.25, 8), xytext=(10.25, 9),
            arrowprops=dict(arrowstyle='->', color='#FFA000', lw=2))

# CompiledTemplate
ax2.add_patch(FancyBboxPatch((5, 2), 10.5, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#E3F2FD', edgecolor='#1565C0', linewidth=2))
ax2.text(10.25, 3.8, 'CompiledTemplate', ha='center', va='center', fontsize=14, fontweight='bold', color='#1565C0')
ax2.text(10.25, 3.1, 'instantiate(context) -> Widget*', ha='center', va='center', fontsize=11, family='monospace', color='#666666')
ax2.text(10.25, 2.5, 'updateBindings(context) -> void', ha='center', va='center', fontsize=11, family='monospace', color='#666666')

# 箭头
ax2.annotate('', xy=(10.25, 4.5), xytext=(10.25, 5.5),
            arrowprops=dict(arrowstyle='->', color='#388E3C', lw=2))

# 特性说明
ax2.add_patch(FancyBboxPatch((0.5, 2), 4, 5, boxstyle='round,pad=0.1',
                              facecolor='#F5F5F5', edgecolor='#616161', linewidth=1.5))
ax2.text(2.5, 6.5, 'Features', ha='center', va='center', fontsize=13, fontweight='bold')
features = [
    'Binding: {{var}}',
    'Event: @click=handler',
    'Loop: for="item in list"',
    'Condition: if="expr"',
]
for i, f in enumerate(features):
    ax2.text(2.5, 5.7 - i * 0.7, f, ha='center', va='center', fontsize=11, family='monospace')

plt.tight_layout()
plt.savefig('/Users/a0000/dev/minecraft-reborn/tech_report/images/kagero_ui.png',
            dpi=150, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()

print("Kagero UI diagram saved to images/kagero_ui.png")
