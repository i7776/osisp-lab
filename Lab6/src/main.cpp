#include <windows.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include "../include/DiskAnalyzer.h"
#include "../include/Utils.h"

const unsigned int STUDENT_ID = 72;
const unsigned int SEED = 7984;

void PrintDisksTable(const std::vector<DiskInfo>& disks) {
    std::cout << "\n-----------------------------------------------------------------------------------------------------------------\n";
    std::cout << "| Диск | Тип накопителя           | ФС     | Метка тома      | Общий объем    | Свободно       | Заполнено (%) |\n";
    std::cout << "-----------------------------------------------------------------------------------------------------------------\n";

    for (const auto& d : disks) {
        if (!d.isAccessible) {
            printf("| %-4s | %-24s | %-6s | %-15s | %-14s | %-14s | %-13s |\n",
                   d.driveLetter.c_str(), d.driveTypeString.c_str(), "-", "Недоступен", "-", "-", "-");
            continue;
        }
        printf("| %-4s | %-24s | %-6s | %-15s | %-14s | %-14s | %-12.2f%% |\n",
               d.driveLetter.c_str(),
               d.driveTypeString.c_str(),
               d.fileSystemName.c_str(),
               d.volumeName.c_str(),
               Utils::FormatBytes(d.totalBytes).c_str(),
               Utils::FormatBytes(d.freeBytes).c_str(),
               d.usagePercentage);
    }
    std::cout << "-----------------------------------------------------------------------------------------------------------------\n\n";
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    Utils::LogMessage("START", "Запуск анализатора логических дисков (ID=72)", "OK");

    // сканирование дисков
    long long tStart = Utils::GetTicks();
    std::vector<DiskInfo> disks = DiskAnalyzer::GetDisksInfo();
    long long tEnd = Utils::GetTicks();
    double scanTime = Utils::CalculateMs(tStart, tEnd);

    Utils::LogMessage("SCAN_DISKS", "Обнаружено дисков: " + std::to_string(disks.size()), "OK");
    std::cout << "[инфо] время первичного сканирования системы: " << scanTime << " мс\n";

    // отображение таблицы
    PrintDisksTable(disks);

    // экспорт в csv
    std::string csvFile = "disks_report.csv";
    if (DiskAnalyzer::ExportToCsv(disks, csvFile)) {
        Utils::LogMessage("EXPORT_CSV", "Сохранено в " + csvFile, "OK");
    } else {
        Utils::LogMessage("EXPORT_CSV", "Ошибка записи CSV", "ERROR");
    }

    // проведение экспериментальной части (файлы 10, 50, 100 МБ)
    std::cout << "\n================= ЭКСПЕРИМЕНТАЛЬНОЕ ИССЛЕДОВАНИЕ =================\n";
    std::cout << "Исследование динамики дискового пространства при создании тестовых файлов 10, 50, 100 МБ\n";

    // выбираем диск для тестов (диск C:\ или текущий рабочий диск)
    std::string targetDrive = "C:\\";
    for (const auto& d : disks) {
        if (d.isAccessible && d.freeBytes > (1024ULL * 1024ULL * 500ULL)) {
            targetDrive = d.driveLetter;
            break;
        }
    }

    std::cout << "[тестовый диск]: " << targetDrive << "\n";
    DiskInfo baseInfo = DiskAnalyzer::GetSingleDiskInfo(targetDrive);
    std::cout << "[исходное свободное место]: " << Utils::FormatBytes(baseInfo.freeBytes) << "\n\n";

    const unsigned int testSizesMb[] = { 10, 50, 100 };
    std::string dummyFileName = "temp_test_72.bin";

    struct ExpRow {
        unsigned int sizeMb;
        double queryTimes[3];
        double avgQueryTimeMs;
        unsigned long long freeBefore;
        unsigned long long freeAfter;
        long long deltaBytes;
    };
    std::vector<ExpRow> expResults;

    for (unsigned int sizeMb : testSizesMb) {
        ExpRow row{};
        row.sizeMb = sizeMb;

        // замеряем состояние ДО создания файла
        DiskInfo before = DiskAnalyzer::GetSingleDiskInfo(targetDrive);
        row.freeBefore = before.freeBytes;

        // создаем тестовый файл заданного размера
        std::cout << "-> Создание тестового файла размером " << sizeMb << " МБ... ";
        Utils::CreateDummyFile(dummyFileName, sizeMb, SEED);
        std::cout << "готово.\n";

        // проводим 3 серии повторных замеров вызова GetDiskFreeSpaceEx
        double sumTime = 0.0;
        for (int run = 0; run < 3; ++run) {
            long long t1 = Utils::GetTicks();
            DiskInfo current = DiskAnalyzer::GetSingleDiskInfo(targetDrive);
            long long t2 = Utils::GetTicks();

            double ms = Utils::CalculateMs(t1, t2);
            row.queryTimes[run] = ms;
            sumTime += ms;
            Sleep(15);
        }
        row.avgQueryTimeMs = sumTime / 3.0;

        // фиксируем состояние после создания файла
        DiskInfo after = DiskAnalyzer::GetSingleDiskInfo(targetDrive);
        row.freeAfter = after.freeBytes;
        row.deltaBytes = static_cast<long long>(row.freeBefore - row.freeAfter);

        expResults.push_back(row);

        // удаляем файл после проведения замера
        Utils::DeleteDummyFile(dummyFileName);
        Sleep(20);
    }

    // печать итоговой таблицы результатов эксперимента
    std::cout << "\n---------------------------------------------------------------------------------------------------------\n";
    std::cout << "| Тест (МБ) | Запуск 1 (мс) | Запуск 2 (мс) | Запуск 3 (мс) | Среднее (мс) | Изменение памяти (дельта)   |\n";
    std::cout << "---------------------------------------------------------------------------------------------------------\n";
    for (const auto& r : expResults) {
        printf("| %-9u | %-13.4f | %-13.4f | %-13.4f | %-12.4f | %-27s |\n",
               r.sizeMb, r.queryTimes[0], r.queryTimes[1], r.queryTimes[2],
               r.avgQueryTimeMs, Utils::FormatBytes(r.deltaBytes).c_str());
    }
    std::cout << "---------------------------------------------------------------------------------------------------------\n";

    Utils::LogMessage("FINISH", "Лабораторная работа №6 успешно завершена", "OK");
    return 0;
}