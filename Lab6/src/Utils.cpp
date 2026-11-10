#include "../include/Utils.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

namespace Utils {

    std::string FormatBytes(unsigned long long bytes) {
        const double kb = 1024.0;
        const double mb = kb * 1024.0;
        const double gb = mb * 1024.0;

        char buffer[64];
        if (bytes >= gb) {
            snprintf(buffer, sizeof(buffer), "%.2f ГБ", static_cast<double>(bytes) / gb);
        } else if (bytes >= mb) {
            snprintf(buffer, sizeof(buffer), "%.2f МБ", static_cast<double>(bytes) / mb);
        } else if (bytes >= kb) {
            snprintf(buffer, sizeof(buffer), "%.2f КБ", static_cast<double>(bytes) / kb);
        } else {
            snprintf(buffer, sizeof(buffer), "%llu Б", bytes);
        }
        return std::string(buffer);
    }

    bool CreateDummyFile(const std::string& filePath, unsigned int sizeMb, unsigned int seed) {
        std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            return false;
        }

        // пишем блоками по 1 мегабайту
        const unsigned int bufferSize = 1024 * 1024;
        std::vector<char> buffer(bufferSize);

        // случайное заполнение
        unsigned char val = static_cast<unsigned char>(seed % 255);
        for (size_t i = 0; i < bufferSize; ++i) {
            buffer[i] = static_cast<char>(val + (i % 23));
        }

        for (unsigned int m = 0; m < sizeMb; ++m) {
            file.write(buffer.data(), bufferSize);
            if (!file.good()) {
                file.close();
                return false;
            }
        }

        file.flush();
        file.close();
        return true;
    }

    bool DeleteDummyFile(const std::string& filePath) {
        return DeleteFileA(filePath.c_str()) != 0;
    }

    long long GetTicks() {
        LARGE_INTEGER li;
        QueryPerformanceCounter(&li);
        return li.QuadPart;
    }

    double CalculateMs(long long startTicks, long long endTicks) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return static_cast<double>(endTicks - startTicks) * 1000.0 / static_cast<double>(freq.QuadPart);
    }

    void LogMessage(const std::string& action, const std::string& details, const std::string& status) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        printf("[%02d:%02d:%02d.%03d] %-16s %-35s [%s]\n",
               st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
               action.c_str(), details.c_str(), status.c_str());
    }

}