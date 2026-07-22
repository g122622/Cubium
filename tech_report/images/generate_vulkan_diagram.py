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

fig, ax = plt.subplots(1, 1, figsize=(16, 11))

# 颜色定义
colors = {
    'game': '#1565C0',
    'scheduler': '#7B1FA2',
    'worker': '#C2185B',
    'gpu': '#D32F2F',
    'vulkan': '#388E3C',
    'frame': '#FF9800',
}

ax.set_xlim(0, 20)
ax.set_ylim(0, 14)
ax.axis('off')
ax.set_title('Trident Vulkan Rendering Pipeline', fontsize=22, fontweight='bold', pad=20)

# ============ 游戏主线程 ============
ax.add_patch(FancyBboxPatch((0.5, 11.5), 4, 2, boxstyle='round,pad=0.1',
                             facecolor='#E3F2FD', edgecolor='#1565C0', linewidth=2))
ax.text(2.5, 12.5, 'Game Thread', ha='center', va='center', fontsize=14, fontweight='bold', color='#1565C0')
ax.text(2.5, 12.0, 'Chunk Updates', ha='center', va='center', fontsize=11, color='#666666')

# ============ MeshBuildScheduler ============
ax.add_patch(FancyBboxPatch((5.5, 10.5), 5, 3, boxstyle='round,pad=0.1',
                             facecolor='#F3E5F5', edgecolor='#7B1FA2', linewidth=2))
ax.text(8, 13.0, 'MeshBuildScheduler', ha='center', va='center', fontsize=14, fontweight='bold', color='#7B1FA2')
ax.text(8, 12.4, 'Priority Queue', ha='center', va='center', fontsize=11, color='#666666')

# 调度算法
algo_lines = [
    'score = distance',
    'if inFrustum: score -= 100',
    'score -= forwardDot * 8',
    'if behind: cancel'
]
for i, line in enumerate(algo_lines):
    ax.text(8, 11.7 - i * 0.4, line, ha='center', va='center', fontsize=10, family='monospace')

# 箭头：游戏线程 -> 调度器
ax.annotate('', xy=(5.5, 12.5), xytext=(4.5, 12.5),
            arrowprops=dict(arrowstyle='->', color='#1565C0', lw=2.5))

# ============ ClientCompute (UniversalWorkerPool) ============
ax.add_patch(FancyBboxPatch((11.5, 10.5), 4, 3, boxstyle='round,pad=0.1',
                             facecolor='#FCE4EC', edgecolor='#C2185B', linewidth=2))
ax.text(13.5, 13.0, 'ClientCompute', ha='center', va='center', fontsize=14, fontweight='bold', color='#C2185B')
ax.text(13.5, 12.4, 'Thread 1: Build Mesh', ha='center', va='center', fontsize=11, color='#666666')
ax.text(13.5, 11.9, 'Thread 2: Build Mesh', ha='center', va='center', fontsize=11, color='#666666')
ax.text(13.5, 11.4, 'Thread N: Build Mesh', ha='center', va='center', fontsize=11, color='#666666')

# 箭头：调度器 -> Worker池
ax.annotate('', xy=(11.5, 12), xytext=(10.5, 12),
            arrowprops=dict(arrowstyle='->', color='#7B1FA2', lw=2.5))
ax.text(11, 12.3, 'dispatch', fontsize=10, ha='center', color='#7B1FA2', style='italic')

# ============ GPU Upload Queue ============
ax.add_patch(FancyBboxPatch((16, 10.5), 3.5, 3, boxstyle='round,pad=0.1',
                             facecolor='#FFEBEE', edgecolor='#D32F2F', linewidth=2))
ax.text(17.75, 13.0, 'Upload Queue', ha='center', va='center', fontsize=13, fontweight='bold', color='#D32F2F')
ax.text(17.75, 12.3, 'Staging Buffer', ha='center', va='center', fontsize=11, color='#666666')
ax.text(17.75, 11.7, 'Transfer Queue', ha='center', va='center', fontsize=11, color='#666666')
ax.text(17.75, 11.1, 'Async Copy', ha='center', va='center', fontsize=11, color='#666666')

