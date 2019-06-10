import numpy as np
import matplotlib.pyplot as mpl
from pprint import pprint

init_array = []
wordscount_array = []
global_array = []
filepath = 'times/benchmarks_times'
with open(filepath) as fp:
    line = fp.readline()
    while line:
        if line[0] != '-':
            parts = line.split(',')
            time = float(parts[1].strip())
            if parts[0] == 'Init':
                init_array.append(time)
            if parts[0] == 'Average Wordscount':
                wordscount_array.append(time)
            if parts[0] == 'Average Global':
                global_array.append(time)
        line = fp.readline()
split_init_array = np.split(np.array(init_array), 24)
split_wordscount_array = np.split(np.array(wordscount_array), 24)
split_global_array = np.split(np.array(global_array), 24)
print("Init array")
pprint(split_init_array)
print("Wordscount array")
pprint(split_wordscount_array)
print("Global array")
pprint(split_global_array)
x_np = np.arange(1, 21)
print("Number of processes")
pprint(x_np)

# Data preparation
y_init_strong_1_1 = split_init_array[0]
y_init_strong_1_2 = split_init_array[1]
y_init_strong_1_3 = split_init_array[2]
y_init_strong_1_4 = split_init_array[3]
y_init_strong_1 = np.mean([y_init_strong_1_1, y_init_strong_1_2, y_init_strong_1_3, y_init_strong_1_4], axis=0)
y_init_strong_1 *= 1000

y_init_strong_2_1 = split_init_array[4]
y_init_strong_2_2 = split_init_array[5]
y_init_strong_2_3 = split_init_array[6]
y_init_strong_2_4 = split_init_array[7]
y_init_strong_2 = np.mean([y_init_strong_2_1, y_init_strong_2_2, y_init_strong_2_3, y_init_strong_2_4], axis=0)
y_init_strong_2 *= 1000

y_init_strong_3_1 = split_init_array[8]
y_init_strong_3_2 = split_init_array[9]
y_init_strong_3_3 = split_init_array[10]
y_init_strong_3_4 = split_init_array[11]
y_init_strong_3 = np.mean([y_init_strong_3_1, y_init_strong_3_2, y_init_strong_3_3, y_init_strong_3_4], axis=0)
y_init_strong_3 *= 1000

y_init_weak_1_1 = split_init_array[12]
y_init_weak_1_2 = split_init_array[13]
y_init_weak_1_3 = split_init_array[14]
y_init_weak_1_4 = split_init_array[15]
y_init_weak_1 = np.mean([y_init_weak_1_1, y_init_weak_1_2, y_init_weak_1_3, y_init_weak_1_4], axis=0)
y_init_weak_1 *= 1000

y_init_weak_2_1 = split_init_array[16]
y_init_weak_2_2 = split_init_array[17]
y_init_weak_2_3 = split_init_array[18]
y_init_weak_2_4 = split_init_array[19]
y_init_weak_2 = np.mean([y_init_weak_2_1, y_init_weak_2_2, y_init_weak_2_3, y_init_weak_2_4], axis=0)
y_init_weak_2 *= 1000

y_init_weak_3_1 = split_init_array[20]
y_init_weak_3_2 = split_init_array[21]
y_init_weak_3_3 = split_init_array[22]
y_init_weak_3_4 = split_init_array[23]
y_init_weak_3 = np.mean([y_init_weak_3_1, y_init_weak_3_2, y_init_weak_3_3, y_init_weak_3_4], axis=0)
y_init_weak_3 *= 1000

y_wordscount_strong_1_1 = split_wordscount_array[0]
y_wordscount_strong_1_2 = split_wordscount_array[1]
y_wordscount_strong_1_3 = split_wordscount_array[2]
y_wordscount_strong_1_4 = split_wordscount_array[3]
y_wordscount_strong_1 = np.mean(
    [y_wordscount_strong_1_1, y_wordscount_strong_1_2, y_wordscount_strong_1_3, y_wordscount_strong_1_4], axis=0)
y_wordscount_strong_1 *= 1000

y_wordscount_strong_2_1 = split_wordscount_array[4]
y_wordscount_strong_2_2 = split_wordscount_array[5]
y_wordscount_strong_2_3 = split_wordscount_array[6]
y_wordscount_strong_2_4 = split_wordscount_array[7]
y_wordscount_strong_2 = np.mean(
    [y_wordscount_strong_2_1, y_wordscount_strong_2_2, y_wordscount_strong_2_3, y_wordscount_strong_2_4], axis=0)
