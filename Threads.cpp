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
        // засекаем время: сколько читатель ждет освобождения замка в очереди!
        waitTimer.Start();
        AcquireSRWLockShared(&arr->lock); // СОВМЕСТНЫЙ ЗАХВАТ
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