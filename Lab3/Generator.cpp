#include "Generator.h"
#include "Protocol.h"
#include "PipeHelper.h"
#include <iostream>
#include <string>

void RunGenerator(int runId, int taskCount) {
    // идентификация процесса
    std::cout << "====================================================\n";
    std::cout << "ROLE: GENERATOR | PID=" << GetCurrentProcessId()
              << " | VARIANT=" << VARIANT_ID
              << " | RUN_ID=" << runId << "\n";
    std::cout << "====================================================\n\n";

    // подключаемся к Процессору
    std::cout << "[Generator] Подключение к входному каналу Процессора...\n";
    HANDLE hPipe = ConnectClientPipe(PIPE_GEN_TO_PROC);
    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "[Generator] Не удалось подключиться к Процессору!\n";
        return;
    }
    std::cout << "[Generator] Успешно подключено! Начинаем генерацию " << taskCount << " задач...\n\n";

    // цикл генерации и отправки заданий
    for (int i = 1; i <= taskCount; ++i) {
        PipeMessage msg{};
        msg.version = 1;
        msg.messageType = MSG_TASK;
        msg.sourcePid = GetCurrentProcessId();
        msg.requestId = static_cast<unsigned int>(i);
        msg.retryCount = 0; // для свежей задачи повторов пока 0

        std::string text = "Input data payload for task #" + std::to_string(i);
        strcpy_s(msg.payload, sizeof(msg.payload), text.c_str());
        msg.payloadSize = static_cast<unsigned int>(text.size());

        if (!SendMessageToPipe(hPipe, msg, runId)) {
            std::cerr << "[Generator] Ошибка отправки задания #" << i << "\n";
            break;
        }

        // микро-пауза, чтобы в консоли было видно движение конвейера
        Sleep(2);
    }

    // отправляем сигнал STOP для штатного завершения всего конвейера
    std::cout << "\n[Generator] Все задачи отправлены. Отправка сигнала STOP...\n";
    PipeMessage stopMsg{};
    stopMsg.version = 1;
    stopMsg.messageType = MSG_STOP;
    stopMsg.sourcePid = GetCurrentProcessId();
    stopMsg.requestId = 0;
    stopMsg.retryCount = 0;
    stopMsg.payloadSize = 0;
    SendMessageToPipe(hPipe, stopMsg, runId);

    std::cout << "[Generator] Работа успешно завершена!\n";

    // закрытие дескриптора
    CloseHandle(hPipe);
}