import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

df = pd.read_csv('scalability_results.csv')
threads = df['Threads']
t20 = df['Time_20000_ms']
t40 = df['Time_40000_ms']

speedup_20 = t20.iloc[0] / t20
speedup_40 = t40.iloc[0] / t40

plt.figure(figsize=(10, 6))
plt.plot(threads, speedup_20, marker='o', label='20000x20000')
plt.plot(threads, speedup_40, marker='s', label='40000x40000')
plt.plot(threads, threads, linestyle='--', color='gray', label='Линейное ускорение (Ideal)')

plt.title('Ускорение в зависимости от количества потоков')
plt.xlabel('Количество потоков')
plt.ylabel('Ускорение (Speedup)')
plt.legend()
plt.grid(True)
plt.savefig('speedup_plot.png', dpi=300)
print("График сохранён в speedup_plot.png")