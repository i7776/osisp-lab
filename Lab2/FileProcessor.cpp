#include "FileProcessor.h"
#include "Utils.h"
#include "Timer.h"
#include <windows.h>
#include <iostream>

// вычисляет контрольную сумму куска памяти
static uint32_t CalculateChecksum(const char* data, size_t size) {
    uint32_t sum = 0;
    for (size_t i = 0; i < size; ++i) {
        sum += static_cast<uint8_t>(data[i]); // складываем каждый байт
    }
    return sum;
}

ProcessResult ProcessFileWithWindows(const std::string& filename, size_t windowSize) {
    ProcessResult result;

    // узнаем системную гранулярность памяти
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD granularity = sysInfo.dwAllocationGranularity; // 64 кб

    //  кратен размер гранулярности
    if (windowSize % granularity != 0) {
        std::cerr << "[ОШИБКА]: Размер окна (" << windowSize << ") не кратен гранулярности системы (" << granularity << ")!\n";
        return result;
    }

    // открываем файл для чтения
    HANDLE hFile = CreateFileA(
            filename.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,        // разрешаем другим процессам читать
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        PrintError("CreateFile (чтение файла)");
        return result;
    }

    // узнаем точный размер файла
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        PrintError("GetFileSizeEx");
        CloseHandle(hFile);
        return result;
    }

    uint64_t totalBytes = static_cast<uint64_t>(fileSize.QuadPart);
    if (totalBytes == 0) {
        std::cerr << "[ОШИБКА]: Файл пуст!\n";
        CloseHandle(hFile);
        return result;
    }

    // создаем объект отображения файла
    HANDLE hMapping = CreateFileMappingA(
            hFile,
            NULL,
            PAGE_READONLY,          // только для чтения
            0, 0, // полностью
            NULL
    );

    if (hMapping == NULL) {
        PrintError("CreateFileMapping");
        CloseHandle(hFile);
        return result;
    }

    // просматриваем файл кусочками
    HighResolutionTimer timer;
    timer.Start();

    uint64_t currentOffset = 0;
    size_t windowIndex = 0;
    uint32_t combinedChecksum = 0;

    while (currentOffset < totalBytes) {
        // байт осталось
        uint64_t bytesRemaining = totalBytes - currentOffset;

        // размер текущего окна
        size_t currentWindowBytes = (bytesRemaining < windowSize) ? static_cast<size_t>(bytesRemaining) : windowSize;

        // смещение в MapViewOfFile передается двумя 32-битными числами (старшая и младшая часть)
        DWORD offsetHigh = static_cast<DWORD>(currentOffset >> 32);
        DWORD offsetLow  = static_cast<DWORD>(currentOffset & 0xFFFFFFFF);

        // прикладываем рамку
        char* mappedPtr = static_cast<char*>(MapViewOfFile(
                hMapping,
                FILE_MAP_READ,
                offsetHigh,
                offsetLow,
                currentWindowBytes
        ));

        if (mappedPtr == nullptr) {
            PrintError("MapViewOfFile на смещении " + std::to_string(currentOffset));
            CloseHandle(hMapping);
            CloseHandle(hFile);
            return result;
        }

        // конрольная сумма окна
        uint32_t windowChecksum = CalculateChecksum(mappedPtr, currentWindowBytes);
        combinedChecksum += windowChecksum; // объединяем контрольные суммы

        // убираем рамку
        if (!UnmapViewOfFile(mappedPtr)) {
            PrintError("UnmapViewOfFile");
        }

        // сдвигаем смещение дальше по файлу
        currentOffset += currentWindowBytes;
        windowIndex++;
    }

    timer.Stop();

    CloseHandle(hMapping);
    CloseHandle(hFile);

    result.success = true;
    result.elapsedMilliseconds = timer.GetMilliseconds();
    result.totalChecksum = combinedChecksum;
    result.windowsProcessed = windowIndex;

    return result;
}