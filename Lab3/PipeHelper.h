#pragma once
#include <windows.h>
#include <iostream>
#include "Protocol.h"
#include "Utils.h"

// создание серверного канала (для принимающей стороны)
inline HANDLE CreateServerPipe(const char* pipeName) {
    HANDLE hPipe = CreateNamedPipeA(
            pipeName,
            PIPE_ACCESS_DUPLEX,       // чтение и запись
            PIPE_TYPE_MESSAGE |       // передаем сообщениями
            PIPE_READMODE_MESSAGE |   // читаем сообщениями
            PIPE_WAIT,                // режим ждать данных
            1,                        // 1 экземпляр канала
            sizeof(PipeMessage) * 16, // размер буфера вывода
            sizeof(PipeMessage) * 16, // размер буфера ввода
            NMPWAIT_USE_DEFAULT_WAIT, // таймаут по умолчанию
            NULL                      // стандартная безопасность
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        PrintError("CreateNamedPipe: " + std::string(pipeName));
    }
    return hPipe;
}

// подключение клиента к каналу (для отправляющей стороны)
inline HANDLE ConnectClientPipe(const char* pipeName) {
    // ждем до 5 секунд, пока сервер создаст канал
    if (!WaitNamedPipeA(pipeName, 5000)) {
        PrintError("WaitNamedPipe (сервер не ответил за 5 сек): " + std::string(pipeName));
        return INVALID_HANDLE_VALUE;
    }

    // подключаемся к трубе
    HANDLE hPipe = CreateFileA(
            pipeName,
            GENERIC_READ | GENERIC_WRITE, // читаем и пишем
            0,                            // без разделения
            NULL,
            OPEN_EXISTING,                // открываем существующую трубу
            0,
            NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        PrintError("CreateFile (подключение к Pipe): " + std::string(pipeName));
    }
    return hPipe;
}

// отправка структуры PipeMessage в канал
inline bool SendMessageToPipe(HANDLE hPipe, const PipeMessage& msg, int runId) {
    DWORD bytesWritten = 0;
    BOOL success = WriteFile(
            hPipe,
            &msg,
            sizeof(PipeMessage),
            &bytesWritten,
            NULL
    );

    if (!success || bytesWritten != sizeof(PipeMessage)) {
        PrintError("WriteFile в Pipe");
        return false;
    }

    LogEvent("SEND", msg, runId); // логируем отправку
    return true;
}

// чтение структуры PipeMessage из канала
inline bool ReceiveMessageFromPipe(HANDLE hPipe, PipeMessage& msg, int runId) {
    DWORD bytesRead = 0;
    BOOL success = ReadFile(
            hPipe,
            &msg,
            sizeof(PipeMessage),
            &bytesRead,
            NULL
    );

    // если труба закрылась с того конца (клиент отключился)
    if (!success) {
        DWORD err = GetLastError();
        if (err == ERROR_BROKEN_PIPE || err == ERROR_PIPE_NOT_CONNECTED) {
            // штатное закрытие канала другим процессом
            return false;
        }
        PrintError("ReadFile из Pipe");
        return false;
    }

    if (bytesRead == sizeof(PipeMessage)) {
        LogEvent("RECEIVE", msg, runId); // логируем
        return true;
    }

    return false;
}