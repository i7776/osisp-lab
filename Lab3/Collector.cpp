#include "Collector.h"
#include "Protocol.h"
#include "PipeHelper.h"
#include "Utils.h"
#include <iostream>

void RunCollector(int runId) {
    // идентификация процесса на старте
    std::cout << "====================================================\n";
    std::cout << "ROLE: COLLECTOR | PID=" << GetCurrentProcessId()
              << " | VARIANT=" << VARIANT_ID
              << " | RUN_ID=" << runId << "\n";
    std::cout << "====================================================\n\n";

    // создаем серверную трубу, в которую Валидатор будет скидывать итоги
    HANDLE hPipe = CreateServerPipe(PIPE_VAL_TO_COL);
    if (hPipe == INVALID_HANDLE_VALUE) return;

    std::cout << "[Collector] Ожидание подключения Валидатора к каналу...\n";

    // системный вызов Windows API: засыпаем, пока клиент не подключится
    if (!ConnectNamedPipe(hPipe, NULL)) {
        if (GetLastError() != ERROR_PIPE_CONNECTED) {
            PrintError("ConnectNamedPipe в Collector");
            CloseHandle(hPipe);
            return;
        }
    }

    std::cout << "[Collector] Валидатор подключился! Начинаем прием результатов...\n\n";

    int successCount = 0;
    int failedCount = 0;

    PipeMessage msg;

    // цикл приема сообщений
    while (ReceiveMessageFromPipe(hPipe, msg, runId)) {
        if (msg.messageType == MSG_STOP) {
            std::cout << "\n[Collector] Получен сигнал STOP! Завершение работы.\n";
            break;
        }

        if (msg.messageType == MSG_RESULT_OK) {
            successCount++;
        } else if (msg.messageType == MSG_RESULT_ERR) {
            failedCount++; // задача, которую не удалось спасти даже после 2 повторов
        }
    }

    // вывод итоговой статистики
    std::cout << "\n================ ИТОГОВАЯ СТАТИСТИКА ================\n";
    std::cout << "Успешно обработано: " << successCount << "\n";
    std::cout << "Не удалось восстановить (отброшено): " << failedCount << "\n";
    std::cout << "Всего заданий дошло до финиша: " << (successCount + failedCount) << "\n";
    std::cout << "=====================================================\n";

    // закрытие дескриптора ядра
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
}