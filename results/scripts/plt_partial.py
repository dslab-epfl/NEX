import matplotlib.pyplot as plt
from matplotlib.transforms import offset_copy
import numpy as np
import seaborn as sns
from matplotlib.ticker import FuncFormatter
from matplotlib.font_manager import FontProperties

import matplotlib.font_manager as fm

families = sorted({f.name for f in fm.fontManager.ttflist})
for family in families:
    print(family)

xtick_colors = ['#8B004B', '#006400', '#4B0082']

# Define the benchmarks and the data for simulation error and speedup over baseline
# benchmarks = ['jpeg', 'matmul', 'yolo-v3-mini', 'resnet18', 'resnet34', 'resnet50', 'hp-bench0', 'hp-bench1', 'hp-bench2', 'hp-bench3', 'hp-bench4', 'hp-bench5']
benchmarks = ['JPEG',
            #   'JPEG-2devices', 
              'JPEG-4devices', 
              'JPEG-8devices', 
            #   'VTA-resnet18-4devices', 'VTA-resnet18-8devices',
            #   'VTA-4devices', 
              'VTA-8devices',
              'VTA-matmul', 
              'VTA-yolo-v3-tiny', 
              'VTA-resnet18', 
            #   'VTA-resnet34', 
              'VTA-resnet50', 
            #   'Protoacc-bench0', 
              'Protoacc-bench1', 'Protoacc-bench2', 'Protoacc-bench3', 'Protoacc-bench4', 
            #   'Protoacc-bench5'
              ]

# Simulation error data for each configuration (gem5+RTL, gem5+LPN, native+RTL, native+LPN)
real_time = np.array([
    [10914, 9582, 2165, 147], #JPEG
    # [10887, 9769, 1244, 167], #JPEG-2
    [10869, 10794, 811, 133], #JPEG-4
    [11363, 10779, 568, 122], #JPEG-8

    # [24869,24167,249,40.5], #resent18 4devices
    [33399,31862,213,38], #resent18 8devices

    [1510, 736, 856, 23],  # matmul_opt
    [2983,2875,127,5.8],  # detection
    [1127,1048,29,2.6],  # resnet18
    # [1256,1095.8,54,4.5],   # resnet34
    [6868,4296,2314,25], # resnet50


    # [324.5,316,49,48], # HP-Bench0
    [155.5,143,2,1.8], # HP-Bench1
    [1005,1060,180,152], # HP-Bench2
    [215,221.5,27,25],  # HP-Bench3
    [248.5,252,35,30], # HP-Bench4
    # [988, 990.5, 190, 160]  # HP-Bench5
])

sim_error_data = np.array([
    [0.00, 1.07, 10.69, 11.51],  #JPEG
    # [0.00, 1.04, 8.15, 8.34], #JPEG-2
    [0.00, 1.83, 8.02, 9.70], #JPEG-4
    [0.00, 0.57, 9.30, 8.54], #JPEG-8

    # [0.00, 1.99, 5.18, 4.38],  #resnet18 4devices
    [0.00, 4.00, 2.67, 2.00],  #resnet18 8devices

    [0.00, 8.57, 6.58, 7.25],       # matmul_opt
    [0.00, 0.00, 2.63, 2.55],        # detection
    [0.00, 2.13, 2.13, 1.70],        # resnet18
    # [0.00, 6.06, 5.61, 3.48],      # resnet34
    [0.00, 5.89, 4.35, 3.30],       #resnet50
    
    # [0.00, 0.06, 9.84, 6.95],  # HP-Bench0
    [0.00,11.18, 16.13, 1.87],     # HP-Bench1
    [0.00,0.00, 13.79, 13.78],     # HP-Bench2
    [0.00,2.42, 14.90, 13.47],       # HP-Bench3
    [0.00,4.83, 7.11, 6.76],         # HP-Bench4
    # [0.00, 0.06, 11.81, 11.41],  # HP-Bench5
])

print(sim_error_data[:, 1])
print(np.mean(sim_error_data[:, 1]))

# Speedup data
speedup_data = real_time[:, 0][:, np.newaxis] / real_time

print(speedup_data[:, 3])
print(speedup_data[:, 2])

for e1, e2 in zip(speedup_data[:, 3], speedup_data[:, 2]):
    print("speedup", e1/e2)


