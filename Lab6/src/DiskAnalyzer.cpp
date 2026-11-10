#include "../include/DiskAnalyzer.h"
#include "../include/Utils.h"
#include <iostream>
#include <fstream>
#include <iomanip>

std::string DiskAnalyzer::GetDriveTypeName(unsigned int type) {
    switch (type) {
        case DRIVE_FIXED:     return "Фиксированный (HDD/SSD)";
        case DRIVE_REMOVABLE: return "Съемный (USB)";
        case DRIVE_REMOTE:    return "Сетевой диск";
        case DRIVE_CDROM:     return "CD/DVD-ROM";
        case DRIVE_RAMDISK:   return "RAM-диск";
        default:              return "Неизвестный";
    }
}

DiskInfo DiskAnalyzer::GetSingleDiskInfo(const std::string& drivePath) {
    DiskInfo info{};
    info.driveLetter = drivePath;
    info.isAccessible = false;

    // определение типа накопителя
    info.driveType = GetDriveTypeA(drivePath.c_str());
    info.driveTypeString = GetDriveTypeName(info.driveType);

    // получение параметров тома и файловой системы
    char volumeNameBuf[MAX_PATH + 1] = { 0 };
    char fsNameBuf[MAX_PATH + 1] = { 0 };
    DWORD serialNumber = 0;
    DWORD maxComponentLength = 0;
    DWORD fileSystemFlags = 0;

    // отключаем всплывающие окна ошибок windows для пустых приводов
    UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    BOOL volOk = GetVolumeInformationA(
            drivePath.c_str(),
            volumeNameBuf, sizeof(volumeNameBuf),
            &serialNumber,
            &maxComponentLength,
            &fileSystemFlags,
            fsNameBuf, sizeof(fsNameBuf)
    );

    if (volOk) {
        info.volumeName = volumeNameBuf;
        info.fileSystemName = fsNameBuf;
    } else {
        info.volumeName = "Нет метки";
        info.fileSystemName = "Недоступно";
    }

    // получение геометрии дискового пространства через 64-битные счетчики
    ULARGE_INTEGER freeBytesAvailableToCaller{};
    ULARGE_INTEGER totalNumberOfBytes{};
    ULARGE_INTEGER totalNumberOfFreeBytes{};

    BOOL spaceOk = GetDiskFreeSpaceExA(
            drivePath.c_str(),
            &freeBytesAvailableToCaller,
            &totalNumberOfBytes,
            &totalNumberOfFreeBytes
    );

    SetErrorMode(oldErrorMode);

    if (spaceOk && totalNumberOfBytes.QuadPart > 0) {
        info.isAccessible = true;
        info.totalBytes = totalNumberOfBytes.QuadPart;
        info.freeBytes = totalNumberOfFreeBytes.QuadPart;
        info.usedBytes = info.totalBytes - info.freeBytes;
        info.usagePercentage = (static_cast<double>(info.usedBytes) / static_cast<double>(info.totalBytes)) * 100.0;
    } else {
        info.totalBytes = 0;
        info.freeBytes = 0;
        info.usedBytes = 0;
        info.usagePercentage = 0.0;
    }

    return info;
}

std::vector<DiskInfo> DiskAnalyzer::GetDisksInfo() {
    std::vector<DiskInfo> disks;

    // запрашиваем требуемый размер буфера для списка букв дисков
    DWORD bufferLen = GetLogicalDriveStringsA(0, nullptr);
    if (bufferLen == 0) {
        Utils::LogMessage("GET_DRIVES", "GetLogicalDriveStrings failed", "ERROR");
        return disks;
    }

    std::vector<char> driveStrings(bufferLen + 1, 0);
    if (GetLogicalDriveStringsA(bufferLen, driveStrings.data()) == 0) {
        Utils::LogMessage("GET_DRIVES", "Failed to retrieve drive strings", "ERROR");
        return disks;
    }

    // парсим сплошной буфер строк с нулевыми терминаторами ("C:\<null>D:\<null><null>")
    char* currentDrive = driveStrings.data();
    while (*currentDrive) {
        std::string drivePath(currentDrive);
        disks.push_back(GetSingleDiskInfo(drivePath));
        currentDrive += strlen(currentDrive) + 1;
    }

    return disks;
}

bool DiskAnalyzer::ExportToCsv(const std::vector<DiskInfo>& disks, const std::string& filePath) {
    std::ofstream file(filePath, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    // заголовок таблицы csv
    file << "Диск;Тип накопителя;Файловая система;Метка тома;Общий объем (байт);Общий объем;Свободно (байт);Свободно;Занято;Заполнено (%)\n";

    for (const auto& d : disks) {
        if (!d.isAccessible) {
            continue;
        }
        file << d.driveLetter << ";"
             << d.driveTypeString << ";"
             << d.fileSystemName << ";"
             << d.volumeName << ";"
             << d.totalBytes << ";"
             << Utils::FormatBytes(d.totalBytes) << ";"
             << d.freeBytes << ";"
             << Utils::FormatBytes(d.freeBytes) << ";"
             << Utils::FormatBytes(d.usedBytes) << ";"
             << std::fixed << std::setprecision(2) << d.usagePercentage << "%\n";
    }

    file.close();
    return true;
}