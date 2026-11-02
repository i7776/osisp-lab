#include "Validator.h"
#include "Protocol.h"
#include "PipeHelper.h"
#include <iostream>

void RunValidator(int runId) {
    // идентификация процесса
    std::cout << "====================================================\n";
    std::cout << "ROLE: VALIDATOR | PID=" << GetCurrentProcessId()
              << " | VARIANT=" << VARIANT_ID
              << " | RUN_ID=" << runId << "\n";
    std::cout << "====================================================\n\n";

    // создаем серверную трубу для приема данных от Процессора
    HANDLE hPipeIn = CreateServerPipe(PIPE_PROC_TO_VAL);
    if (hPipeIn == INVALID_HANDLE_VALUE) return;

    std::cout << "[Validator] Ожидание подключения Процессора...\n";
    if (!ConnectNamedPipe(hPipeIn, NULL)) {
        if (GetLastError() != ERROR_PIPE_CONNECTED) {
            PrintError("ConnectNamedPipe в Validator");
            CloseHandle(hPipeIn);
            return;
        }
    }
    std::cout << "[Validator] Процессор подключен!\n";

    // подключаемся к Коллектору (куда скидывать итоги)
    std::cout << "[Validator] Подключение к Коллектору...\n";
    HANDLE hPipeToCollector = ConnectClientPipe(PIPE_VAL_TO_COL);
    if (hPipeToCollector == INVALID_HANDLE_VALUE) {
        CloseHandle(hPipeIn);
        return;
    }
    std::cout << "[Validator] Соединение с Коллектором установлено!\n";

    // подключаемся к Процессору по обратному каналу (для повторов)
    std::cout << "[Validator] Подключение к обратному каналу Процессора (Retry)...\n";
    HANDLE hPipeToProcessorRetry = ConnectClientPipe(PIPE_VAL_TO_PROC);
    if (hPipeToProcessorRetry == INVALID_HANDLE_VALUE) {
        CloseHandle(hPipeIn);
        CloseHandle(hPipeToCollector);
        return;
    }
    std::cout << "[Validator] Обратный канал повторов готов!\n\n";

    PipeMessage msg;
    int forwardedOk = 0;
    int retriedCount = 0;
    int droppedCount = 0;

    // основной цикл валидации сообщений
    while (ReceiveMessageFromPipe(hPipeIn, msg, runId)) {
        // Если пришла команда завершения
        if (msg.messageType == MSG_STOP) {
            std::cout << "\n[Validator] Получен STOP. Пересылка в Коллектор и завершение.\n";
            SendMessageToPipe(hPipeToCollector, msg, runId);
            break;
        }

        // УСПЕШНАЯ ЗАДАЧА -> отправляем дальше в Коллектор
        if (msg.messageType == MSG_RESULT_OK) {
            forwardedOk++;
            msg.sourcePid = GetCurrentProcessId();
            SendMessageToPipe(hPipeToCollector, msg, runId);
        }
            // ОШИБКА ОБРАБОТКИ -> проверяем число попыток
        else if (msg.messageType == MSG_RESULT_ERR) {
            // НЕ БОЛЕЕ двух повторов
            if (msg.retryCount < 2) {
                msg.retryCount++;
                msg.messageType = MSG_TASK; // снова превращаем в задание
                msg.sourcePid = GetCurrentProcessId();

                retriedCount++;
                std::cout << ">>> [Validator] ВНИМАНИЕ: Задание reqId=" << msg.requestId
                          << " с ошибкой! Отправка на повтор #" << msg.retryCount << " <<<\n";

                // отправляем назад в Процессор по каналу повторов
                SendMessageToPipe(hPipeToProcessorRetry, msg, runId);
            } else {
                // превышен лимит из 2 повторов -> отбрасываем задачу в Коллектор как сбой
                droppedCount++;
                std::cout << "!!! [Validator] ФАТАЛЬНЫЙ СБОЙ: Задание reqId=" << msg.requestId
                          << " исчерпало лимит 2 повторов! Сброс в Коллектор. !!!\n";

                msg.sourcePid = GetCurrentProcessId();
                SendMessageToPipe(hPipeToCollector, msg, runId);
            }
        }
    }

    // итоговый отчет Валидатора
    std::cout << "\n================ СТАТИСТИКА ВАЛИДАТОРА ================\n";
    std::cout << "Пропущено успешных: " << forwardedOk << "\n";
    std::cout << "Отправлено на повторную обработку: " << retriedCount << "\n";
    std::cout << "Отброшено после 2 повторов: " << droppedCount << "\n";
    std::cout << "=======================================================\n";

    // закрытие всех открытых дескрипторов
    DisconnectNamedPipe(hPipeIn);
    CloseHandle(hPipeIn);
    CloseHandle(hPipeToCollector);
    CloseHandle(hPipeToProcessorRetry);
}