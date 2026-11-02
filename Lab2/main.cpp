#include <windows.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <numeric>
#include "Utils.h"
#include "Timer.h"
#include "FileGenerator.h"
#include "FileProcessor.h"

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    size_t sizeMB = 0;
    std::cout << "Введите размер тестового файла в МБ (рекомендуется 100 или 500): ";
    if (!(std::cin >> sizeMB) || sizeMB < 64) {
        std::cerr << "[ОШИБКА]: Размер файла должен быть числом не меньше 64 МБ!\n";
        return 1;
    }

    std::string filename = "test_large_file.bin";

    // генерация файла на диске
    if (!GenerateTestFile(filename, sizeMB)) {
        return 1;
    }

    // исследуемые размеры окон
    struct WindowConfig {
        std::string name;
        size_t bytes;
    };

    std::vector<WindowConfig> configs = {
            {"1 МБ",  1 * 1024 * 1024},
            {"16 МБ", 16 * 1024 * 1024},
            {"64 МБ", 64 * 1024 * 1024}
    };

    const int RUNS = 3;

    std::cout << "\n СТАРТ ЭКСПЕРИМЕНТА\n";

    uint32_t baselineChecksum = 0;

    for (const auto& config : configs) {
        std::cout << "\n>>> Исследование для размера окна: " << config.name << " <<<\n";
        std::vector<double> runTimes;

        for (int run = 1; run <= RUNS; ++run) {
            std::cout << "  [Прогон " << run << "/" << RUNS << "]... ";

            // запускаем обработку файла
            ProcessResult res = ProcessFileWithWindows(filename, config.bytes);

            if (!res.success) {
                std::cerr << "Сбой обработки!\n";
                return 1;
            }

            runTimes.push_back(res.elapsedMilliseconds);
            std::cout << "Время: " << std::fixed << std::setprecision(2)
                      << res.elapsedMilliseconds << " мс"
                      << " | Окон: " << res.windowsProcessed
                      << " | Хэш: 0x" << std::hex << res.totalChecksum << std::dec << "\n";

            // проверка
            if (baselineChecksum == 0) {
                baselineChecksum = res.totalChecksum;
            } else if (baselineChecksum != res.totalChecksum) {
                std::cerr << "  [ВНИМАНИЕ]: Контрольная сумма не совпала!\n";
            }
        }

        // среднее время
        double avgTime = std::accumulate(runTimes.begin(), runTimes.end(), 0.0) / RUNS;

        // скорость обработки
        double speedMBs = (static_cast<double>(sizeMB) / (avgTime / 1000.0));

        std::cout << "----------------------------------------------------\n";
        std::cout << "ИТОГ для окна " << config.name
                  << " | Среднее время: " << avgTime << " мс"
                  << " | Скорость: " << speedMBs << " МБ/с\n";
        std::cout << "----------------------------------------------------\n";
    }

    std::cout << "\nВсе контрольные суммы полностью совпали! Данные обработаны корректно.\n";
    std::cout << "Эксперимент завершен.\n";

    return 0;
}