y_wordscount_strong_2 *= 1000

y_wordscount_strong_3_1 = split_wordscount_array[8]
y_wordscount_strong_3_2 = split_wordscount_array[9]
y_wordscount_strong_3_3 = split_wordscount_array[10]
y_wordscount_strong_3_4 = split_wordscount_array[11]
y_wordscount_strong_3 = np.mean(
    [y_wordscount_strong_3_1, y_wordscount_strong_3_2, y_wordscount_strong_3_3, y_wordscount_strong_3_4], axis=0)
y_wordscount_strong_3 *= 1000

y_wordscount_weak_1_1 = split_wordscount_array[12]
y_wordscount_weak_1_2 = split_wordscount_array[13]
y_wordscount_weak_1_3 = split_wordscount_array[14]
y_wordscount_weak_1_4 = split_wordscount_array[15]
y_wordscount_weak_1 = np.mean(
    [y_wordscount_weak_1_1, y_wordscount_weak_1_2, y_wordscount_weak_1_3, y_wordscount_weak_1_4], axis=0)
y_wordscount_weak_1 *= 1000

y_wordscount_weak_2_1 = split_wordscount_array[16]
y_wordscount_weak_2_2 = split_wordscount_array[17]
y_wordscount_weak_2_3 = split_wordscount_array[18]
y_wordscount_weak_2_4 = split_wordscount_array[19]
y_wordscount_weak_2 = np.mean(
    [y_wordscount_weak_2_1, y_wordscount_weak_2_2, y_wordscount_weak_2_3, y_wordscount_weak_2_4], axis=0)
y_wordscount_weak_2 *= 1000

y_wordscount_weak_3_1 = split_wordscount_array[20]
y_wordscount_weak_3_2 = split_wordscount_array[21]
y_wordscount_weak_3_3 = split_wordscount_array[22]
y_wordscount_weak_3_4 = split_wordscount_array[23]
y_wordscount_weak_3 = np.mean(
    [y_wordscount_weak_3_1, y_wordscount_weak_3_2, y_wordscount_weak_3_3, y_wordscount_weak_3_4], axis=0)
y_wordscount_weak_3 *= 1000

y_global_strong_1_1 = split_global_array[0]
y_global_strong_1_2 = split_global_array[1]
y_global_strong_1_3 = split_global_array[2]
y_global_strong_1_4 = split_global_array[3]
y_global_strong_1 = np.mean(
    [y_global_strong_1_1, y_global_strong_1_2, y_global_strong_1_3, y_global_strong_1_4], axis=0)
y_global_strong_1 *= 1000

y_global_strong_2_1 = split_global_array[4]
y_global_strong_2_2 = split_global_array[5]
y_global_strong_2_3 = split_global_array[6]
y_global_strong_2_4 = split_global_array[7]
y_global_strong_2 = np.mean(
    [y_global_strong_2_1, y_global_strong_2_2, y_global_strong_2_3, y_global_strong_2_4], axis=0)
y_global_strong_2 *= 1000

y_global_strong_3_1 = split_global_array[8]
y_global_strong_3_2 = split_global_array[9]
y_global_strong_3_3 = split_global_array[10]
y_global_strong_3_4 = split_global_array[11]
y_global_strong_3 = np.mean(
    [y_global_strong_3_1, y_global_strong_3_2, y_global_strong_3_3, y_global_strong_3_4], axis=0)
y_global_strong_3 *= 1000

y_global_weak_1_1 = split_global_array[12]
y_global_weak_1_2 = split_global_array[13]
y_global_weak_1_3 = split_global_array[14]
y_global_weak_1_4 = split_global_array[15]
y_global_weak_1 = np.mean(
    [y_global_weak_1_1, y_global_weak_1_2, y_global_weak_1_3, y_global_weak_1_4], axis=0)
y_global_weak_1 *= 1000

y_global_weak_2_1 = split_global_array[16]
y_global_weak_2_2 = split_global_array[17]
y_global_weak_2_3 = split_global_array[18]
y_global_weak_2_4 = split_global_array[19]
y_global_weak_2 = np.mean(
    [y_global_weak_2_1, y_global_weak_2_2, y_global_weak_2_3, y_global_weak_2_4], axis=0)
y_global_weak_2 *= 1000

