#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include "Protocol.h"

namespace NetUtils {
    // инициализация сетевой библиотеки winsock
    bool InitializeWinsock();

    // освобождение ресурсов winsock
    void CleanupWinsock();

    // отправка буфера
    bool SendAll(SOCKET sock, const void* buffer, unsigned int length);

    // гарантированный прием буфера
    bool RecvAll(SOCKET sock, void* buffer, unsigned int length);

    // упаковка заголовка в сетевой порядок байтов и отправка
    bool SendMessagePacket(SOCKET sock, unsigned short type, unsigned int reqId, unsigned int clientId, const void* payload, unsigned int payloadLen);

    // текстовое описание системного кода ошибки winsock
    std::string GetLastErrorMessage(int errorCode);

    // функции замера времени
    long long GetTimeTicks();
    double CalculateMs(long long startTicks, long long endTicks);
}

#endif // SOCKET_UTILS_H