# Bar width and positions
figsize_x_ratio = 1.3
fig_height_size = 5
bar_padding_height = -4
bar_width = 0.22
bar_dis = 0.22
index = np.arange(len(benchmarks))
lw = 2.5  # linewidth of border of bars
xticklabels_rotation = 20
hatchpattern = ''
title_size = 17
ticklabel_size = 24
ticklabel_size_y = 27
label_size = ticklabel_size_y
tick_size = 13
# barvalue_size = 22 
barvalue_size = 26
legend_label_size = ticklabel_size_y

# Custom legend labels
legend_labels = ['gem5+RTL', 'gem5+DSim', 'NEX+RTL', 'NEX+DSim']

# Define custom colors
# colors = sns.color_palette("crest", 16)  # Generates 4 pastel colors
border_color = 'white'  # Set the color of the bar borders
# colors = ['#003f5c', '#7a5195', '#ef5675', '#ffa600']
colors = ['#9dc6e2', '#3dd2ca', '#87ce63', '#ffa600']
colors = ['#3e3a71', '#a7407f', '#f45a5a', '#ffa600']
colors = ['#3e3a71', '#f45a5a', '#2ca02c', '#ffa600']

bar_rotate = 90
arial_narrow = FontProperties(family='Nimbus Sans Narrow')
# Create and save separate plots

alpha_value = 1
# Plot Simulation Time
fig1, ax1 = plt.subplots(figsize=(len(benchmarks)*figsize_x_ratio, fig_height_size))
for i in range(4):
    if i == 0: #gem5 + rtl
        bars = ax1.bar(index + i * bar_dis, real_time[:, i], bar_width, alpha=alpha_value, color=colors[i], edgecolor=border_color, linewidth=lw, label=legend_labels[i])
        # ax1.bar_label(bars, padding=bar_padding_height + 4, fontsize=barvalue_size, labels=[f'{int(v)}' for v in bars.datavalues], rotation=25)
        # ax1.bar_label(bars, padding=bar_padding_height, fontsize=barvalue_size, labels=[f'{int(v)}' for v in bars.datavalues])
    elif i == 1:
        import math
        bars = ax1.bar(index + i * bar_dis, real_time[:, i], bar_width, alpha=alpha_value, color=colors[i], edgecolor=border_color, linewidth=lw, label=legend_labels[i])
        print(speedup_data[:, i])
        ax1.bar_label(bars, padding=bar_padding_height+4, fontsize=barvalue_size, labels=[f'{sp:.1f}x' for sp in speedup_data[:, i] ], rotation=bar_rotate, fontproperties=arial_narrow)
    elif i == 2:
        import math
        bars = ax1.bar(index + i * bar_dis, real_time[:, i], bar_width, alpha=alpha_value, color=colors[i], edgecolor=border_color, linewidth=lw, label=legend_labels[i])
        ax1.bar_label(bars, padding=bar_padding_height+4, fontsize=barvalue_size, labels=[f'{sp:.1f}x' for sp in speedup_data[:, i]], rotation=bar_rotate, fontproperties=arial_narrow)
    else:
        import math
        bars = ax1.bar(index + i * bar_dis, real_time[:, i], bar_width, alpha=alpha_value, color=colors[i], edgecolor=border_color, linewidth=lw, label=legend_labels[i])
        ax1.bar_label(bars, padding=bar_padding_height+4, fontsize=barvalue_size, labels=[f'{int(sp+0.1)}x' for sp in speedup_data[:, i]], rotation=bar_rotate, fontproperties=arial_narrow)
        # ax1.bar_label(bars, padding=bar_padding_height+4, fontsize=barvalue_size, labels=[f'{sp:.1f}x' for sp in speedup_data[:, i]], rotation=90)
        print(speedup_data[:, i])
