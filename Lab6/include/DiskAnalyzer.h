#ifndef DISK_ANALYZER_H
#define DISK_ANALYZER_H

#include <windows.h>
#include <string>
#include <vector>

// структура с полной информацией о логическом диске
struct DiskInfo {
    std::string driveLetter;        // буква диска
    std::string volumeName;         // метка тома
    std::string fileSystemName;     // тип файловой системы
    unsigned int driveType;         // тип диска
    std::string driveTypeString;    // текстовое описание типа
    unsigned long long totalBytes;  // общий объем в байтах
    unsigned long long freeBytes;   // свободный объем в байтах
    unsigned long long usedBytes;   // занятый объем в байтах
    double usagePercentage;         // процент заполнения (от 0.0 до 100.0)
    bool isAccessible;              // доступен ли диск для чтения
};

class DiskAnalyzer {
public:
    // получение списка всех доступных логических дисков в системе
    static std::vector<DiskInfo> GetDisksInfo();

    // получение информации об одном конкретном диске по его пути
    static DiskInfo GetSingleDiskInfo(const std::string& drivePath);

    // экспорт списка дисков в файл csv
    static bool ExportToCsv(const std::vector<DiskInfo>& disks, const std::string& filePath);

private:
    static std::string GetDriveTypeName(unsigned int type);
};

#endif // DISK_ANALYZER_H