#ifndef UTILS_H
#define UTILS_H

#include <string>

namespace Utils {
    // форматирование размера байтов в Б, КБ, МБ или ГБ
    std::string FormatBytes(unsigned long long bytes);

    // создание тестового файла заданного размера в мегабайтах
    bool CreateDummyFile(const std::string& filePath, unsigned int sizeMb, unsigned int seed);

    // удаление тестового файла
    bool DeleteDummyFile(const std::string& filePath);

    // таймер
    long long GetTicks();
    double CalculateMs(long long startTicks, long long endTicks);

    // вывод системного журнала с отметкой времени
    void LogMessage(const std::string& action, const std::string& details, const std::string& status);
}

#endif // UTILS_H