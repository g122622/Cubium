import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import Rectangle, FancyBboxPatch
import numpy as np

# 使用新罗马字体，增大字号
plt.rcParams['font.family'] = 'serif'
plt.rcParams['font.serif'] = ['Times New Roman', 'Times', 'DejaVu Serif']
plt.rcParams['mathtext.fontset'] = 'stix'
plt.rcParams['axes.unicode_minus'] = False
plt.rcParams['font.size'] = 14

fig, axes = plt.subplots(2, 1, figsize=(16, 10))

# 颜色定义
colors = {
    'critical': '#D32F2F',     # 红色 - Critical
    'high': '#F57C00',          # 橙色 - High
    'normal': '#388E3C',        # 绿色 - Normal
    'low': '#1976D2',           # 蓝色 - Low
    'background': '#7B1FA2',    # 紫色 - Background
    'io': '#0097A7',            # 青色 - IO
    'mesh': '#C2185B',          # 粉色 - Mesh
}

# ============ 上图：服务端甘特图 ============
ax1 = axes[0]
ax1.set_xlim(0, 20)
ax1.set_ylim(0, 6)
ax1.set_xlabel('Time (ms)', fontsize=16, fontweight='bold')
ax1.set_title('Server Thread Pools - Task Scheduling Gantt Chart', fontsize=20, fontweight='bold', pad=15)

# 绘制线程泳道
thread_labels = ['Compute\nThread 1', 'Compute\nThread 2', 'Compute\nThread 3', 'IO\nThread 1', 'IO\nThread 2']
thread_positions = [4.5, 3.5, 2.5, 1.5, 0.5]

for i, (label, pos) in enumerate(zip(thread_labels, thread_positions)):
    ax1.text(-0.5, pos, label, ha='right', va='center', fontsize=14, fontweight='bold')
    ax1.axhline(y=pos + 0.5, color='#E0E0E0', linestyle='-', linewidth=1)

# 分隔线：计算池和IO池
ax1.axhline(y=2.0, color='#424242', linestyle='--', linewidth=2)
ax1.text(21, 3.5, 'Compute Pool', ha='left', va='center', fontsize=15, fontweight='bold', color='#388E3C')
ax1.text(21, 1.0, 'IO Pool', ha='left', va='center', fontsize=15, fontweight='bold', color='#0097A7')

