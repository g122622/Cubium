import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import Rectangle, FancyBboxPatch, FancyArrowPatch
import numpy as np

# 使用新罗马字体，增大字号
plt.rcParams['font.family'] = 'serif'
plt.rcParams['font.serif'] = ['Times New Roman', 'Times', 'DejaVu Serif']
plt.rcParams['mathtext.fontset'] = 'stix'
plt.rcParams['axes.unicode_minus'] = False
plt.rcParams['font.size'] = 14

fig, axes = plt.subplots(2, 1, figsize=(16, 12))

# 颜色定义
colors = {
    'increase': '#4CAF50',   # 绿色 - 增加队列
    'decrease': '#F44336',   # 红色 - 减少队列
    'source': '#FFC107',     # 黄色 - 光源
    'block': '#9E9E9E',      # 灰色 - 方块
    'light': '#FFF59D',      # 浅黄 - 光照
}

# ============ 上图：64位队列编码 ============
ax1 = axes[0]
ax1.set_xlim(0, 16)
ax1.set_ylim(0, 6)
ax1.axis('off')
ax1.set_title('64-bit Queue Element Encoding', fontsize=20, fontweight='bold', pad=15)

# 绘制位字段
bit_fields = [
    (0, 6, 'x_rel', '6 bits', colors['increase']),
    (6, 12, 'z_rel', '6 bits', '#2196F3'),
    (12, 28, 'y_rel', '16 bits', '#9C27B0'),
    (28, 32, 'L', '4 bits', colors['source']),
    (32, 38, 'D', '6 bits', '#FF5722'),
    (38, 41, 'F', '3 bits', '#795548'),
    (41, 64, 'Reserved', '23 bits', '#BDBDBD'),
]

# 绘制64位字段 (缩放到16单位宽度)
scale = 16 / 64
y_start = 3.5
height = 1.5

for start, end, name, bits, color in bit_fields:
    x = start * scale
    width = (end - start) * scale
    rect = Rectangle((x, y_start), width, height, facecolor=color, edgecolor='black', linewidth=2)
    ax1.add_patch(rect)
    ax1.text(x + width/2, y_start + height/2, f'{name}\n{bits}', ha='center', va='center',
             fontsize=12, fontweight='bold', color='white' if color != colors['source'] else 'black')

# 位编号
for i in range(0, 65, 8):
    x = i * scale
    ax1.text(x, y_start - 0.3, str(i), ha='center', va='top', fontsize=10)

# 字段说明
legend_y = 1.5
legends = [
    (1, 'x_rel: X coordinate offset [-32, 31]', colors['increase']),
    (5, 'z_rel: Z coordinate offset [-32, 31]', '#2196F3'),
    (9, 'y_rel: Y coordinate [-32768, 32767]', '#9C27B0'),
    (13, 'L: Light level [0, 15]', colors['source']),
]
for x, text, color in legends:
    rect = Rectangle((x-0.8, legend_y-0.3), 0.6, 0.6, facecolor=color, edgecolor='black', linewidth=1)
    ax1.add_patch(rect)
    ax1.text(x + 0.2, legend_y, text, ha='left', va='center', fontsize=12)

# 第二行说明
legend_y2 = 0.5
legends2 = [
    (1, 'D: Direction bitset (6 directions)', '#FF5722'),
    (7, 'F: Flags (WRITE_LEVEL, RECHECK, HAS_SIDED)', '#795548'),
    (13, 'Reserved: Future use', '#BDBDBD'),
]
for x, text, color in legends2:
    rect = Rectangle((x-0.8, legend_y2-0.3), 0.6, 0.6, facecolor=color, edgecolor='black', linewidth=1)
    ax1.add_patch(rect)
    ax1.text(x + 0.2, legend_y2, text, ha='left', va='center', fontsize=12)

# ============ 下图：双队列传播算法 ============
ax2 = axes[1]
ax2.set_xlim(0, 16)
ax2.set_ylim(0, 8)
ax2.axis('off')
ax2.set_title('Dual Queue Light Propagation Algorithm', fontsize=20, fontweight='bold', pad=15)

# 增加队列
ax2.add_patch(FancyBboxPatch((0.5, 5.5), 4, 2, boxstyle='round,pad=0.1',
                              facecolor='#E8F5E9', edgecolor='#4CAF50', linewidth=2))
