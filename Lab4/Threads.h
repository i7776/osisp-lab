#pragma once
#include "SharedData.h"

// параметры для одного рабочего потока
struct ThreadArgs {
    int threadId;
    SharedArray* sharedArray;
    ExperimentConfig config;
    ReaderStats stats; // только для читателей
};

// ф-ии потоков WinAPI
DWORD WINAPI WriterThreadProc(LPVOID lpParam);
DWORD WINAPI ReaderThreadProc(LPVOID lpParam);

void RunExperiment(ExperimentConfig config);