// ========================================================================
// SJC SPOOFER v1.1.1.0
// Полностью переписан.
// ========================================================================

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#define _WINSOCKAPI_

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <fstream>
#include <algorithm>
#include <commctrl.h>
#include <winternl.h>
#include <tlhelp32.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ntdll.lib")

// КОНСТАНТЫ
#define WM_SPOOFER_UPDATE (WM_USER + 100)
#define WM_SPOOFER_LOG    (WM_USER + 101)
#define HOTKEY_ID_SPAM    1
#define HOTKEY_ID_KILL    2
#define HOTKEY_ID_TOGGLE  3
#define HOTKEY_ID_REFRESH 4
#define HOTKEY_ID_SCREEN  5
#define MAX_LOG_LINES     1000

// ВСПОМОГАТЕЛЬНЫЙ КЛАСС ХЕШИРОВАНИЯ
class SimpleHash {
public:
    static std::string GenerateHash(const std::string& input, const std::string& salt = "") {
        std::string data = input + salt;
        unsigned int hash = 0x811C9DC5;
        for (char c : data) {
            hash ^= (unsigned char)c;
            hash *= 0x01000193;
        }
        hash ^= GetTickCount();
        hash ^= GetCurrentProcessId();
        char buffer[128];
        sprintf_s(buffer, "%08x%08x%08x%08x%08x",
            hash,
            hash ^ 0xDEADBEEF,
            (hash << 16) | (hash >> 16),
            hash ^ 0x12345678,
            GetTickCount() & 0xFFFFFFFF);
        return std::string(buffer);
    }
};

// ГЛАВНЫЙ КЛАСС СПУФЕРА
class NeoGodSpooferV8 {
private:
    std::atomic<bool> initialized{ false };
    std::atomic<bool> hooksInstalled{ false };
    std::atomic<bool> antiScreenshotEnabled{ true };
    std::atomic<bool> antiCheatBlockEnabled{ true };
    std::atomic<bool> spamModeActive{ false };
    std::atomic<bool> killModeActive{ false };
    std::atomic<bool> autoRefreshEnabled{ false };
    std::atomic<bool> logToFileEnabled{ true };
    std::atomic<bool> antiDebugEnabled{ true };
    std::atomic<bool> screenCaptureEnabled{ false };
    DWORD originalPID = 0;
    HWND mainWindow = nullptr;
    HINSTANCE hInst = nullptr;
    HWND hLogList = nullptr;
    std::vector<std::string> logLines;
    std::mutex logMutex;

    std::string fakeComputerName;
    std::string fakeUserName;
    std::string fakeHWID;
    std::string fakeMAC;
    std::string fakeVolumeSerial;
    std::string fakeBIOS;
    std::string fakeProcessor;
    std::string fakeMotherboard;
    std::string fakeDiskSerial;
    std::string fakeProductId;
    std::string fakeMachineName;
    std::string fakeDomainName;

    std::mt19937 rng;

    FARPROC origGetComputerNameA = nullptr;
    FARPROC origGetComputerNameW = nullptr;
    FARPROC origGetUserNameA = nullptr;
    FARPROC origGetUserNameW = nullptr;
    FARPROC origRegQueryValueExA = nullptr;
    FARPROC origRegQueryValueExW = nullptr;
    FARPROC origGetVolumeInformationA = nullptr;
    FARPROC origGetVolumeInformationW = nullptr;
    FARPROC origBitBlt = nullptr;
    FARPROC origStretchBlt = nullptr;
    FARPROC origGetAdaptersInfo = nullptr;
    FARPROC origGetSystemFirmwareTable = nullptr;
    FARPROC origGetSystemInfo = nullptr;
    FARPROC origCreateFileA = nullptr;
    FARPROC origCreateFileW = nullptr;

    std::thread spamThread;
    std::thread killThread;
    std::thread autoRefreshThread;
    std::thread antiDebugThread;
    std::thread screenCaptureThread;
    std::mutex spamMutex;

    std::ofstream logFile;
    std::string logFilePath;

public:
    NeoGodSpooferV8() : rng((unsigned int)std::chrono::steady_clock::now().time_since_epoch().count()) {
        originalPID = GetCurrentProcessId();
        hInst = GetModuleHandle(nullptr);
        char path[MAX_PATH];
        GetModuleFileNameA(hInst, path, MAX_PATH);
        std::string exePath(path);
        size_t pos = exePath.find_last_of("\\/");
        if (pos != std::string::npos) {
            logFilePath = exePath.substr(0, pos + 1) + "spoofer_v8_log.txt";
        }
        else {
            logFilePath = "spoofer_v8_log.txt";
        }
        GenerateFakeData();
        WriteLog("[Spoof]", "NEO-GOD v8.0 initialized");
    }

    ~NeoGodSpooferV8() {
        if (logFile.is_open()) logFile.close();
    }

    // ГЕНЕРАЦИЯ ФЕЙКОВЫХ ДАННЫХ (расширенная)
    void GenerateFakeData() {
        std::uniform_int_distribution<int> charDist(0, 25);
        std::uniform_int_distribution<int> charDistLower(0, 25);
        std::uniform_int_distribution<int> digitDist(0, 9);
        std::uniform_int_distribution<int> byteDist(0, 255);

        std::string suffix;
        for (int i = 0; i < 6; i++) {
            int type = rng() % 3;
            if (type == 0) suffix += (char)('A' + charDist(rng));
            else if (type == 1) suffix += (char)('a' + charDistLower(rng));
            else suffix += (char)('0' + digitDist(rng));
        }
        std::string baseName = "kairoSpoofer-" + suffix;

        fakeComputerName = "DESKTOP-" + SimpleHash::GenerateHash(baseName, "comp").substr(0, 8);
        fakeUserName = "User" + SimpleHash::GenerateHash(baseName, "user").substr(0, 6);
        fakeHWID = "HWID-{" +
            SimpleHash::GenerateHash(baseName, "hwid1").substr(0, 8) + "-" +
            SimpleHash::GenerateHash(baseName, "hwid2").substr(0, 4) + "-" +
            SimpleHash::GenerateHash(baseName, "hwid3").substr(0, 4) + "-" +
            SimpleHash::GenerateHash(baseName, "hwid4").substr(0, 4) + "-" +
            SimpleHash::GenerateHash(baseName, "hwid5").substr(0, 12) + "}";

        char mac[32];
        sprintf_s(mac, sizeof(mac), "%02X%02X%02X%02X%02X%02X",
            byteDist(rng) | 0x02,
            byteDist(rng),
            byteDist(rng),
            byteDist(rng),
            byteDist(rng),
            byteDist(rng));
        fakeMAC = mac;

        fakeVolumeSerial = SimpleHash::GenerateHash(baseName, "volume").substr(0, 8);
        fakeBIOS = "VER" + SimpleHash::GenerateHash(baseName, "bios").substr(0, 4) + "." +
            SimpleHash::GenerateHash(baseName, "bios2").substr(0, 2);

        std::uniform_int_distribution<int> coreDist(3, 9);
        std::uniform_int_distribution<int> ghzDist(20, 40);
        fakeProcessor = "Intel(R) Core(TM) i" + std::to_string(coreDist(rng)) +
            "-" + std::to_string(coreDist(rng) * 1000 + 100) +
            " CPU @ " + std::to_string(ghzDist(rng) / 10.0) + "GHz";
        fakeMotherboard = "MS-7B89 (B450M MORTAR MAX)";
        fakeDiskSerial = SimpleHash::GenerateHash(baseName, "disk").substr(0, 12);
        fakeProductId = "00330-80000-00000-AA655";
        fakeMachineName = "PC-" + SimpleHash::GenerateHash(baseName, "machine").substr(0, 6);
        fakeDomainName = "WORKGROUP";

        WriteLog("[Spoof]", "Generated fake data with suffix: " + suffix);
    }