ax2.text(2.5, 7.0, 'Increase Queue', ha='center', va='center', fontsize=14, fontweight='bold', color='#2E7D32')
ax2.text(2.5, 6.3, 'FIFO Array', ha='center', va='center', fontsize=12, color='#666666')
ax2.text(2.5, 5.8, 'O(1) push/pop', ha='center', va='center', fontsize=11, color='#666666')

# 减少队列
ax2.add_patch(FancyBboxPatch((0.5, 2.5), 4, 2, boxstyle='round,pad=0.1',
                              facecolor='#FFEBEE', edgecolor='#F44336', linewidth=2))
ax2.text(2.5, 4.0, 'Decrease Queue', ha='center', va='center', fontsize=14, fontweight='bold', color='#C62828')
ax2.text(2.5, 3.3, 'FIFO Array', ha='center', va='center', fontsize=12, color='#666666')
ax2.text(2.5, 2.8, 'O(1) push/pop', ha='center', va='center', fontsize=11, color='#666666')

# 光照网格示意
grid_x, grid_y = 6, 2.5
grid_size = 1.2
for i in range(5):
    for j in range(5):
        x = grid_x + i * grid_size
        y = grid_y + j * grid_size
        # 光源在中心
        if i == 2 and j == 2:
            color = '#FFD700'
            light_level = 15
        elif abs(i-2) + abs(j-2) == 1:
            color = '#FFF59D'
            light_level = 14
        elif abs(i-2) + abs(j-2) == 2:
            color = '#FFEE58'
            light_level = 13
        elif abs(i-2) + abs(j-2) == 3:
            color = '#FDD835'
            light_level = 12
        elif abs(i-2) + abs(j-2) == 4:
            color = '#FBC02D'
            light_level = 11
        else:
            color = '#E0E0E0'
            light_level = 0

        rect = Rectangle((x, y), grid_size * 0.9, grid_size * 0.9,
                         facecolor=color, edgecolor='#424242', linewidth=1)
        ax2.add_patch(rect)
        if light_level > 0:
            ax2.text(x + grid_size * 0.45, y + grid_size * 0.45, str(light_level),
                    ha='center', va='center', fontsize=11, fontweight='bold')

ax2.text(8.5, 8.2, 'Light Propagation', ha='center', va='center', fontsize=14, fontweight='bold')

# 箭头：从队列到网格
ax2.annotate('', xy=(6, 4.5), xytext=(4.7, 6.5),
            arrowprops=dict(arrowstyle='->', color='#4CAF50', lw=2.5))
ax2.text(4.5, 5.8, 'propagate', fontsize=11, ha='right', color='#4CAF50', style='italic')

ax2.annotate('', xy=(6, 3.5), xytext=(4.7, 3.5),
            arrowprops=dict(arrowstyle='->', color='#F44336', lw=2.5))
ax2.text(4.5, 3.2, 'decrease', fontsize=11, ha='right', color='#F44336', style='italic')

# 算法说明
algo_box = FancyBboxPatch((12, 2), 3.5, 5.5, boxstyle='round,pad=0.1',
                           facecolor='#F5F5F5', edgecolor='#616161', linewidth=1.5)
ax2.add_patch(algo_box)
ax2.text(13.75, 7.2, 'Algorithm', ha='center', va='center', fontsize=14, fontweight='bold')

algo_text = [
    '1. Pop from queue',
    '2. Decode element',
    '3. Check cancel flag',
    '4. For each direction:',
    '   - Calculate new level',
    '   - If brighter: push inc',
    '   - If darker: push dec',
    '5. Repeat until empty'
]
for i, line in enumerate(algo_text):
    ax2.text(12.2, 6.5 - i * 0.55, line, ha='left', va='center', fontsize=10, family='monospace')

# 标题说明
ax2.text(2.5, 0.8, 'O(n) total - vs O(n log n) with priority queue',
         ha='center', va='center', fontsize=12, color='#666666', style='italic')

plt.tight_layout()
plt.savefig('/Users/a0000/dev/minecraft-reborn/tech_report/images/lighting_system.png',
            dpi=150, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()

print("Lighting system diagram saved to images/lighting_system.png")
