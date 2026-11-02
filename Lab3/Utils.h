#pragma once
#include <windows.h>
#include <iostream>
#include <string>

inline void PrintError(const std::string& operatName){
    auto errorCode = GetLastError();
    if(errorCode == 0) return;

    char* messageBuffer = nullptr;

    FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            errorCode,
            0,
            (char*)&messageBuffer,
            0,
            NULL
    );

    std::cerr << "[ОШИБКА]: " << operatName
              << "\n Код: " << errorCode
              << "\n Описание: " << (messageBuffer ? messageBuffer : "Неизвестно")  << '\n';

    if (messageBuffer != nullptr) {
        LocalFree(messageBuffer);
    }
}