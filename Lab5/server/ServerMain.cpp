#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "../common/Protocol.h"
#include "../common/SocketUtils.h"
#include "TextAnalyzer.h"

// форматированный вывод журнала сервера со временем
void LogServer(const std::string& msg) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("[%02d:%02d:%02d.%03d] [SERVER] %s\n",
           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg.c_str());
}

// функция потока для обслуживания подключенного клиента
void HandleClient(SOCKET clientSocket, sockaddr_in clientAddr) {
    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddr.sin_port);

    LogServer("CLIENT_CONNECTED ip=" + std::string(clientIp) + " port=" + std::to_string(clientPort));

    while (true) {
        // чтение фиксированного заголовка
        MessageHeader header{};
        if (!NetUtils::RecvAll(clientSocket, &header, sizeof(header))) {
            LogServer("CLIENT_DISCONNECTED ip=" + std::string(clientIp) + " port=" + std::to_string(clientPort));
            break;
        }

        // преобразование полей из сетевого порядка в порядок хоста
        unsigned int payloadLen = ntohl(header.payloadLength);
        unsigned short version = ntohs(header.version);
        unsigned short msgType = ntohs(header.msgType);
        unsigned int reqId = ntohl(header.requestId);
        unsigned int clientId = ntohl(header.clientId);

        // валидация версии протокола и ограничения размера
        if (version != PROTOCOL_VERSION || payloadLen > MAX_PAYLOAD_SIZE) {
            LogServer("PROTOCOL_ERROR: invalid version or oversized payload");
            NetUtils::SendMessagePacket(clientSocket, MSG_ERROR, reqId, clientId, nullptr, 0);
            break;
        }

        // обработка штатного отключения
        if (msgType == MSG_DISCONNECT) {
            LogServer("CLIENT_DISCONNECT_REQUEST clientId=" + std::to_string(clientId));
            break;
        }

        // обработка запроса на анализ текста
        if (msgType == MSG_TEXT_REQUEST) {
            std::string textBuffer(payloadLen, '\0');
            if (payloadLen > 0) {
                if (!NetUtils::RecvAll(clientSocket, textBuffer.data(), payloadLen)) {
                    LogServer("ERROR reading payload from clientId=" + std::to_string(clientId));
                    break;
                }
            }

            LogServer("RECV requestId=" + std::to_string(reqId) + " clientId=" + std::to_string(clientId) +
                      " size=" + std::to_string(payloadLen));

            // выполнение анализа текста
            AnalysisResponsePayload stats = TextAnalyzer::Analyze(textBuffer);

            // отправка результата анализа клиенту
            bool sentOk = NetUtils::SendMessagePacket(
                    clientSocket,
                    MSG_ANALYSIS_RESULT,
                    reqId,
                    clientId,
                    &stats,
                    sizeof(stats)
            );

            if (!sentOk) {
                LogServer("ERROR sending result to clientId=" + std::to_string(clientId));
                break;
            }

            LogServer("SEND requestId=" + std::to_string(reqId) + " bytes=" + std::to_string(stats.totalBytes) +
                      " lines=" + std::to_string(stats.totalLines) + " words=" + std::to_string(stats.totalWords) +
                      " digits=" + std::to_string(stats.totalDigits) + " letters=" + std::to_string(stats.totalLetters));
        }
    }

    // корректное закрытие сокета клиента
    shutdown(clientSocket, SD_BOTH);
    closesocket(clientSocket);
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    unsigned short port = DEFAULT_PORT;
    if (argc >= 3 && std::string(argv[1]) == "--port") {
        port = static_cast<unsigned short>(std::stoi(argv[2]));
    }

    if (!NetUtils::InitializeWinsock()) {
        return 1;
    }

    // создание слушающего сокета tcp
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << NetUtils::GetLastErrorMessage(WSAGetLastError()) << "\n";
        NetUtils::CleanupWinsock();
        return 1;
    }

    // включение повторного использования адреса
    int optval = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(optval));

    // настройка адреса сервера
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, DEFAULT_HOST, &serverAddr.sin_addr);

    // привязка сокета к адресу
    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed: " << NetUtils::GetLastErrorMessage(WSAGetLastError()) << "\n";
        closesocket(listenSocket);
        NetUtils::CleanupWinsock();
        return 1;
    }

    // перевод сокета в режим ожидания подключений
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen() failed: " << NetUtils::GetLastErrorMessage(WSAGetLastError()) << "\n";
        closesocket(listenSocket);
        NetUtils::CleanupWinsock();
        return 1;
    }

    LogServer("SERVER_START port=" + std::to_string(port) + " student_id=72 host=" + std::string(DEFAULT_HOST));
    std::cout << "[info] server listening. press ctrl+c to terminate.\n";

    // цикл приема входящих клиентских соединений
    while (true) {
        sockaddr_in clientAddr{};
        int addrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept() failed: " << NetUtils::GetLastErrorMessage(WSAGetLastError()) << "\n";
            break;
        }

        // обслуживание каждого клиента в отдельном потоке
        std::thread t(HandleClient, clientSocket, clientAddr);
        t.detach();
    }

    closesocket(listenSocket);
    NetUtils::CleanupWinsock();
    return 0;
}