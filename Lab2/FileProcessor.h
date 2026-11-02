#pragma once
#include <string>
#include <cstdint>

// структура для возврата результата работы
struct ProcessResult {
    bool success = false;          // успешно ли прошла обработка
    double elapsedMilliseconds = 0;// время работы в мс
    uint32_t totalChecksum = 0;    // итоговая сумма по всему файлу
    size_t windowsProcessed = 0;   // кусков было обработано
};

// обрабатывает файл
ProcessResult ProcessFileWithWindows(const std::string& filename, size_t windowSize);