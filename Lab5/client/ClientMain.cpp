#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <iomanip>
#include <windows.h>
#include "../common/Protocol.h"
#include "../common/SocketUtils.h"

// форматированный вывод журнала клиента
void LogClient(const std::string& msg) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("[%02d:%02d:%02d.%03d] [CLIENT] %s\n",
           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg.c_str());
}

// генерация детерминированного текста заданного размера на основе seed
std::string GenerateSampleText(unsigned int targetLength, unsigned int seed) {
    std::mt19937 rng(seed);
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789   \n";
    const size_t charsetSize = sizeof(charset) - 1;

    std::string result;
    result.reserve(targetLength);
    for (unsigned int i = 0; i < targetLength; ++i) {
        result.push_back(charset[rng() % charsetSize]);
    }
    return result;
}

// отправка одного запроса на анализ текста с замером времени
bool SendAnalysisRequest(SOCKET sock, unsigned int reqId, unsigned int clientId, const std::string& text, AnalysisResponsePayload& outResponse, double& outDurationMs) {
    long long startTicks = NetUtils::GetTimeTicks();

    // отправка сообщения серверу
    if (!NetUtils::SendMessagePacket(sock, MSG_TEXT_REQUEST, reqId, clientId, text.data(), static_cast<unsigned int>(text.size()))) {
        return false;
    }

    // чтение заголовка ответа
    MessageHeader respHeader{};
    if (!NetUtils::RecvAll(sock, &respHeader, sizeof(respHeader))) {
        return false;
    }

    unsigned int payloadLen = ntohl(respHeader.payloadLength);
    unsigned short msgType = ntohs(respHeader.msgType);

    if (msgType == MSG_ERROR || payloadLen != sizeof(AnalysisResponsePayload)) {
        return false;
    }

    // чтение тела ответа (статистики)
    if (!NetUtils::RecvAll(sock, &outResponse, sizeof(outResponse))) {
        return false;
    }

    long long endTicks = NetUtils::GetTimeTicks();
    outDurationMs = NetUtils::CalculateMs(startTicks, endTicks);
    return true;
}

