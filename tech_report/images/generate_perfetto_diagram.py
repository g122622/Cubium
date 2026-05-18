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
    'game': '#1565C0',
    'perfetto': '#7B1FA2',
    'trace': '#388E3C',
    'buffer': '#FF9800',
    'output': '#D32F2F',
}

# ============ 左图：Perfetto架构 ============
ax1 = axes[0]
ax1.set_xlim(0, 16)
ax1.set_ylim(0, 12)
ax1.axis('off')
ax1.set_title('Perfetto Integration Architecture', fontsize=20, fontweight='bold', pad=15)

# 游戏代码
ax1.add_patch(FancyBboxPatch((1, 9), 14, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#E3F2FD', edgecolor='#1565C0', linewidth=2))
ax1.text(8, 10.5, 'Game Code', ha='center', va='center', fontsize=14, fontweight='bold', color='#1565C0')

# 追踪点示例
trace_points = [
    'TRACE_EVENT("rendering", "DrawChunk")',
    'TRACE_EVENT("world", "GenerateChunk")',
    'TRACE_EVENT("server", "TickEntities")',
]
for i, tp in enumerate(trace_points):
    ax1.text(8, 9.8 - i * 0.45, tp, ha='center', va='center', fontsize=10, family='monospace')

# Perfetto Manager
ax1.add_patch(FancyBboxPatch((1, 5.5), 14, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#F3E5F5', edgecolor='#7B1FA2', linewidth=2))
ax1.text(8, 7.3, 'Perfetto Manager (Singleton)', ha='center', va='center', fontsize=14, fontweight='bold', color='#7B1FA2')
ax1.text(8, 6.7, 'TrackEvent Data Source', ha='center', va='center', fontsize=11, color='#666666')
ax1.text(8, 6.2, 'Categories: rendering, world, server, game...', ha='center', va='center', fontsize=11, color='#666666')

# 箭头
ax1.annotate('', xy=(8, 8), xytext=(8, 9),
            arrowprops=dict(arrowstyle='->', color='#1565C0', lw=2.5))

# Buffer
ax1.add_patch(FancyBboxPatch((1, 2.5), 6.5, 2, boxstyle='round,pad=0.1',
                              facecolor='#FFF3E0', edgecolor='#FF9800', linewidth=2))
ax1.text(4.25, 4, 'Ring Buffer', ha='center', va='center', fontsize=13, fontweight='bold', color='#E65100')
ax1.text(4.25, 3.4, 'Default: 64MB', ha='center', va='center', fontsize=11, color='#666666')
ax1.text(4.25, 2.9, 'InProcess Mode', ha='center', va='center', fontsize=11, color='#666666')

# Output
ax1.add_patch(FancyBboxPatch((8.5, 2.5), 6.5, 2, boxstyle='round,pad=0.1',
                              facecolor='#FFEBEE', edgecolor='#D32F2F', linewidth=2))
ax1.text(11.75, 4, 'Output', ha='center', va='center', fontsize=13, fontweight='bold', color='#C62828')
ax1.text(11.75, 3.4, 'perfetto_trace.pb', ha='center', va='center', fontsize=11, family='monospace', color='#666666')
ax1.text(11.75, 2.9, 'JSON (for chrome://tracing)', ha='center', va='center', fontsize=11, color='#666666')

# 箭头
ax1.annotate('', xy=(4.25, 4.5), xytext=(4.25, 5.5),
            arrowprops=dict(arrowstyle='->', color='#7B1FA2', lw=2.5))
ax1.annotate('', xy=(11.75, 4.5), xytext=(11.75, 5.5),
            arrowprops=dict(arrowstyle='->', color='#7B1FA2', lw=2.5))

# 低开销说明
ax1.text(8, 1.5, 'Overhead: <1% (production mode)', ha='center', va='center', fontsize=12, color='#2E7D32', fontweight='bold')

# ============ 右图：追踪分类体系 ============
ax2 = axes[1]
ax2.set_xlim(0, 16)
ax2.set_ylim(0, 12)
ax2.axis('off')
ax2.set_title('Trace Category Hierarchy (50+ categories)', fontsize=20, fontweight='bold', pad=15)

# 渲染分类
ax2.add_patch(FancyBboxPatch((0.5, 8.5), 7, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#E8F5E9', edgecolor='#388E3C', linewidth=2))
ax2.text(4, 10.5, 'rendering.*', ha='center', va='center', fontsize=14, fontweight='bold', color='#2E7D32')
render_cats = ['frame', 'vulkan', 'chunk_mesh', 'uniform_update', 'chunk_draw']
for i, cat in enumerate(render_cats):
    ax2.text(4, 9.8 - i * 0.35, f'  - {cat}', ha='center', va='center', fontsize=10, color='#666666')

# 游戏逻辑分类
ax2.add_patch(FancyBboxPatch((8.5, 8.5), 7, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#E3F2FD', edgecolor='#1565C0', linewidth=2))
ax2.text(12, 10.5, 'game.*', ha='center', va='center', fontsize=14, fontweight='bold', color='#1565C0')
game_cats = ['tick', 'entity', 'physics', 'ai']
for i, cat in enumerate(game_cats):
    ax2.text(12, 9.8 - i * 0.35, f'  - {cat}', ha='center', va='center', fontsize=10, color='#666666')

# 世界分类
ax2.add_patch(FancyBboxPatch((0.5, 5), 7, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#FFF3E0', edgecolor='#FF9800', linewidth=2))
ax2.text(4, 7, 'world.*', ha='center', va='center', fontsize=14, fontweight='bold', color='#E65100')
world_cats = ['chunk_gen', 'chunk_load', 'biome', 'lighting']
for i, cat in enumerate(world_cats):
    ax2.text(4, 6.3 - i * 0.35, f'  - {cat}', ha='center', va='center', fontsize=10, color='#666666')

# 服务端分类
ax2.add_patch(FancyBboxPatch((8.5, 5), 7, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#FCE4EC', edgecolor='#C2185B', linewidth=2))
ax2.text(12, 7, 'server.*', ha='center', va='center', fontsize=14, fontweight='bold', color='#C2185B')
server_cats = ['tick', 'world', 'lighting', 'network']
for i, cat in enumerate(server_cats):
    ax2.text(12, 6.3 - i * 0.35, f'  - {cat}', ha='center', va='center', fontsize=10, color='#666666')

# 使用示例
ax2.add_patch(FancyBboxPatch((2, 0.5), 12, 3.5, boxstyle='round,pad=0.1',
                              facecolor='#F5F5F5', edgecolor='#616161', linewidth=1.5))
ax2.text(8, 3.5, 'Usage Example', ha='center', va='center', fontsize=13, fontweight='bold')
code = [
    'TRACE_EVENT_BEGIN("rendering", "DrawFrame");',
    'TRACE_EVENT_END("rendering");',
    'TRACE_COUNTER("memory", "chunk_count", chunks);',
]
for i, line in enumerate(code):
    ax2.text(8, 2.8 - i * 0.55, line, ha='center', va='center', fontsize=10, family='monospace')

plt.tight_layout()
plt.savefig('/Users/a0000/dev/minecraft-reborn/tech_report/images/perfetto_system.png',
            dpi=150, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()

print("Perfetto system diagram saved to images/perfetto_system.png")
