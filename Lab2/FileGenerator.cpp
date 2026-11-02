#include "FileGenerator.h"
#include "Utils.h"
#include <windows.h>
#include <vector>
#include <iostream>

bool GenerateTestFile(const std::string& filename, size_t sizeInMB) {
    std::cout << "Создание тестового файла \"" << filename
              << "\" размером " << sizeInMB << " МБ...\n";

    // создаем файл
    HANDLE hFile = CreateFileA(
            filename.c_str(),
            GENERIC_WRITE,
            0,                      // только мы доступ
            NULL,
            CREATE_ALWAYS,          // перезаписать, если файл уже существует
            FILE_ATTRIBUTE_NORMAL,
            NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        PrintError("CreateFile (генерация файла)");
        return false;
    }

    // пишем кусками по 1 мб
    const size_t BUFFER_SIZE = 1024 * 1024;
    std::vector<char> buffer(BUFFER_SIZE);

    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
        buffer[i] = static_cast<char>((i % 256));
    }

    for (size_t i = 0; i < sizeInMB; ++i) {
        DWORD bytesWritten = 0;

        // запись
        BOOL success = WriteFile(
                hFile,              // дескриптор файла
                buffer.data(),      // откуда
                BUFFER_SIZE,        // сколько
                &bytesWritten,      // сколько реально записалось
                NULL                // синхронная запись
        );

        if (!success) {
            PrintError("WriteFile");
            CloseHandle(hFile);
            return false;
        }
    }

    CloseHandle(hFile);
    std::cout << "Файл успешно создан!\n";
    return true;
}