# 箭头：Worker池 -> 上传队列
ax.annotate('', xy=(16, 12), xytext=(15.5, 12),
            arrowprops=dict(arrowstyle='->', color='#C2185B', lw=2.5))

# ============ Vulkan Frame Rendering ============
ax.add_patch(FancyBboxPatch((0.5, 2), 19, 7.5, boxstyle='round,pad=0.1',
                             facecolor='#E8F5E9', edgecolor='#388E3C', linewidth=2))
ax.text(10, 9.0, 'Vulkan Frame Rendering', ha='center', va='center', fontsize=16, fontweight='bold', color='#2E7D32')

# 帧流程
steps = [
    (1.5, 'Wait Fence\n(Frame N-2)', '#9E9E9E'),
    (4.5, 'Acquire\nImage', '#1976D2'),
    (7.5, 'Record\nCommands', '#388E3C'),
    (10.5, 'Submit\nQueue', '#F57C00'),
    (13.5, 'Present\nImage', '#D32F2F'),
    (16.5, 'Signal\nFence', '#9E9E9E'),
]

for x, label, color in steps:
    ax.add_patch(FancyBboxPatch((x-1, 6), 2.5, 2, boxstyle='round,pad=0.1',
                                 facecolor=color, edgecolor='black', linewidth=1.5, alpha=0.8))
    ax.text(x+0.25, 7, label, ha='center', va='center', fontsize=11, fontweight='bold', color='white')

# 箭头连接步骤
for i in range(len(steps) - 1):
    ax.annotate('', xy=(steps[i+1][0]-1, 7), xytext=(steps[i][0]+1.5, 7),
                arrowprops=dict(arrowstyle='->', color='#424242', lw=2))

# 同步说明
ax.text(10, 5.3, 'Synchronization:', ha='center', va='center', fontsize=12, fontweight='bold')
sync_items = [
    'Semaphores: Image Available, Render Finished',
    'Fences: Frame Fence (CPU-GPU sync)',
    'Double/Triple Buffering: N frames in flight'
]
for i, item in enumerate(sync_items):
    ax.text(10, 4.7 - i * 0.5, item, ha='center', va='center', fontsize=11, color='#666666')

# ============ Descriptor Sets ============
ax.add_patch(FancyBboxPatch((0.5, 0.3), 9, 1.2, boxstyle='round,pad=0.1',
                             facecolor='#FFF3E0', edgecolor='#FF9800', linewidth=2))
ax.text(5, 1.1, 'Descriptor Sets', ha='center', va='center', fontsize=13, fontweight='bold', color='#E65100')
ax.text(5, 0.6, 'UBO: Camera Matrix, Light Data | Texture Array: Block Textures', ha='center', va='center', fontsize=11, color='#666666')

# ============ 命令缓冲池 ============
ax.add_patch(FancyBboxPatch((10.5, 0.3), 9, 1.2, boxstyle='round,pad=0.1',
                             facecolor='#E1F5FE', edgecolor='#0288D1', linewidth=2))
ax.text(15, 1.1, 'Command Buffer Pool', ha='center', va='center', fontsize=13, fontweight='bold', color='#01579B')
ax.text(15, 0.6, 'Per-frame command buffers | Reset on fence signal', ha='center', va='center', fontsize=11, color='#666666')

# 图例
legend_elements = [
    mpatches.Patch(facecolor='#E3F2FD', edgecolor='#1565C0', label='Game Thread'),
    mpatches.Patch(facecolor='#F3E5F5', edgecolor='#7B1FA2', label='Scheduler'),
    mpatches.Patch(facecolor='#FCE4EC', edgecolor='#C2185B', label='Worker Pool'),
    mpatches.Patch(facecolor='#FFEBEE', edgecolor='#D32F2F', label='GPU Upload'),
    mpatches.Patch(facecolor='#E8F5E9', edgecolor='#388E3C', label='Vulkan Render'),
]
ax.legend(handles=legend_elements, loc='upper right', fontsize=11, framealpha=0.9)

plt.tight_layout()
plt.savefig('/Users/a0000/dev/minecraft-reborn/tech_report/images/vulkan_pipeline.png',
            dpi=150, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()

print("Vulkan pipeline diagram saved to images/vulkan_pipeline.png")
