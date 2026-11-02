#include <windows.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <numeric>
#include "Timer.h"
#include "Matrix.h"
#include "Threads.h"
#include "Utils.h"

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    // ввод размера пользователем
    int N = 0;
    std::cout << "Введите размер квадратной матрицы N (например, 500): ";
    if (!(std::cin >> N) || N <= 0) {
        std::cerr << "Ошибка: некорректный размер матрицы!\n";
        return 1;
    }

    std::cout << "\nГенерация матриц A и B (" << N << "x" << N << ")...\n";
    Matrix A = GenerateRandomMatrix(N);
    Matrix B = GenerateRandomMatrix(N);
    Matrix C_seq(N * N, 0.0);
    Matrix C_par(N * N, 0.0);

    HighResolutionTimer timer;

    // последовательный эталон
    std::cout << "\n--- Запуск последовательного умножения (Эталон) ---\n";
    timer.Start();
    MultiplyMatrix(A, B, C_seq, N);
    timer.Stop();
    double seqTime = timer.GetMilliseconds();
    std::cout << "Время последовательного выполнения: "
              << std::fixed << std::setprecision(2) << seqTime << " мс.\n";

    // эксперимент: K = 1, 2, 4, 8 (по 3 прогона)
    std::vector<int> threadList = {1, 2, 4, 8};
    const int RUNS = 3;

    std::cout << "\n================ НАЧАЛО ЭКСПЕРИМЕНТА ================\n";

    for (int K : threadList) {
        std::cout << "\n>>> Исследование для K = " << K << " потоков <<<\n";
        std::vector<double> runTimes;

        for (int run = 1; run <= RUNS; ++run) {
            std::cout << "  Прогон " << run << " из " << RUNS << ":\n";

            timer.Start();
            RunParallelMultiplication(A, B, C_par, N, K);
            timer.Stop();

            double t = timer.GetMilliseconds();
            runTimes.push_back(t);
            std::cout << "  Время: " << t << " мс.\n";

            // Проверка правильности (на первом прогоне)
            if (run == 1) {
                if (!AreMatricesEqual(C_seq, C_par, N)) {
                    std::cerr << "  [ОШИБКА] Результаты параллельного умножения НЕ СОВПАДАЮТ!\n";
                } else {
                    std::cout << "  [Валидация] Результат совпал с эталоном!\n";
                }
            }
        }

        // Считаем среднее время
        double avgTime = std::accumulate(runTimes.begin(), runTimes.end(), 0.0) / RUNS;
        double speedup = seqTime / avgTime;

        std::cout << "----------------------------------------------------\n";
        std::cout << "ИТОГ для K = " << K
                  << " | Среднее время: " << avgTime << " мс."
                  << " | Ускорение: " << speedup << "x\n";
        std::cout << "----------------------------------------------------\n";
    }

    std::cout << "\nЭксперимент успешно завершен!\n";
    return 0;
}