ax1.axhline(y=10, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=100, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=1000, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=10000, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=100000, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')

# f"{int(speedup_data[:, i])}x

# ax1.set_title('Simulation Cost', fontsize=title_size)
# ax1.set_xlabel('Applications', fontsize=label_size)
ax1.set_ylabel('Time(s)', fontsize=label_size)
ax1.set_xticks(index + 1.5 * bar_dis)
labels = ax1.set_xticklabels(benchmarks, rotation=xticklabels_rotation, fontweight='bold')
# labels = ax1.set_xticklabels(benchmarks)

_j = 3
_v = 8
for label in labels[:_j]:
    label.set_color(xtick_colors[0])
for label in labels[_j:_v]:
    label.set_color(xtick_colors[1])
for label in labels[_v:]:
    label.set_color(xtick_colors[2])

ax1.tick_params(axis='x', labelsize=ticklabel_size)
ax1.tick_params(axis='y', labelsize=ticklabel_size_y)
ax1.set_yscale('log')
ax1.set_xlim(-0.25, 12)
ax1.set_ylim(0, 400000)

for label in ax1.get_xticklabels():
    label.set_ha('right')  # Align tick labels to the right

# ax1.axhline(y=10, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
# ax1.axhline(y=100, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
# ax1.axhline(y=1000, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
# ax.axhline(y=100, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
# ax.axhline(y=200, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')

# Ensure the y-axis includes the tick at y=20
yticks = ax1.get_yticks()  # Get existing y-ticks
yticks = [1,10,100,1000,10000, 100000] # Add y=20 if not already present
ax1.set_yticks(yticks)  # Update y-ticks
# fig1.legend(title="", labels=legend_labels, loc='upper right', fontsize=legend_label_size, handlelength=2.5, labelspacing=0.2, borderpad=0.05, bbox_to_anchor=(0.98, 0.9), ncol=2, title_fontsize=14, handletextpad=1)
fig1.legend(labels=legend_labels, loc='upper center', fontsize=legend_label_size, ncol=len(legend_labels), handlelength=1.5, labelspacing=1.5, borderpad=0.15, bbox_to_anchor=(0.52, 1.12))


plt.tight_layout()
# plt.show()
fig1.savefig('simtime_allbars_partial.pdf', bbox_inches='tight')
plt.close(fig1)

bar_width = 0.2
bar_dis = 0.27

fig3, ax3 = plt.subplots(figsize=(len(benchmarks)*figsize_x_ratio, fig_height_size))
for i in range(1,4):
    bars = ax3.bar(index + i * bar_dis, sim_error_data[:, i], bar_width, color=colors[i], edgecolor=border_color, hatch=hatchpattern, linewidth=lw, label=legend_labels[i])
    ax3.bar_label(bars, padding=bar_padding_height+2, fontsize=barvalue_size, labels=[f'{v:.1f}%' for v in bars.datavalues], rotation=90)

ax3.axhline(y=5, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax3.axhline(y=10, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax3.axhline(y=15, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
# Define a formatter function to append '%'
def percent_formatter(x, pos):
    return f"{x:.0f}%"

ax3.set_ylabel('Relative Error (%)', fontsize=label_size)
ax3.set_xticks(index + 2 * bar_dis)
labels = ax3.set_xticklabels(benchmarks, rotation=xticklabels_rotation, fontweight='bold')

_j = 3
_v = 8
for label in labels[:_j]:
    label.set_color(xtick_colors[0])
for label in labels[_j:_v]:
    label.set_color(xtick_colors[1])
for label in labels[_v:]:
    label.set_color(xtick_colors[2])

for label in ax3.get_xticklabels():
    label.set_ha('right')  # Align tick labels to the right

ax3.tick_params(axis='x', labelsize=ticklabel_size)
ax3.tick_params(axis='y', labelsize=ticklabel_size_y)
# ax3.set_yscale('log')
ax3.set_ylim(0,25)
# ax3.set_xlim(-0.23, 11)
yticks = ax3.get_yticks()  # Get existing y-ticks
yticks = [5, 10, 15] # Add y=20 if not already present
ax3.set_yticks(yticks) 
# fig3.legend(title="", labels=legend_labels, loc='upper left', fontsize=legend_label_size, handlelength=1.5, labelspacing=0.2, borderpad=0.2, bbox_to_anchor=(0.06, 0.88), ncol=1, title_fontsize=14, handletextpad=1)
fig3.legend(labels=legend_labels[1:], loc='upper center', fontsize=legend_label_size, ncol=len(legend_labels), handlelength=1.5, labelspacing=1.5, borderpad=0.15, bbox_to_anchor=(0.52, 1.12))

plt.tight_layout()
# plt.show()
fig3.savefig('simerror_allbars_partial.pdf', bbox_inches='tight')  # Save as PNG
plt.close(fig3)
