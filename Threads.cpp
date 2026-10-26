#include "Threads.h"
#include <random>
#include <iomanip>

// ПОТОК ПИСАТЕЛЯ
DWORD WINAPI WriterThreadProc(LPVOID lpParam) {
    auto* args = static_cast<ThreadArgs*>(lpParam);
    auto* arr = args->sharedArray;

    // генератор чисел для каждого потока
    // зерно уникально для каждого писателя
    std::mt19937 gen(SEED_BASE + args->threadId);
    std::uniform_int_distribution<int> indexDist(0, ARRAY_SIZE - 1);
    std::uniform_int_distribution<int> deltaDist(1, 5); // Переносим от 1 до 5 единиц

    int groupsCount = OPERATIONS_COUNT / args->config.groupSize;

    for (int g = 0; g < groupsCount; ++g) {

        // режим 1: блокировка на всю группу операций
        if (args->config.strategy == LOCK_GROUP) {
            AcquireSRWLockExclusive(&arr->lock); // закрыли дверь

            for (int k = 0; k < args->config.groupSize; ++k) {
                int src = indexDist(gen);
                int dst = indexDist(gen);
                if (src == dst) continue;

                int delta = deltaDist(gen);
                arr->data[src] -= delta;
                arr->data[dst] += delta; // сумма массива сохраняется
            }

            ReleaseSRWLockExclusive(&arr->lock); // открыли дверь
        }
            // режим 2: блокировка отдельно на каждый перенос
        else {
            for (int k = 0; k < args->config.groupSize; ++k) {
                int src = indexDist(gen);
                int dst = indexDist(gen);
                if (src == dst) continue;
                int delta = deltaDist(gen);

                AcquireSRWLockExclusive(&arr->lock); // захватили на 1 перенос
                arr->data[src] -= delta;
                arr->data[dst] += delta;
                ReleaseSRWLockExclusive(&arr->lock); // отпустили
            }
        }
    }

    return 0;
}

// ПОТОК ЧИТАТЕЛЯ
DWORD WINAPI ReaderThreadProc(LPVOID lpParam) {
    auto* args = static_cast<ThreadArgs*>(lpParam);
    auto* arr = args->sharedArray;
    PrecisionTimer waitTimer;

    for (int i = 0; i < OPERATIONS_COUNT; ++i) {
        // засекаем время: сколько читатель ждет освобождения замка в очереди
        waitTimer.Start();
        AcquireSRWLockShared(&arr->lock); // совместный захват
        waitTimer.Stop();

        // фиксируем задержку ожидания
        args->stats.totalWaitTimeMs += (waitTimer.GetMicroseconds() / 1000.0);
        args->stats.totalReadOps++;

        // считаем сумму всех 100 элементов
        int currentSum = 0;
        for (int j = 0; j < ARRAY_SIZE; ++j) {
            currentSum += arr->data[j];
        }

        //  проверка инварианта (сумма 1000)
        if (currentSum != EXPECTED_SUM) {
            args->stats.invariantViolations++;
        }

        ReleaseSRWLockShared(&arr->lock); // освободили замок
    }

    return 0;
}

void RunExperiment(ExperimentConfig config) {
    SharedArray sharedArray;
    sharedArray.Initialize();

    int totalThreads = config.readersCount + config.writersCount;
    std::vector<HANDLE> hThreads(totalThreads);
    std::vector<ThreadArgs> threadArgs(totalThreads);

    PrecisionTimer totalTimer;
    totalTimer.Start();

    int threadIdx = 0;

    // создаем потоки читателей
    for (int i = 0; i < config.readersCount; ++i) {
        threadArgs[threadIdx] = { i + 1, &sharedArray, config, {} };
        hThreads[threadIdx] = CreateThread(NULL, 0, ReaderThreadProc, &threadArgs[threadIdx], 0, NULL);
        threadIdx++;
    }

    // создаем потоки писателей
    for (int i = 0; i < config.writersCount; ++i) {
        threadArgs[threadIdx] = { i + 1, &sharedArray, config, {} };
        hThreads[threadIdx] = CreateThread(NULL, 0, WriterThreadProc, &threadArgs[threadIdx], 0, NULL);
        threadIdx++;
    }

    // завершения всех потоков
    WaitForMultipleObjects(totalThreads, hThreads.data(), TRUE, INFINITE);
    totalTimer.Stop();

    for (HANDLE h : hThreads) {
        CloseHandle(h);
    }

    // сбор статистики читателей
    double sumWaitTimeMs = 0.0;
    long long totalReads = 0;
    int totalViolations = 0;

    for (int i = 0; i < config.readersCount; ++i) {
        sumWaitTimeMs += threadArgs[i].stats.totalWaitTimeMs;
        totalReads += threadArgs[i].stats.totalReadOps;
        totalViolations += threadArgs[i].stats.invariantViolations;
    }

    // среднее время ожидания одного читателя
    double avgWaitMicroseconds = (sumWaitTimeMs * 1000.0) / totalReads;

    // пропускная способность
    long long totalOperations = (config.writersCount * OPERATIONS_COUNT) + (config.readersCount * OPERATIONS_COUNT);
    double throughputOpsPerSec = static_cast<double>(totalOperations) / (totalTimer.GetMilliseconds() / 1000.0);

    std::string stratName = (config.strategy == LOCK_GROUP) ? "ГРУППОВАЯ (Lock Group)" : "ПООДИНОЧНАЯ (Lock Each)";
    std::cout << "| " << std::left << std::setw(23) << stratName
              << "| " << std::setw(6) << config.groupSize
              << "| " << std::setw(15) << std::fixed << std::setprecision(2) << totalTimer.GetMilliseconds()
              << "| " << std::setw(17) << std::fixed << std::setprecision(2) << avgWaitMicroseconds
              << "| " << std::setw(17) << static_cast<long long>(throughputOpsPerSec)
              << "| " << std::setw(12) << (totalViolations == 0 ? "0 (OK)" : "СБОЙ!")
              << " |\n";
}
