#!/usr/bin/env bash

if [ "$#" -eq 0 ]; then
    echo "Ошибка: передайте хотя бы одно число."
    echo "Пример: ./average.sh 10 20 30"
    exit 1
fi

sum=0

for number in "$@"; do
    sum=$((sum + number))
done

average=$(awk "BEGIN {printf \"%.2f\", $sum / $#}")
echo "Количество чисел: $#"
echo "Среднее арифметическое: $average"