# 计算池任务 (甘特条)
# Thread 1
ax1.add_patch(Rectangle((0.5, 4.1), 3.5, 0.8, facecolor=colors['high'], edgecolor='black', linewidth=1.5))
ax1.text(2.25, 4.5, 'ChunkGen\n(nearby)', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((4.2, 4.1), 2.0, 0.8, facecolor=colors['normal'], edgecolor='black', linewidth=1.5))
ax1.text(5.2, 4.5, 'LightCalc', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((6.5, 4.1), 4.0, 0.8, facecolor=colors['high'], edgecolor='black', linewidth=1.5))
ax1.text(8.5, 4.5, 'ChunkGen (player)', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((10.8, 4.1), 3.0, 0.8, facecolor=colors['low'], edgecolor='black', linewidth=1.5))
ax1.text(12.3, 4.5, 'Population', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

# Thread 2
ax1.add_patch(Rectangle((0.5, 3.1), 2.5, 0.8, facecolor=colors['normal'], edgecolor='black', linewidth=1.5))
ax1.text(1.75, 3.5, 'LightCalc', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((3.2, 3.1), 3.8, 0.8, facecolor=colors['high'], edgecolor='black', linewidth=1.5))
ax1.text(5.1, 3.5, 'ChunkGen (nearby)', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((7.2, 3.1), 2.5, 0.8, facecolor=colors['normal'], edgecolor='black', linewidth=1.5))
ax1.text(8.45, 3.5, 'LightCalc', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((10.0, 3.1), 4.0, 0.8, facecolor=colors['background'], edgecolor='black', linewidth=1.5))
ax1.text(12.0, 3.5, 'WorldImport', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

# Thread 3
ax1.add_patch(Rectangle((0.5, 2.1), 4.0, 0.8, facecolor=colors['low'], edgecolor='black', linewidth=1.5))
ax1.text(2.5, 2.5, 'ChunkGen (far)', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((4.8, 2.1), 2.0, 0.8, facecolor=colors['normal'], edgecolor='black', linewidth=1.5))
ax1.text(5.8, 2.5, 'LightCalc', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((7.0, 2.1), 3.5, 0.8, facecolor=colors['low'], edgecolor='black', linewidth=1.5))
ax1.text(8.75, 2.5, 'ChunkGen (far)', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

# IO池任务
# Thread 1
ax1.add_patch(Rectangle((0.5, 1.1), 2.0, 0.8, facecolor=colors['io'], edgecolor='black', linewidth=1.5))
ax1.text(1.5, 1.5, 'ChunkLoad', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((2.8, 1.1), 1.5, 0.8, facecolor=colors['critical'], edgecolor='black', linewidth=1.5))
ax1.text(3.55, 1.5, 'Save', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((4.6, 1.1), 2.0, 0.8, facecolor=colors['io'], edgecolor='black', linewidth=1.5))
ax1.text(5.6, 1.5, 'DBRead', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((6.9, 1.1), 2.5, 0.8, facecolor=colors['io'], edgecolor='black', linewidth=1.5))
ax1.text(8.15, 1.5, 'ChunkLoad', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

# Thread 2
ax1.add_patch(Rectangle((0.5, 0.1), 1.8, 0.8, facecolor=colors['io'], edgecolor='black', linewidth=1.5))
ax1.text(1.4, 0.5, 'DBWrite', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((2.6, 0.1), 2.2, 0.8, facecolor=colors['io'], edgecolor='black', linewidth=1.5))
ax1.text(3.7, 0.5, 'ChunkLoad', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((5.1, 0.1), 1.5, 0.8, facecolor=colors['critical'], edgecolor='black', linewidth=1.5))
ax1.text(5.85, 0.5, 'Save', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.add_patch(Rectangle((6.9, 0.1), 3.0, 0.8, facecolor=colors['background'], edgecolor='black', linewidth=1.5))
ax1.text(8.4, 0.5, 'Snapshot', ha='center', va='center', fontsize=14, color='white', fontweight='bold')

ax1.set_xticks(range(0, 21, 2))
ax1.spines['top'].set_visible(False)
ax1.spines['right'].set_visible(False)

# ============ 下图：客户端甘特图 ============
ax2 = axes[1]
ax2.set_xlim(0, 20)
ax2.set_ylim(0, 4)
ax2.set_xlabel('Time (ms)', fontsize=16, fontweight='bold')
ax2.set_title('Client Mesh Worker Pool - FIFO Execution with Scheduler Reordering', fontsize=20, fontweight='bold', pad=15)

# 线程泳道
mesh_labels = ['Mesh\nThread 1', 'Mesh\nThread 2', 'Scheduler\n(Priority)', 'Main\nThread']
mesh_positions = [3.5, 2.5, 1.0, 0.0]

for i, (label, pos) in enumerate(zip(mesh_labels, mesh_positions)):
    ax2.text(-0.5, pos + 0.4, label, ha='right', va='center', fontsize=14, fontweight='bold')

# 分隔线
ax2.axhline(y=2.0, color='#E0E0E0', linestyle='-', linewidth=1)
ax2.text(21, 3.0, 'Mesh Workers', ha='left', va='center', fontsize=15, fontweight='bold', color='#C2185B')
ax2.text(21, 0.5, 'Control', ha='left', va='center', fontsize=15, fontweight='bold', color='#616161')

# Mesh Thread 1
ax2.add_patch(Rectangle((0.5, 3.1), 4.0, 0.8, facecolor=colors['mesh'], edgecolor='black', linewidth=1.5))
ax2.text(2.5, 3.5, 'Build Chunk A (in frustum)', ha='center', va='center', fontsize=13, color='white', fontweight='bold')

ax2.add_patch(Rectangle((4.8, 3.1), 3.5, 0.8, facecolor=colors['mesh'], edgecolor='black', linewidth=1.5))
ax2.text(6.55, 3.5, 'Build Chunk C (front)', ha='center', va='center', fontsize=13, color='white', fontweight='bold')

ax2.add_patch(Rectangle((8.6, 3.1), 4.0, 0.8, facecolor=colors['mesh'], edgecolor='black', linewidth=1.5))
ax2.text(10.6, 3.5, 'Build Chunk E (near)', ha='center', va='center', fontsize=13, color='white', fontweight='bold')

# Mesh Thread 2
ax2.add_patch(Rectangle((0.5, 2.1), 3.5, 0.8, facecolor=colors['mesh'], edgecolor='black', linewidth=1.5))
ax2.text(2.25, 2.5, 'Build Chunk B (in frustum)', ha='center', va='center', fontsize=13, color='white', fontweight='bold')

ax2.add_patch(Rectangle((4.3, 2.1), 4.0, 0.8, facecolor=colors['mesh'], edgecolor='black', linewidth=1.5))
ax2.text(6.3, 2.5, 'Build Chunk D (in frustum)', ha='center', va='center', fontsize=13, color='white', fontweight='bold')

ax2.add_patch(Rectangle((8.6, 2.1), 3.0, 0.8, facecolor=colors['mesh'], edgecolor='black', linewidth=1.5))
ax2.text(10.1, 2.5, 'Build Chunk F', ha='center', va='center', fontsize=13, color='white', fontweight='bold')

# Scheduler 优先级队列
ax2.add_patch(Rectangle((0.5, 0.8), 12.0, 0.6, facecolor='#E0E0E0', edgecolor='black', linewidth=1.5))
ax2.text(6.5, 1.1, 'Priority Queue: A(score=-105) > B(-103) > C(-98) > D(-95) > E(-80) > F(15)',
         ha='center', va='center', fontsize=13, fontweight='bold')

# 取消标记
ax2.annotate('Cancel\n(behind)', xy=(7.5, 1.4), xytext=(9.5, 1.8),
            fontsize=12, ha='center', color='red',
            arrowprops=dict(arrowstyle='->', color='red', lw=1.5))

# 主线程
ax2.add_patch(Rectangle((0.5, 0.0), 2.0, 0.5, facecolor='#9E9E9E', edgecolor='black', linewidth=1))
ax2.text(1.5, 0.25, 'Frame N', ha='center', va='center', fontsize=12, color='white', fontweight='bold')
ax2.add_patch(Rectangle((2.7, 0.0), 2.0, 0.5, facecolor='#9E9E9E', edgecolor='black', linewidth=1))
ax2.text(3.7, 0.25, 'Frame N+1', ha='center', va='center', fontsize=12, color='white', fontweight='bold')
ax2.add_patch(Rectangle((4.9, 0.0), 2.0, 0.5, facecolor='#9E9E9E', edgecolor='black', linewidth=1))
ax2.text(5.9, 0.25, 'Frame N+2', ha='center', va='center', fontsize=12, color='white', fontweight='bold')

ax2.set_xticks(range(0, 21, 2))
ax2.spines['top'].set_visible(False)
ax2.spines['right'].set_visible(False)

# 图例
legend_elements = [
    mpatches.Patch(facecolor=colors['critical'], edgecolor='black', label='Critical'),
    mpatches.Patch(facecolor=colors['high'], edgecolor='black', label='High'),
    mpatches.Patch(facecolor=colors['normal'], edgecolor='black', label='Normal'),
    mpatches.Patch(facecolor=colors['low'], edgecolor='black', label='Low'),
    mpatches.Patch(facecolor=colors['background'], edgecolor='black', label='Background'),
    mpatches.Patch(facecolor=colors['io'], edgecolor='black', label='IO Task'),
    mpatches.Patch(facecolor=colors['mesh'], edgecolor='black', label='Mesh Build'),
]
fig.legend(handles=legend_elements, loc='upper center', ncol=7, fontsize=12,
           bbox_to_anchor=(0.5, 0.98), frameon=False)

plt.tight_layout(rect=[0, 0, 1, 0.95])
plt.savefig('/Users/a0000/dev/minecraft-reborn/tech_report/images/thread_pool_architecture.png',
            dpi=150, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()

print("Thread pool Gantt chart saved to images/thread_pool_architecture.png")