y_global_weak_3_1 = split_global_array[20]
y_global_weak_3_2 = split_global_array[21]
y_global_weak_3_3 = split_global_array[22]
y_global_weak_3_4 = split_global_array[23]
y_global_weak_3 = np.mean(
    [y_global_weak_3_1, y_global_weak_3_2, y_global_weak_3_3, y_global_weak_3_4], axis=0)
y_global_weak_3 *= 1000

# Init strong
mpl.figure(1)
mpl.plot(x_np, y_init_strong_1, label=".strong_files/1/")
mpl.plot(x_np, y_init_strong_2, label=".strong_files/2/")
mpl.plot(x_np, y_init_strong_3, label=".strong_files/3/")
mpl.xticks(x_np)
mpl.title("Init strong scalability")
mpl.xlabel("#processes")
mpl.ylabel("Time (ms)")
legend = mpl.legend(loc='lower center', bbox_to_anchor=(0.5, -0.22), shadow=True, ncol=5)
mpl.savefig('figures/init_strong.png', bbox_extra_artists=[legend], bbox_inches='tight')

# Wordscount strong
mpl.figure(2)
mpl.plot(x_np, y_wordscount_strong_1, label=".strong_files/1/")
mpl.plot(x_np, y_wordscount_strong_2, label=".strong_files/2/")
mpl.plot(x_np, y_wordscount_strong_3, label=".strong_files/3/")
mpl.xticks(x_np)
mpl.title("Wordscount strong scalability")
mpl.xlabel("#processes")
mpl.ylabel("Time (ms)")
legend = mpl.legend(loc='lower center', bbox_to_anchor=(0.5, -0.22), shadow=True, ncol=5)
mpl.savefig('figures/wordscount_strong.png', bbox_extra_artists=[legend], bbox_inches='tight')

# Global strong
mpl.figure(3)
mpl.plot(x_np, y_global_strong_1, label=".strong_files/1/")
mpl.plot(x_np, y_global_strong_2, label=".strong_files/2/")
mpl.plot(x_np, y_global_strong_3, label=".strong_files/3/")
mpl.xticks(x_np)
mpl.title("Global strong scalability")
mpl.xlabel("#processes")
mpl.ylabel("Time (ms)")
legend = mpl.legend(loc='lower center', bbox_to_anchor=(0.5, -0.22), shadow=True, ncol=5)
mpl.savefig('figures/global_strong.png', bbox_extra_artists=[legend], bbox_inches='tight')

# Init weak
mpl.figure(4)
mpl.plot(x_np, y_init_weak_1, label=".weak_files/1/")
mpl.plot(x_np, y_init_weak_2, label=".weak_files/2/")
mpl.plot(x_np, y_init_weak_3, label=".weak_files/3/")
mpl.xticks(x_np)
mpl.title("Init weak scalability")
mpl.xlabel("#processes")
mpl.ylabel("Time (ms)")
legend = mpl.legend(loc='lower center', bbox_to_anchor=(0.5, -0.22), shadow=True, ncol=5)
mpl.savefig('figures/init_weak.png', bbox_extra_artists=[legend], bbox_inches='tight')

# Wordscount weak
mpl.figure(5)
mpl.plot(x_np, y_wordscount_weak_1, label=".weak_files/1/")
mpl.plot(x_np, y_wordscount_weak_2, label=".weak_files/2/")
mpl.plot(x_np, y_wordscount_weak_3, label=".weak_files/3/")
mpl.xticks(x_np)
mpl.title("Wordscount weak scalability")
mpl.xlabel("#processes")
mpl.ylabel("Time (ms)")
legend = mpl.legend(loc='lower center', bbox_to_anchor=(0.5, -0.22), shadow=True, ncol=5)
mpl.savefig('figures/wordscount_weak.png', bbox_extra_artists=[legend], bbox_inches='tight')

# Global weak
mpl.figure(6)
mpl.plot(x_np, y_global_weak_1, label=".weak_files/1/")
mpl.plot(x_np, y_global_weak_2, label=".weak_files/2/")
mpl.plot(x_np, y_global_weak_3, label=".weak_files/3/")
mpl.xticks(x_np)
mpl.title("Global weak scalability")
mpl.xlabel("#processes")
mpl.ylabel("Time (ms)")
legend = mpl.legend(loc='lower center', bbox_to_anchor=(0.5, -0.22), shadow=True, ncol=5)
mpl.savefig('figures/global_weak.png', bbox_extra_artists=[legend], bbox_inches='tight')
