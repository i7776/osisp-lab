#pragma once
#include <windows.h>

class HighResolutionTimer {
private:
    long long freq = 0;
    long long start = 0;
    long long end = 0;

public:
    HighResolutionTimer() {
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    }

    void Start() {
        QueryPerformanceCounter((LARGE_INTEGER*)&start);
    }

    void Stop() {
        QueryPerformanceCounter((LARGE_INTEGER*)&end);
    }

    double GetMilliseconds() const {
        return static_cast<double>(end - start) * 1000.0 / static_cast<double>(freq);
    }
};


