import matplotlib.pyplot as plt
import pandas as pd
import sys

file = sys.argv[1] if len(sys.argv) > 1 else "task1_output.csv"
df = pd.read_csv(file, sep=',', comment='#')

threads = [1, 2, 4, 7, 8, 16, 20, 40]
for size in df['Size'].unique():
    sub = df[df['Size'] == size].sort_values('Threads')
    base_time = sub[sub['Threads'] == 1]['CompTime(s)'].values[0]
    sub['Speedup'] = base_time / sub['CompTime(s)']
    plt.plot(sub['Threads'], sub['Speedup'], marker='o', label=f'Size={size}')

plt.axhline(y=1, color='gray', linestyle='--')
plt.xlabel('Количество потоков')
plt.ylabel('Ускорение (Speedup)')
plt.title('Масштабируемость матрично-векторного умножения')
plt.legend()
plt.grid(True)
plt.savefig('scaling_plot.png', dpi=300)
print("График сохранен в scaling_plot.png")