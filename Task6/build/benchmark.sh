SIZES=(128 256 512 1024)
# Если есть доступ к Linux с Nvidia GPU, добавь "gpu" в массив TARGETS
TARGETS=("host" "multicore") 

echo "size,target,time_sec,iterations,error" > results.csv

for size in "${SIZES[@]}"; do
    for target in "${TARGETS[@]}"; do
        echo "🚀 Запуск: size=$size, target=$target..."
        
        # Запускаем программу и перехватываем вывод
        output=$(./heat_solver -n $size --eps=1e-6 --max-iter=1000000 -o /dev/null -acc=$target 2>&1)
        
        # Парсим нужные значения из вывода программы
        time=$(echo "$output" | grep "time_sec=" | cut -d'=' -f2)
        iters=$(echo "$output" | grep "iterations=" | cut -d'=' -f2)
        err=$(echo "$output" | grep "error=" | cut -d'=' -f2)
        
        # Записываем в CSV
        echo "$size,$target,$time,$iters,$err" >> results.csv
        echo "✅ Готово: time=$time sec"
    done
done