// эмуляция отправки для проверки устойчивости к частичному tcp recv
bool TestPartialTcpSend(SOCKET sock, unsigned int reqId, unsigned int clientId) {
    std::string text = "TCP fragmentation test string 12345";
    unsigned int payloadLen = static_cast<unsigned int>(text.size());

    MessageHeader header{};
    header.payloadLength = htonl(payloadLen);
    header.version = htons(PROTOCOL_VERSION);
    header.msgType = htons(MSG_TEXT_REQUEST);
    header.requestId = htonl(reqId);
    header.clientId = htonl(clientId);

    // собираем полный пакет в один буфер
    std::vector<char> packet(sizeof(header) + payloadLen);
    memcpy(packet.data(), &header, sizeof(header));
    memcpy(packet.data() + sizeof(header), text.data(), payloadLen);

    LogClient("PARTIAL_SEND: sending packet in small 7-byte chunks to test framing...");

    // отправляем пакет мелкими порциями по 7 байт с задержкой
    size_t offset = 0;
    while (offset < packet.size()) {
        size_t chunkSize = min(static_cast<size_t>(7), packet.size() - offset);
        send(sock, packet.data() + offset, static_cast<int>(chunkSize), 0);
        offset += chunkSize;
        Sleep(5); // искусственная задержка между фрагментами
    }

    // принимаем ответ от сервера
    MessageHeader respHeader{};
    if (!NetUtils::RecvAll(sock, &respHeader, sizeof(respHeader))) {
        return false;
    }

    AnalysisResponsePayload stats{};
    if (!NetUtils::RecvAll(sock, &stats, sizeof(stats))) {
        return false;
    }

    LogClient("PARTIAL_SEND_OK: received result: bytes=" + std::to_string(stats.totalBytes) +
              " words=" + std::to_string(stats.totalWords));
    return true;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    unsigned short port = DEFAULT_PORT;
    std::string host = DEFAULT_HOST;
    unsigned int clientId = 72; // идентификатор студента

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--port" && i + 1 < argc) port = static_cast<unsigned short>(std::stoi(argv[++i]));
        if (std::string(argv[i]) == "--host" && i + 1 < argc) host = argv[++i];
        if (std::string(argv[i]) == "--id" && i + 1 < argc) clientId = static_cast<unsigned int>(std::stoi(argv[++i]));
    }

    if (!NetUtils::InitializeWinsock()) {
        return 1;
    }

    // создание сокета клиента
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << NetUtils::GetLastErrorMessage(WSAGetLastError()) << "\n";
        NetUtils::CleanupWinsock();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

    LogClient("CONNECTING to " + host + ":" + std::to_string(port) + " (id=" + std::to_string(clientId) + ")...");

    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "connect() failed: " << NetUtils::GetLastErrorMessage(WSAGetLastError()) << "\n";
        closesocket(sock);
        NetUtils::CleanupWinsock();
        return 1;
    }

    LogClient("CONNECTED successfully");

    // функциональные тесты
    std::cout << "\n================= ФУНКЦИОНАЛЬНОЕ ТЕСТИРОВАНИЕ =================\n";
    AnalysisResponsePayload resp{};
    double duration = 0.0;
    unsigned int reqCounter = 100;

    // тест 1.1: пустая строка
    SendAnalysisRequest(sock, ++reqCounter, clientId, "", resp, duration);
    LogClient("TEST_EMPTY_STRING: bytes=" + std::to_string(resp.totalBytes) + " words=" + std::to_string(resp.totalWords));

    // тест 1.2: множественные пробелы и переносы
    SendAnalysisRequest(sock, ++reqCounter, clientId, "word1    word2\n\n\nword3   1234", resp, duration);
    LogClient("TEST_SPACES_NEWLINES: bytes=" + std::to_string(resp.totalBytes) + " words=" + std::to_string(resp.totalWords) +
              " lines=" + std::to_string(resp.totalLines) + " digits=" + std::to_string(resp.totalDigits));

    // тест 1.3: фрагментированный tcp recv
    TestPartialTcpSend(sock, ++reqCounter, clientId);

    // экспериментальное исследование (100, 1000, 10000 байт по 3 запуска)
    std::cout << "\n================ ЭКСПЕРИМЕНТАЛЬНОЕ ИССЛЕДОВАНИЕ ================\n";
    const unsigned int sizes[] = { 100, 1000, 10000 };
    struct ExperimentRecord {
        unsigned int size;
        double runs[3];
        double avgTime;
        double throughputKBps;
    };
    std::vector<ExperimentRecord> expResults;

    for (unsigned int targetSize : sizes) {
        ExperimentRecord rec{};
        rec.size = targetSize;
        std::string sample = GenerateSampleText(targetSize, DEFAULT_SEED + targetSize);
        double totalSum = 0.0;

        for (int run = 0; run < 3; ++run) {
            double runTime = 0.0;
            AnalysisResponsePayload currentResp{};
            SendAnalysisRequest(sock, ++reqCounter, clientId, sample, currentResp, runTime);
            rec.runs[run] = runTime;
            totalSum += runTime;
            Sleep(20);
        }

        rec.avgTime = totalSum / 3.0;
        // пропускная способность: (размер в кб) / (время в секундах)
        rec.throughputKBps = (static_cast<double>(targetSize) / 1024.0) / (rec.avgTime / 1000.0);
        expResults.push_back(rec);
    }

    // печать итоговой таблицы для отчета
    std::cout << "\n----------------------------------------------------------------------------------\n";
    std::cout << "| Размер (байт) | Запуск 1 (мс) | Запуск 2 (мс) | Запуск 3 (мс) | Среднее (мс) | Пропускная сп. (КБ/с) |\n";
    std::cout << "----------------------------------------------------------------------------------\n";
    for (const auto& r : expResults) {
        printf("| %-13u | %-13.4f | %-13.4f | %-13.4f | %-12.4f | %-21.2f |\n",
               r.size, r.runs[0], r.runs[1], r.runs[2], r.avgTime, r.throughputKBps);
    }
    std::cout << "----------------------------------------------------------------------------------\n";

    // завершение сессии
    LogClient("SEND DISCONNECT signal...");
    NetUtils::SendMessagePacket(sock, MSG_DISCONNECT, ++reqCounter, clientId, nullptr, 0);

    shutdown(sock, SD_BOTH);
    closesocket(sock);
    NetUtils::CleanupWinsock();
    LogClient("CLIENT FINISHED successfully");
    return 0;
}