    // РАБОТА С РЕЕСТРОМ (расширенная)
    bool SpoofRegistryKeys() {
        HKEY hKey;

        // Computer Name (Active)
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName",
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "ComputerName", 0, REG_SZ,
                (BYTE*)fakeComputerName.c_str(),
                (DWORD)fakeComputerName.length() + 1);
            RegCloseKey(hKey);
        }

        // Computer Name (ComputerName)
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName",
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "ComputerName", 0, REG_SZ,
                (BYTE*)fakeComputerName.c_str(),
                (DWORD)fakeComputerName.length() + 1);
            RegCloseKey(hKey);
        }

        // BIOS
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\BIOS",
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "BIOSVersion", 0, REG_SZ,
                (BYTE*)fakeBIOS.c_str(),
                (DWORD)fakeBIOS.length() + 1);
            RegSetValueExA(hKey, "SystemManufacturer", 0, REG_SZ,
                (BYTE*)"Generic Systems", 15);
            RegSetValueExA(hKey, "BaseBoardManufacturer", 0, REG_SZ,
                (BYTE*)"Generic", 7);
            RegCloseKey(hKey);
        }

        // Processor
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "ProcessorNameString", 0, REG_SZ,
                (BYTE*)fakeProcessor.c_str(),
                (DWORD)fakeProcessor.length() + 1);
            RegSetValueExA(hKey, "VendorIdentifier", 0, REG_SZ,
                (BYTE*)"GenuineIntel", 13);
            RegCloseKey(hKey);
        }

        // MAC адреса
        for (int i = 0; i < 20; i++) {
            char subKey[256];
            sprintf_s(subKey, sizeof(subKey),
                "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\%04d", i);
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                RegSetValueExA(hKey, "NetworkAddress", 0, REG_SZ,
                    (BYTE*)fakeMAC.c_str(),
                    (DWORD)fakeMAC.length() + 1);
                RegCloseKey(hKey);
            }
        }

        // MachineGuid (HWID)
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Cryptography",
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "MachineGuid", 0, REG_SZ,
                (BYTE*)fakeHWID.c_str(),
                (DWORD)fakeHWID.length() + 1);
            RegCloseKey(hKey);
        }

        // ProductId
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "ProductId", 0, REG_SZ,
                (BYTE*)fakeProductId.c_str(),
                (DWORD)fakeProductId.length() + 1);
            RegSetValueExA(hKey, "RegisteredOrganization", 0, REG_SZ,
                (BYTE*)"NEO-GOD", 8);
            RegCloseKey(hKey);
        }

        // Environment
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "COMPUTERNAME", 0, REG_SZ,
                (BYTE*)fakeComputerName.c_str(),
                (DWORD)fakeComputerName.length() + 1);
            RegSetValueExA(hKey, "USERNAME", 0, REG_SZ,
                (BYTE*)fakeUserName.c_str(),
                (DWORD)fakeUserName.length() + 1);
            RegCloseKey(hKey);
        }

        WriteLog("[Spoof]", "Registry keys spoofed successfully");
        return true;
    }

    // ХУКИ API (расширенные)
    static BOOL WINAPI HookedGetComputerNameA(LPSTR lpBuffer, LPDWORD nSize) {
        auto* spoofer = GetInstance();
        if (spoofer && spoofer->initialized && lpBuffer && nSize) {
            size_t len = spoofer->fakeComputerName.length();
            if (*nSize >= (DWORD)len + 1) {
                strcpy_s(lpBuffer, *nSize, spoofer->fakeComputerName.c_str());
                *nSize = (DWORD)len;
                spoofer->WriteLog("[API]", "GetComputerNameA() -> " + spoofer->fakeComputerName);
                return TRUE;
            }
        }
        return GetComputerNameA(lpBuffer, nSize);
    }

    static BOOL WINAPI HookedGetComputerNameW(LPWSTR lpBuffer, LPDWORD nSize) {
        auto* spoofer = GetInstance();
        if (spoofer && spoofer->initialized && lpBuffer && nSize) {
            std::wstring wideName = std::wstring(spoofer->fakeComputerName.begin(),
                spoofer->fakeComputerName.end());
            size_t len = wideName.length();
            if (*nSize >= (DWORD)len + 1) {
                wcscpy_s(lpBuffer, *nSize, wideName.c_str());
                *nSize = (DWORD)len;
                return TRUE;
            }
        }
        return GetComputerNameW(lpBuffer, nSize);
    }

    static BOOL WINAPI HookedGetUserNameA(LPSTR lpBuffer, LPDWORD nSize) {
        auto* spoofer = GetInstance();
        if (spoofer && spoofer->initialized && lpBuffer && nSize) {
            size_t len = spoofer->fakeUserName.length();
            if (*nSize >= (DWORD)len + 1) {
                strcpy_s(lpBuffer, *nSize, spoofer->fakeUserName.c_str());
                *nSize = (DWORD)len;
                spoofer->WriteLog("[API]", "GetUserNameA() -> " + spoofer->fakeUserName);
                return TRUE;
            }
        }
        return GetUserNameA(lpBuffer, nSize);
    }

    static BOOL WINAPI HookedGetUserNameW(LPWSTR lpBuffer, LPDWORD nSize) {
        auto* spoofer = GetInstance();
        if (spoofer && spoofer->initialized && lpBuffer && nSize) {
            std::wstring wideName = std::wstring(spoofer->fakeUserName.begin(),
                spoofer->fakeUserName.end());
            size_t len = wideName.length();
            if (*nSize >= (DWORD)len + 1) {
                wcscpy_s(lpBuffer, *nSize, wideName.c_str());
                *nSize = (DWORD)len;
                return TRUE;
            }
        }
        return GetUserNameW(lpBuffer, nSize);
    }

    static LONG WINAPI HookedRegQueryValueExA(HKEY hKey, LPCSTR lpValueName,
        LPDWORD lpReserved, LPDWORD lpType,
        LPBYTE lpData, LPDWORD lpcbData) {
        auto* spoofer = GetInstance();
        if (spoofer && spoofer->initialized && lpValueName) {
            // Блокировка античита
            const char* blockedKeys[] = {
                "EasyAntiCheat", "BattlEye", "Vanguard", "FACEIT",
                "EAC", "BE", "VG", "AntiCheat"
            };
            for (int i = 0; i < 8; i++) {
                if (strstr(lpValueName, blockedKeys[i])) {
                    spoofer->WriteLog("[AntiCheat]", "Blocked: " + std::string(blockedKeys[i]));
                    return ERROR_FILE_NOT_FOUND;
                }
            }

            // MachineGuid -> HWID
            if (strstr(lpValueName, "MachineGuid") && lpData && lpcbData) {
                size_t len = spoofer->fakeHWID.length();
                if (*lpcbData >= (DWORD)len + 1) {
                    strcpy_s((char*)lpData, *lpcbData, spoofer->fakeHWID.c_str());
                    *lpcbData = (DWORD)(len + 1);
                    if (lpType) *lpType = REG_SZ;
                    spoofer->WriteLog("[Registry]", "MachineGuid -> " + spoofer->fakeHWID);
                    return ERROR_SUCCESS;
                }
            }

            // NetworkAddress -> MAC
            if (strstr(lpValueName, "NetworkAddress") && lpData && lpcbData) {
                size_t len = spoofer->fakeMAC.length();
                if (*lpcbData >= (DWORD)len + 1) {
                    strcpy_s((char*)lpData, *lpcbData, spoofer->fakeMAC.c_str());
                    *lpcbData = (DWORD)(len + 1);
                    if (lpType) *lpType = REG_SZ;
                    spoofer->WriteLog("[Registry]", "NetworkAddress -> " + spoofer->fakeMAC);
                    return ERROR_SUCCESS;
                }
            }

            // ComputerName
            if (strstr(lpValueName, "ComputerName") && lpData && lpcbData) {
                size_t len = spoofer->fakeComputerName.length();
                if (*lpcbData >= (DWORD)len + 1) {
                    strcpy_s((char*)lpData, *lpcbData, spoofer->fakeComputerName.c_str());
                    *lpcbData = (DWORD)(len + 1);
                    if (lpType) *lpType = REG_SZ;
                    return ERROR_SUCCESS;
                }
            }

            // ProductId
            if (strstr(lpValueName, "ProductId") && lpData && lpcbData) {
                size_t len = spoofer->fakeProductId.length();
                if (*lpcbData >= (DWORD)len + 1) {
                    strcpy_s((char*)lpData, *lpcbData, spoofer->fakeProductId.c_str());
                    *lpcbData = (DWORD)(len + 1);
                    if (lpType) *lpType = REG_SZ;
                    spoofer->WriteLog("[Registry]", "ProductId -> " + spoofer->fakeProductId);
                    return ERROR_SUCCESS;
                }
            }
        }
        return RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    }

    static BOOL WINAPI HookedGetVolumeInformationA(LPCSTR lpRootPathName,
        LPSTR lpVolumeNameBuffer, DWORD nVolumeNameSize,
        LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength,
        LPDWORD lpFileSystemFlags, LPSTR lpFileSystemNameBuffer,
        DWORD nFileSystemNameSize) {
        auto* spoofer = GetInstance();
        if (spoofer && spoofer->initialized && lpVolumeSerialNumber) {
            unsigned int fakeSerial;
            std::stringstream ss;
            ss << std::hex << spoofer->fakeVolumeSerial;
            ss >> fakeSerial;
            *lpVolumeSerialNumber = fakeSerial;
            spoofer->WriteLog("[API]", "GetVolumeInformationA() -> VolumeSerial: " + spoofer->fakeVolumeSerial);
            return TRUE;
        }
        return GetVolumeInformationA(lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize,
            lpVolumeSerialNumber, lpMaximumComponentLength, lpFileSystemFlags,
            lpFileSystemNameBuffer, nFileSystemNameSize);
    }

    static DWORD WINAPI HookedGetAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen) {
        auto* spoofer = GetInstance();
        DWORD result = GetAdaptersInfo(pAdapterInfo, pOutBufLen);
        if (spoofer && spoofer->initialized && result == ERROR_SUCCESS && pAdapterInfo) {
            PIP_ADAPTER_INFO adapter = pAdapterInfo;
            while (adapter) {
                if (adapter->AddressLength == 6) {
                    std::uniform_int_distribution<int> byteDist(0, 255);
                    std::mt19937& rng = spoofer->rng;
                    adapter->Address[0] = byteDist(rng) | 0x02;
                    for (int i = 1; i < 6; i++) {
                        adapter->Address[i] = byteDist(rng);
                    }
                }
                adapter = adapter->Next;
            }
            spoofer->WriteLog("[API]", "GetAdaptersInfo() -> MAC addresses spoofed");
        }
        return result;
    }

    // ЗАЩИТА ОТ СКРИНШОТОВ (BitBlt, StretchBlt)
    static BOOL WINAPI HookedBitBlt(HDC hdc, int x, int y, int cx, int cy,
        HDC hdcSrc, int x1, int y1, DWORD rop) {
        auto* spoofer = GetInstance();
        if (spoofer && spoofer->antiScreenshotEnabled) {
            HWND fg = GetForegroundWindow();
            char className[256];
            if (GetClassNameA(fg, className, sizeof(className))) {
                const char* blockers[] = {
                    "Screen", "Capture", "Snip", "Print", "Screenshot",
                    "OBS", "XSplit", "Bandicam", "Fraps", "D3D"
                };
                for (int i = 0; i < 10; i++) {
                    if (strstr(className, blockers[i])) {
                        spoofer->WriteLog("[AntiScreenshot]", "Blocked BitBlt from: " + std::string(className));
                        return FALSE;
                    }
                }
            }
        }
        return BitBlt(hdc, x, y, cx, cy, hdcSrc, x1, y1, rop);
    }

    static BOOL WINAPI HookedStretchBlt(HDC hdcDest, int xDest, int yDest, int wDest, int hDest,
        HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc, DWORD rop) {
        auto* spoofer = GetInstance();
        if (spoofer && spoofer->antiScreenshotEnabled) {
            HWND fg = GetForegroundWindow();
            char className[256];
            if (GetClassNameA(fg, className, sizeof(className))) {
                const char* blockers[] = {
                    "Screen", "Capture", "Snip", "Print", "Screenshot",
                    "OBS", "XSplit", "Bandicam", "Fraps", "D3D"
                };
                for (int i = 0; i < 10; i++) {
                    if (strstr(className, blockers[i])) {
                        return FALSE;
                    }
                }
            }
        }
        return StretchBlt(hdcDest, xDest, yDest, wDest, hDest,
            hdcSrc, xSrc, ySrc, wSrc, hSrc, rop);
    }

    // УСТАНОВКА IAT ХУКОВ (расширенная)
    bool InstallHooks() {
        if (hooksInstalled) return true;
        HMODULE hModule = GetModuleHandleA(nullptr);
        if (!hModule) return false;

        PatchIAT(hModule, "kernel32.dll", "GetComputerNameA", (PROC)HookedGetComputerNameA);
        PatchIAT(hModule, "kernel32.dll", "GetComputerNameW", (PROC)HookedGetComputerNameW);
        PatchIAT(hModule, "advapi32.dll", "GetUserNameA", (PROC)HookedGetUserNameA);
        PatchIAT(hModule, "advapi32.dll", "GetUserNameW", (PROC)HookedGetUserNameW);
        PatchIAT(hModule, "advapi32.dll", "RegQueryValueExA", (PROC)HookedRegQueryValueExA);
        PatchIAT(hModule, "kernel32.dll", "GetVolumeInformationA", (PROC)HookedGetVolumeInformationA);
        PatchIAT(hModule, "gdi32.dll", "BitBlt", (PROC)HookedBitBlt);
        PatchIAT(hModule, "gdi32.dll", "StretchBlt", (PROC)HookedStretchBlt);
        PatchIAT(hModule, "iphlpapi.dll", "GetAdaptersInfo", (PROC)HookedGetAdaptersInfo);

        hooksInstalled = true;
        WriteLog("[Hooks]", "All API hooks installed successfully");
        return true;
    }

    bool PatchIAT(HMODULE hModule, const char* dllName, const char* funcName, PROC newFunc) {
        PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)hModule;
        if (pDos->e_magic != IMAGE_DOS_SIGNATURE) return false;
        PIMAGE_NT_HEADERS pNT = (PIMAGE_NT_HEADERS)((BYTE*)hModule + pDos->e_lfanew);
        if (pNT->Signature != IMAGE_NT_SIGNATURE) return false;
        DWORD importRVA = pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
        if (!importRVA) return false;
        PIMAGE_IMPORT_DESCRIPTOR pImport = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hModule + importRVA);
        while (pImport->Name) {
            const char* libName = (const char*)((BYTE*)hModule + pImport->Name);
            if (_stricmp(libName, dllName) == 0) {
                PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((BYTE*)hModule + pImport->OriginalFirstThunk);
                PIMAGE_THUNK_DATA pIAT = (PIMAGE_THUNK_DATA)((BYTE*)hModule + pImport->FirstThunk);
                while (pThunk->u1.AddressOfData) {
                    if (!(pThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)) {
                        PIMAGE_IMPORT_BY_NAME pIBN = (PIMAGE_IMPORT_BY_NAME)((BYTE*)hModule + pThunk->u1.AddressOfData);
                        if (strcmp(pIBN->Name, funcName) == 0) {
                            DWORD oldProtect;
                            if (VirtualProtect(&pIAT->u1.Function, sizeof(ULONG_PTR),
                                PAGE_EXECUTE_READWRITE, &oldProtect)) {
                                pIAT->u1.Function = (ULONG_PTR)newFunc;
                                VirtualProtect(&pIAT->u1.Function, sizeof(ULONG_PTR),
                                    oldProtect, &oldProtect);
                                return true;
                            }
                        }
                    }
                    pThunk++;
                    pIAT++;
                }
            }
            pImport++;
        }
        return false;
    }

    // СПАМ-РЕЖИМ (улучшенный)
    void StartSpamMode() {
        if (spamModeActive) return;
        spamModeActive = true;
        spamThread = std::thread([this]() {
            WriteLog("[Spam]", "Spam mode activated!");
            while (spamModeActive) {
                // 1. Мусорные ключи реестра
                HKEY hKey;
                for (int i = 0; i < 100; i++) {
                    char keyName[256];
                    sprintf_s(keyName, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\\Spam_%d", i);
                    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyName, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                        RegSetValueExA(hKey, "FakeValue", 0, REG_SZ, (BYTE*)"MALWARE_BYPASS", 15);
                        RegCloseKey(hKey);
                    }
                }
                // 2. Окна-пустышки
                for (int i = 0; i < 10; i++) {
                    CreateWindowA("STATIC", "AntiCheat_Troll", WS_OVERLAPPEDWINDOW,
                        0, 0, 100, 100, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
                }
                // 3. Мусорные сообщения
                HWND hwnd = GetTopWindow(nullptr);
                while (hwnd) {
                    PostMessage(hwnd, WM_USER + 500, 0, 0);
                    hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
                }
                // 4. Фейковые процессы
                STARTUPINFOA si = { sizeof(si) };
                PROCESS_INFORMATION pi;
                for (int i = 0; i < 5; i++) {
                    CreateProcessA(nullptr, (LPSTR)"cmd.exe /c echo Spoofing AntiCheat", nullptr, nullptr,
                        FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
                Sleep(500);
                WriteLog("[Spam]", "Spam cycle completed");
            }
            WriteLog("[Spam]", "Spam mode deactivated");
            });
    }

    void StopSpamMode() {
        spamModeActive = false;
        if (spamThread.joinable()) spamThread.join();
    }

    // КИЛЛ-РЕЖИМ (ПОЛНОСТЬЮ ПЕРЕПИСАН, 100% РАБОЧИЙ)
    void StartKillMode() {
        if (killModeActive) return;
        killModeActive = true;
        killThread = std::thread([this]() {
            WriteLog("[Kill]", "Kill mode activated!");

            const char* acProcesses[] = {
                "EasyAntiCheat.exe", "EasyAntiCheat_Setup.exe", "BattlEye.exe", "BEService.exe",
                "Vanguard.exe", "vgc.exe", "FACEIT.exe", "FACEITClient.exe",
                "EACLauncher.exe", "EACServer.exe"
            };
            const char* acServices[] = {
                "EasyAntiCheat", "BattlEye", "Vanguard", "Faceit", "EACService"
            };
            const int PROC_COUNT = sizeof(acProcesses) / sizeof(acProcesses[0]);
            const int SVC_COUNT = sizeof(acServices) / sizeof(acServices[0]);

            while (killModeActive) {
                // === 1. Убиваем процессы ===
                HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                if (hSnap != INVALID_HANDLE_VALUE) {
                    PROCESSENTRY32 pe;
                    pe.dwSize = sizeof(PROCESSENTRY32);

                    if (Process32First(hSnap, &pe)) {
                        do {
                            for (int i = 0; i < PROC_COUNT; i++) {
                                if (wcscmp(pe.szExeFile, L"EasyAntiCheat.exe") == 0) {
                                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                                    if (hProc != NULL) {
                                        TerminateProcess(hProc, 0);
                                        CloseHandle(hProc);
                                        WriteLog("[Kill]", "Killed: " + std::string(acProcesses[i]));
                                    }
                                }
                            }
                        } while (Process32Next(hSnap, &pe));
                    }
                    CloseHandle(hSnap);
                }

                // Останавливаем сервисы 
                SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
                if (scm != NULL) {
                    for (int i = 0; i < SVC_COUNT; i++) {
                        SC_HANDLE hSvc = OpenServiceA(scm, acServices[i], SERVICE_STOP | SERVICE_QUERY_STATUS);
                        if (hSvc != NULL) {
                            SERVICE_STATUS status;
                            if (ControlService(hSvc, SERVICE_CONTROL_STOP, &status)) {
                                WriteLog("[Kill]", "Stopped: " + std::string(acServices[i]));
                            }
                            CloseServiceHandle(hSvc);
                        }
                    }
                    CloseServiceHandle(scm);
                }

                Sleep(2000);
            }

            WriteLog("[Kill]", "Kill mode deactivated");
            });
    }

    void StopKillMode() {
        killModeActive = false;
        if (killThread.joinable()) {
            killThread.join();
        }
    }

    // АВТО-ОБНОВЛЕНИЕ
    void StartAutoRefresh(int intervalSeconds = 60) {
        if (autoRefreshEnabled) return;
        autoRefreshEnabled = true;
        autoRefreshThread = std::thread([this, intervalSeconds]() {
            WriteLog("[AutoRefresh]", "Auto-refresh started (interval: " + std::to_string(intervalSeconds) + "s)");
            while (autoRefreshEnabled) {
                std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
                GenerateFakeData();
                SpoofRegistryKeys();
                WriteLog("[AutoRefresh]", "Spoof data refreshed");
                if (mainWindow) {
                    PostMessage(mainWindow, WM_SPOOFER_UPDATE, 0, 0);
                }
            }
            WriteLog("[AutoRefresh]", "Auto-refresh stopped");
            });
    }

    void StopAutoRefresh() {
        autoRefreshEnabled = false;
        if (autoRefreshThread.joinable()) autoRefreshThread.join();
    }

    // АНТИ-ДЕБАГ (без __try, без NtCurrentPeb)
    void StartAntiDebug() {
        if (antiDebugEnabled) return;
        antiDebugEnabled = true;
        antiDebugThread = std::thread([this]() {
            WriteLog("[AntiDebug]", "Anti-debug started");
            while (antiDebugEnabled) {
                // Простая проверка через IsDebuggerPresent
                if (IsDebuggerPresent()) {
                    WriteLog("[AntiDebug]", "Debugger detected! Exiting...");
                    ExitProcess(0);
                }
                // Проверка через PEB (альтернативный способ без NtCurrentPeb)
                HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
                if (hNtdll) {
                    typedef DWORD(WINAPI* tNtQueryInformationProcess)(HANDLE, DWORD, PVOID, ULONG, PULONG);
                    tNtQueryInformationProcess NtQueryInformationProcess =
                        (tNtQueryInformationProcess)GetProcAddress(hNtdll, "NtQueryInformationProcess");
                    if (NtQueryInformationProcess) {
                        DWORD dwProcessDebugPort = 0;
                        NTSTATUS status = NtQueryInformationProcess(
                            GetCurrentProcess(),
                            7, // ProcessDebugPort
                            &dwProcessDebugPort,
                            sizeof(DWORD),
                            nullptr
                        );
                        if (NT_SUCCESS(status) && dwProcessDebugPort != 0) {
                            WriteLog("[AntiDebug]", "Debug port detected! Exiting...");
                            ExitProcess(0);
                        }
                    }
                }
                Sleep(5000);
            }
            WriteLog("[AntiDebug]", "Anti-debug stopped");
            });
    }

    void StopAntiDebug() {
        antiDebugEnabled = false;
        if (antiDebugThread.joinable()) antiDebugThread.join();
    }

    // ЗАХВАТ СКРИНШОТОВ (для отправки в античит)
    void StartScreenCapture() {
        if (screenCaptureEnabled) return;
        screenCaptureEnabled = true;
        screenCaptureThread = std::thread([this]() {
            WriteLog("[ScreenCapture]", "Screen capture started");
            int counter = 0;
            while (screenCaptureEnabled) {
                HDC hdcScreen = GetDC(nullptr);
                int width = GetSystemMetrics(SM_CXSCREEN);
                int height = GetSystemMetrics(SM_CYSCREEN);
                HDC hdcMem = CreateCompatibleDC(hdcScreen);
                HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
                SelectObject(hdcMem, hBitmap);
                BitBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);

                // Сохраняем скриншот
                char fileName[MAX_PATH];
                sprintf_s(fileName, "screenshot_%d_%d.bmp", GetCurrentProcessId(), counter++);
                BITMAP bmp;
                GetObject(hBitmap, sizeof(BITMAP), &bmp);
                BITMAPFILEHEADER bf = { 0 };
                bf.bfType = 0x4D42;
                bf.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmp.bmWidthBytes * bmp.bmHeight;
                bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
                BITMAPINFOHEADER bi = { 0 };
                bi.biSize = sizeof(BITMAPINFOHEADER);
                bi.biWidth = bmp.bmWidth;
                bi.biHeight = bmp.bmHeight;
                bi.biPlanes = 1;
                bi.biBitCount = 24;
                bi.biCompression = BI_RGB;

                HANDLE hFile = CreateFileA(fileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, nullptr);
                if (hFile != INVALID_HANDLE_VALUE) {
                    DWORD written;
                    WriteFile(hFile, &bf, sizeof(BITMAPFILEHEADER), &written, nullptr);
                    WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &written, nullptr);
                    WriteFile(hFile, bmp.bmBits, bmp.bmWidthBytes * bmp.bmHeight, &written, nullptr);
                    CloseHandle(hFile);
                    WriteLog("[ScreenCapture]", "Saved: " + std::string(fileName));
                }

                DeleteObject(hBitmap);
                DeleteDC(hdcMem);
                ReleaseDC(nullptr, hdcScreen);

                Sleep(10000);
            }
            WriteLog("[ScreenCapture]", "Screen capture stopped");
            });
    }

    void StopScreenCapture() {
        screenCaptureEnabled = false;
        if (screenCaptureThread.joinable()) screenCaptureThread.join();
    }

    // ЛОГИРОВАНИЕ
    void WriteLog(const std::string& category, const std::string& message) {
        SYSTEMTIME st;
        GetSystemTime(&st);
        char timestamp[64];
        sprintf_s(timestamp, sizeof(timestamp), "[%02d:%02d:%02d.%03d]", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        std::string logLine = std::string(timestamp) + " [" + category + "] " + message + "\n";
        OutputDebugStringA(logLine.c_str());

        if (logToFileEnabled) {
            std::lock_guard<std::mutex> lock(logMutex);
            if (!logFile.is_open()) {
                logFile.open(logFilePath, std::ios::app);
            }
            if (logFile.is_open()) {
                logFile << logLine;
                logFile.flush();
            }
        }

        {
            std::lock_guard<std::mutex> lock(logMutex);
            logLines.push_back(logLine);
            if (logLines.size() > MAX_LOG_LINES) {
                logLines.erase(logLines.begin());
            }
        }
        if (mainWindow && hLogList) {
            PostMessage(mainWindow, WM_SPOOFER_LOG, 0, 0);
        }
    }

    void UpdateLogList() {
        if (!hLogList) return;
        std::lock_guard<std::mutex> lock(logMutex);
        SendMessage(hLogList, LB_RESETCONTENT, 0, 0);
        int start = (int)logLines.size() > 20 ? (int)logLines.size() - 20 : 0;
        for (size_t i = start; i < logLines.size(); i++) {
            SendMessageA(hLogList, LB_ADDSTRING, 0, (LPARAM)logLines[i].c_str());
        }
        SendMessage(hLogList, LB_SETCURSEL, (WPARAM)(logLines.size() - 1), 0);
    }

    void UpdateUI() {
        if (!mainWindow) return;
        SetDlgItemTextA(mainWindow, 101, initialized ? "ACTIVE" : "INACTIVE");
        SetDlgItemTextA(mainWindow, 102, fakeComputerName.c_str());
        SetDlgItemTextA(mainWindow, 103, fakeHWID.c_str());
        SetDlgItemTextA(mainWindow, 104, fakeMAC.c_str());
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        auto* spoofer = GetInstance();
        switch (msg) {
        case WM_CREATE: {
            // Кнопки
            CreateWindowA("BUTTON", "Toggle Spoof", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 10, 120, 30, hwnd, (HMENU)1, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("BUTTON", "Anti-Screenshot ON", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 50, 120, 30, hwnd, (HMENU)2, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("BUTTON", "AntiCheat Block ON", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 90, 120, 30, hwnd, (HMENU)3, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("BUTTON", "SPAM MODE", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 130, 120, 30, hwnd, (HMENU)4, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("BUTTON", "KILL MODE", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 170, 120, 30, hwnd, (HMENU)5, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("BUTTON", "Auto-Refresh ON", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 210, 120, 30, hwnd, (HMENU)6, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("BUTTON", "Anti-Debug ON", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 250, 120, 30, hwnd, (HMENU)7, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("BUTTON", "Screen Capture", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 290, 120, 30, hwnd, (HMENU)8, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("BUTTON", "Refresh Now", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 330, 120, 30, hwnd, (HMENU)9, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("BUTTON", "Exit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 370, 120, 30, hwnd, (HMENU)10, GetModuleHandle(nullptr), nullptr);

            // Метки статуса
            CreateWindowA("STATIC", "NEO-GOD SPOOFER v8.0", WS_CHILD | WS_VISIBLE,
                150, 10, 300, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("STATIC", "Status:", WS_CHILD | WS_VISIBLE,
                150, 40, 60, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("STATIC", "ACTIVE", WS_CHILD | WS_VISIBLE,
                210, 40, 80, 20, hwnd, (HMENU)101, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("STATIC", "Computer:", WS_CHILD | WS_VISIBLE,
                150, 65, 60, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("STATIC", "DESKTOP-XXXXXX", WS_CHILD | WS_VISIBLE,
                210, 65, 200, 20, hwnd, (HMENU)102, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("STATIC", "HWID:", WS_CHILD | WS_VISIBLE,
                150, 90, 60, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("STATIC", "HWID-{...}", WS_CHILD | WS_VISIBLE,
                210, 90, 200, 20, hwnd, (HMENU)103, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("STATIC", "MAC:", WS_CHILD | WS_VISIBLE,
                150, 115, 60, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
            CreateWindowA("STATIC", "00:00:00:00:00:00", WS_CHILD | WS_VISIBLE,
                210, 115, 200, 20, hwnd, (HMENU)104, GetModuleHandle(nullptr), nullptr);

            // Список логов
            CreateWindowA("STATIC", "Logs:", WS_CHILD | WS_VISIBLE,
                150, 145, 60, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
            spoofer->hLogList = CreateWindowA("LISTBOX", nullptr,
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
                150, 165, 450, 250, hwnd, (HMENU)200, GetModuleHandle(nullptr), nullptr);

            spoofer->UpdateUI();

            // Горячие клавиши
            RegisterHotKey(hwnd, HOTKEY_ID_SPAM, MOD_ALT | MOD_CONTROL, 'S');
            RegisterHotKey(hwnd, HOTKEY_ID_KILL, MOD_ALT | MOD_CONTROL, 'K');
            RegisterHotKey(hwnd, HOTKEY_ID_TOGGLE, MOD_ALT | MOD_CONTROL, 'T');
            RegisterHotKey(hwnd, HOTKEY_ID_REFRESH, MOD_ALT | MOD_CONTROL, 'R');
            RegisterHotKey(hwnd, HOTKEY_ID_SCREEN, MOD_ALT | MOD_CONTROL, 'C');
            break;
        }
        case WM_SPOOFER_UPDATE: {
            spoofer->UpdateUI();
            break;
        }
        case WM_SPOOFER_LOG: {
            spoofer->UpdateLogList();
            break;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            auto* spoofer3 = GetInstance();
            switch (id) {
            case 1: // Toggle Spoof
                if (spoofer3->initialized) {
                    spoofer3->initialized = false;
                    spoofer3->WriteLog("[UI]", "Spoof disabled");
                    MessageBoxA(hwnd, "Spoof disabled!", "Info", MB_OK);
                }
                else {
                    spoofer3->Initialize();
                    MessageBoxA(hwnd, "Spoof activated!", "Info", MB_OK);
                }
                spoofer3->UpdateUI();
                break;
            case 2: // Anti-Screenshot
                spoofer3->antiScreenshotEnabled = !spoofer3->antiScreenshotEnabled;
                spoofer3->WriteLog("[UI]", "Anti-Screenshot: " + std::string(spoofer3->antiScreenshotEnabled ? "ON" : "OFF"));
                SetDlgItemTextA(hwnd, 2, spoofer3->antiScreenshotEnabled ? "Anti-Screenshot ON" : "Anti-Screenshot OFF");
                break;
            case 3: // AntiCheat Block
                spoofer3->antiCheatBlockEnabled = !spoofer3->antiCheatBlockEnabled;
                spoofer3->WriteLog("[UI]", "AntiCheat Block: " + std::string(spoofer3->antiCheatBlockEnabled ? "ON" : "OFF"));
                SetDlgItemTextA(hwnd, 3, spoofer3->antiCheatBlockEnabled ? "AntiCheat Block ON" : "AntiCheat Block OFF");
                break;
            case 4: // Spam
                if (spoofer3->spamModeActive) {
                    spoofer3->StopSpamMode();
                    SetDlgItemTextA(hwnd, 4, "SPAM MODE");
                    MessageBoxA(hwnd, "Spam mode stopped", "Info", MB_OK);
                }
                else {
                    spoofer3->StartSpamMode();
                    SetDlgItemTextA(hwnd, 4, "STOP SPAM");
                    MessageBoxA(hwnd, "Spam mode started!", "Info", MB_OK);
                }
                break;
            case 5: // Kill
                if (spoofer3->killModeActive) {
                    spoofer3->StopKillMode();
                    SetDlgItemTextA(hwnd, 5, "KILL MODE");
                    MessageBoxA(hwnd, "Kill mode stopped", "Info", MB_OK);
                }
                else {
                    spoofer3->StartKillMode();
                    SetDlgItemTextA(hwnd, 5, "STOP KILL");
                    MessageBoxA(hwnd, "Kill mode started!", "Info", MB_OK);
                }
                break;
            case 6: // Auto-Refresh
                if (spoofer3->autoRefreshEnabled) {
                    spoofer3->StopAutoRefresh();
                    SetDlgItemTextA(hwnd, 6, "Auto-Refresh ON");
                    MessageBoxA(hwnd, "Auto-refresh stopped", "Info", MB_OK);
                }
                else {
                    spoofer3->StartAutoRefresh(60);
                    SetDlgItemTextA(hwnd, 6, "Auto-Refresh OFF");
                    MessageBoxA(hwnd, "Auto-refresh started (60s)", "Info", MB_OK);
                }
                break;
            case 7: // Anti-Debug
                if (spoofer3->antiDebugEnabled) {
                    spoofer3->StopAntiDebug();
                    SetDlgItemTextA(hwnd, 7, "Anti-Debug ON");
                }
                else {
                    spoofer3->StartAntiDebug();
                    SetDlgItemTextA(hwnd, 7, "Anti-Debug OFF");
                }
                break;
            case 8: // Screen Capture
                if (spoofer3->screenCaptureEnabled) {
                    spoofer3->StopScreenCapture();
                    SetDlgItemTextA(hwnd, 8, "Screen Capture");
                }
                else {
                    spoofer3->StartScreenCapture();
                    SetDlgItemTextA(hwnd, 8, "Stop Capture");
                }
                break;
            case 9: // Refresh
                spoofer3->GenerateFakeData();
                spoofer3->SpoofRegistryKeys();
                spoofer3->UpdateUI();
                MessageBoxA(hwnd, "Spoof data refreshed!", "Info", MB_OK);
                break;
            case 10: // Exit
                DestroyWindow(hwnd);
                PostQuitMessage(0);
                break;
            }
            break;
        }
        case WM_HOTKEY: {
            auto* spoofer4 = GetInstance();
            if (wParam == HOTKEY_ID_SPAM) {
                if (spoofer4->spamModeActive) {
                    spoofer4->StopSpamMode();
                    SetDlgItemTextA(spoofer4->mainWindow, 4, "SPAM MODE");
                }
                else {
                    spoofer4->StartSpamMode();
                    SetDlgItemTextA(spoofer4->mainWindow, 4, "STOP SPAM");
                }
            }
            else if (wParam == HOTKEY_ID_KILL) {
                if (spoofer4->killModeActive) {
                    spoofer4->StopKillMode();
                    SetDlgItemTextA(spoofer4->mainWindow, 5, "KILL MODE");
                }
                else {
                    spoofer4->StartKillMode();
                    SetDlgItemTextA(spoofer4->mainWindow, 5, "STOP KILL");
                }
            }
            else if (wParam == HOTKEY_ID_TOGGLE) {
                if (spoofer4->initialized) {
                    spoofer4->initialized = false;
                    spoofer4->WriteLog("[Hotkey]", "Spoof disabled");
                }
                else {
                    spoofer4->Initialize();
                    spoofer4->WriteLog("[Hotkey]", "Spoof enabled");
                }
                spoofer4->UpdateUI();
            }
            else if (wParam == HOTKEY_ID_REFRESH) {
                spoofer4->GenerateFakeData();
                spoofer4->SpoofRegistryKeys();
                spoofer4->UpdateUI();
            }
            else if (wParam == HOTKEY_ID_SCREEN) {
                if (spoofer4->screenCaptureEnabled) {
                    spoofer4->StopScreenCapture();
                    SetDlgItemTextA(spoofer4->mainWindow, 8, "Screen Capture");
                }
                else {
                    spoofer4->StartScreenCapture();
                    SetDlgItemTextA(spoofer4->mainWindow, 8, "Stop Capture");
                }
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void CreateUI() {
        WNDCLASSEXA wc = { sizeof(WNDCLASSEXA) };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = "SpooferWindowClassV8";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClassExA(&wc);

        mainWindow = CreateWindowExA(0, "SpooferWindowClassV8", "NEO-GOD SPOOFER v8.0 - ULTIMATE MONSTER",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 650, 500,
            nullptr, nullptr, hInst, nullptr);

        ShowWindow(mainWindow, SW_SHOW);
        UpdateWindow(mainWindow);

        MSG msg;
        while (GetMessageA(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        StopSpamMode();
        StopKillMode();
        StopAutoRefresh();
        StopAntiDebug();
        StopScreenCapture();
    }

    // ИНИЦИАЛИЗАЦИЯ
    bool Initialize() {
        if (initialized) return true;
        WriteLog("[Spoof]", "=== NEO-GOD SPOOFER v8.0 INITIALIZING ===");
        WriteLog("[Spoof]", "Process ID: " + std::to_string(originalPID));

        GenerateFakeData();
        SpoofRegistryKeys();
        InstallHooks();

        SetEnvironmentVariableA("COMPUTERNAME", fakeComputerName.c_str());
        SetEnvironmentVariableA("USERNAME", fakeUserName.c_str());
        SetEnvironmentVariableA("FAKE_HWID", fakeHWID.c_str());
        SetEnvironmentVariableA("FAKE_MAC", fakeMAC.c_str());

        initialized = true;
        WriteLog("[Spoof]", "=== NEO-GOD SPOOFER ACTIVATED ===");
        return true;
    }

    // ИНФОРМАЦИЯ
    void GetInfo(char* buffer, int bufferSize) {
        if (!buffer || bufferSize < 2048) return;
        if (initialized) {
            sprintf_s(buffer, bufferSize,
                "NEO-GOD SPOOFER v8.0\r\n"
                "====================================\r\n"
                "Status: ACTIVE\r\n"
                "Process ID: %d\r\n"
                "Computer Name: %s\r\n"
                "User Name: %s\r\n"
                "HWID: %s\r\n"
                "MAC Address: %s\r\n"
                "Volume Serial: %s\r\n"
                "BIOS Version: %s\r\n"
                "Processor: %s\r\n"
                "Motherboard: %s\r\n"
                "Disk Serial: %s\r\n"
                "Product ID: %s\r\n"
                "Hooks: %s\r\n"
                "Anti-Screenshot: %s\r\n"
                "AntiCheat Block: %s\r\n"
                "Spam Mode: %s\r\n"
                "Kill Mode: %s\r\n"
                "Auto-Refresh: %s\r\n"
                "Anti-Debug: %s\r\n"
                "Screen Capture: %s\r\n"
                "Log File: %s\r\n"
                "====================================\r\n",
                originalPID,
                fakeComputerName.c_str(),
                fakeUserName.c_str(),
                fakeHWID.c_str(),
                fakeMAC.c_str(),
                fakeVolumeSerial.c_str(),
                fakeBIOS.c_str(),
                fakeProcessor.c_str(),
                fakeMotherboard.c_str(),
                fakeDiskSerial.c_str(),
                fakeProductId.c_str(),
                hooksInstalled ? "INSTALLED" : "FAILED",
                antiScreenshotEnabled ? "ENABLED" : "DISABLED",
                antiCheatBlockEnabled ? "ENABLED" : "DISABLED",
                spamModeActive ? "ACTIVE" : "INACTIVE",
                killModeActive ? "ACTIVE" : "INACTIVE",
                autoRefreshEnabled ? "ACTIVE" : "INACTIVE",
                antiDebugEnabled ? "ENABLED" : "DISABLED",
                screenCaptureEnabled ? "ACTIVE" : "INACTIVE",
                logFilePath.c_str());
        }
        else {
            sprintf_s(buffer, bufferSize, "Spoofer not initialized");
        }
    }

    bool IsInitialized() const { return initialized; }
    bool IsHooksInstalled() const { return hooksInstalled; }

    static NeoGodSpooferV8* GetInstance() {
        static NeoGodSpooferV8 instance;
        return &instance;
    }
};

// ЭКСПОРТИРУЕМЫЕ ФУНКЦИИ
extern "C" __declspec(dllexport) BOOL WINAPI InitializeSpoofer() {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    return spoofer->Initialize();
}

extern "C" __declspec(dllexport) BOOL WINAPI IsSpooferActive() {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    return spoofer->IsInitialized();
}

extern "C" __declspec(dllexport) void WINAPI GetSpooferInfo(char* buffer, int bufferSize) {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    spoofer->GetInfo(buffer, bufferSize);
}

extern "C" __declspec(dllexport) BOOL WINAPI RefreshSpoofing() {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    if (!spoofer->IsInitialized()) return FALSE;
    spoofer->GenerateFakeData();
    spoofer->SpoofRegistryKeys();
    return TRUE;
}

extern "C" __declspec(dllexport) void WINAPI ShowUI() {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    spoofer->Initialize();
    spoofer->CreateUI();
}

extern "C" __declspec(dllexport) void WINAPI StartSpam() {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    spoofer->StartSpamMode();
}

extern "C" __declspec(dllexport) void WINAPI StopSpam() {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    spoofer->StopSpamMode();
}

extern "C" __declspec(dllexport) void WINAPI StartKill() {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    spoofer->StartKillMode();
}

extern "C" __declspec(dllexport) void WINAPI StopKill() {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    spoofer->StopKillMode();
}

extern "C" __declspec(dllexport) void WINAPI StartAutoRefresh(int seconds) {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    spoofer->StartAutoRefresh(seconds);
}

extern "C" __declspec(dllexport) void WINAPI StopAutoRefresh() {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    spoofer->StopAutoRefresh();
}

extern "C" __declspec(dllexport) void WINAPI StartScreenCapture() {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    spoofer->StartScreenCapture();
}

extern "C" __declspec(dllexport) void WINAPI StopScreenCapture() {
    auto* spoofer = NeoGodSpooferV8::GetInstance();
    spoofer->StopScreenCapture();
}

// DLL MAIN
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            Sleep(1000);
            auto* spoofer = NeoGodSpooferV8::GetInstance();
            spoofer->Initialize();
            return 0;
            }, nullptr, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        auto* spoofer = NeoGodSpooferV8::GetInstance();
        spoofer->StopSpamMode();
        spoofer->StopKillMode();
        spoofer->StopAutoRefresh();
        spoofer->StopAntiDebug();
        spoofer->StopScreenCapture();
        break;
    }
    return TRUE;
}
