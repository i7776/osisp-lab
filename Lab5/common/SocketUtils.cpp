#include "SocketUtils.h"
#include <iostream>

namespace NetUtils {

    bool InitializeWinsock() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "[winsock error] wsastartup failed: " << GetLastErrorMessage(result) << "\n";
            return false;
        }
        return true;
    }

    void CleanupWinsock() {
        WSACleanup();
    }

    bool SendAll(SOCKET sock, const void* buffer, unsigned int length) {
        const char* ptr = static_cast<const char*>(buffer);
        unsigned int totalSent = 0;

        // крутим цикл, пока не отправим ровно length байтов
        while (totalSent < length) {
            int sent = send(sock, ptr + totalSent, static_cast<int>(length - totalSent), 0);
            if (sent == SOCKET_ERROR) {
                int err = WSAGetLastError();
                std::cerr << "[socket error] send() failed with code " << err << ": " << GetLastErrorMessage(err) << "\n";
                return false;
            }
            totalSent += static_cast<unsigned int>(sent);
        }
        return true;
    }

    bool RecvAll(SOCKET sock, void* buffer, unsigned int length) {
        char* ptr = static_cast<char*>(buffer);
        unsigned int totalReceived = 0;

        // крутим цикл, пока не вычитаем ровно length байтов из потока tcp
        while (totalReceived < length) {
            int received = recv(sock, ptr + totalReceived, static_cast<int>(length - totalReceived), 0);
            if (received == 0) {
                // соединение закрыто удаленной стороной
                return false;
            }
            if (received == SOCKET_ERROR) {
                int err = WSAGetLastError();
                std::cerr << "[socket error] recv() failed with code " << err << ": " << GetLastErrorMessage(err) << "\n";
                return false;
            }
            totalReceived += static_cast<unsigned int>(received);
        }
        return true;
    }

    bool SendMessagePacket(SOCKET sock, unsigned short type, unsigned int reqId, unsigned int clientId, const void* payload, unsigned int payloadLen) {
        MessageHeader header{};
        // перевод числовых полей в сетевой порядок байтов
        header.payloadLength = htonl(payloadLen);
        header.version = htons(PROTOCOL_VERSION);
        header.msgType = htons(type);
        header.requestId = htonl(reqId);
        header.clientId = htonl(clientId);

        // передача фиксированного заголовка
        if (!SendAll(sock, &header, sizeof(header))) {
            return false;
        }

        // передача полезной нагрузки
        if (payloadLen > 0 && payload != nullptr) {
            if (!SendAll(sock, payload, payloadLen)) {
                return false;
            }
        }
        return true;
    }

    std::string GetLastErrorMessage(int errorCode) {
        char* msgBuf = nullptr;
        FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPSTR>(&msgBuf), 0, nullptr
        );
        std::string message = msgBuf ? msgBuf : "unknown error";
        if (msgBuf) {
            LocalFree(msgBuf);
        }
        return message;
    }

    long long GetTimeTicks() {
        LARGE_INTEGER li;
        QueryPerformanceCounter(&li);
        return li.QuadPart;
    }

    double CalculateMs(long long startTicks, long long endTicks) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return static_cast<double>(endTicks - startTicks) * 1000.0 / static_cast<double>(freq.QuadPart);
    }

}