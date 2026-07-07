
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <fstream>

extern "C" __declspec(dllexport) void InitializeSpoofer();
extern "C" __declspec(dllexport) void CleanupSpoofer();

static HHOOK g_hKeyboardHook = nullptr;
static HHOOK g_hMouseHook = nullptr;
static bool g_bInitialized = false;

typedef DWORD(WINAPI* tGetTickCount)();
typedef UINT(WINAPI* tGetSystemDirectoryA)(LPSTR, UINT);
typedef BOOL(WINAPI* tGetComputerNameA)(LPSTR, LPDWORD);
typedef LONG(WINAPI* tRegQueryValueExA)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

static tGetTickCount pOriginalGetTickCount = nullptr;
static tGetComputerNameA pOriginalGetComputerNameA = nullptr;
static tRegQueryValueExA pOriginalRegQueryValueExA = nullptr;

std::string GenerateFakeComputerName() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    return "PC-" + std::to_string(dis(gen));
}

std::string GenerateFakeHWID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::string hwid = "HWID-";
    for (int i = 0; i < 8; i++) {
        int val = dis(gen);
        hwid += (val < 10) ? ('0' + val) : ('A' + val - 10);
        if (i % 2 == 1 && i != 7) hwid += '-';
    }
    return hwid;
}

DWORD WINAPI HookedGetTickCount() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 100);

    if (pOriginalGetTickCount) {
        return pOriginalGetTickCount() + dis(gen);
    }
    return 0;
}

BOOL WINAPI HookedGetComputerNameA(LPSTR lpBuffer, LPDWORD nSize) {
    if (lpBuffer && *nSize > 0) {
        std::string fakeName = GenerateFakeComputerName();
        strncpy_s(lpBuffer, *nSize, fakeName.c_str(), fakeName.length());
        *nSize = static_cast<DWORD>(fakeName.length());
        return TRUE;
    }
    return FALSE;
}

LONG WINAPI HookedRegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
    LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    if (lpValueName) {
        std::string valueName(lpValueName);

        if (valueName == "MachineGuid") {
            if (lpData && lpcbData) {
                std::string fakeGUID = GenerateFakeHWID();
                if (*lpcbData >= fakeGUID.length() + 1) {
                    strcpy_s(reinterpret_cast<char*>(lpData), *lpcbData, fakeGUID.c_str());
                    *lpcbData = static_cast<DWORD>(fakeGUID.length() + 1);
                    if (lpType) *lpType = REG_SZ;
                    return ERROR_SUCCESS;
                }
            }
        }

        if (valueName.find("EasyAntiCheat") != std::string::npos ||
            valueName.find("BattlEye") != std::string::npos) {
            return ERROR_FILE_NOT_FOUND;
        }
    }


    if (pOriginalRegQueryValueExA) {
        return pOriginalRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    }
    return ERROR_PROC_NOT_FOUND;
}


HANDLE WINAPI HookedCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    if (lpFileName) {
        std::string fileName(lpFileName);

        if (fileName.find("update") != std::string::npos ||
            fileName.find("patch") != std::string::npos ||
            fileName.find("version") != std::string::npos) {
            return reinterpret_cast<HANDLE>(0x12345678);
        }
    }

    typedef HANDLE(WINAPI* tCreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    static tCreateFileA pOriginal = reinterpret_cast<tCreateFileA>(
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateFileA"));

    if (pOriginal) {
        return pOriginal(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
            dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }
    return INVALID_HANDLE_VALUE;
}

bool InstallHooks() {
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    HMODULE hAdvapi32 = GetModuleHandleA("advapi32.dll");

    if (!hKernel32 || !hAdvapi32) return false;

    pOriginalGetTickCount = reinterpret_cast<tGetTickCount>(
        GetProcAddress(hKernel32, "GetTickCount"));
    pOriginalGetComputerNameA = reinterpret_cast<tGetComputerNameA>(
        GetProcAddress(hKernel32, "GetComputerNameA"));
    pOriginalRegQueryValueExA = reinterpret_cast<tRegQueryValueExA>(
        GetProcAddress(hAdvapi32, "RegQueryValueExA"));

    return true;
}

bool HookInternetFunctions() {
    HMODULE hWininet = LoadLibraryA("wininet.dll");
    if (!hWininet) return false;

    return true;
}


bool HideFromAntiCheat() {
    SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);

    return true;
}


void LogMessage(const std::string& message) {
    // В продакшене лучше отключить логгирование
    /*
    std::ofstream log("spoofer.log", std::ios::app);
    if (log.is_open()) {
        log << "[" << GetCurrentThreadId() << "] " << message << std::endl;
        log.close();
    }
    */
}

extern "C" __declspec(dllexport) void InitializeSpoofer() {
    if (g_bInitialized) return;

    try {
        LogMessage("Initializing spoofer...");

        if (InstallHooks()) {
            LogMessage("Hooks installed successfully");
        }
        else {
            LogMessage("Failed to install hooks");
        }

        if (HideFromAntiCheat()) {
            LogMessage("Anti-cheat bypass activated");
        }

        if (HookInternetFunctions()) {
            LogMessage("Update blocking activated");
        }

        SetEnvironmentVariableA("COMPUTERNAME", GenerateFakeComputerName().c_str());
        SetEnvironmentVariableA("USERNAME", "User");
        SetEnvironmentVariableA("FAKE_HWID", GenerateFakeHWID().c_str());

        g_bInitialized = true;
        LogMessage("Spoofer initialized successfully");

    }
    catch (...) {
        LogMessage("Error during spoofer initialization");
    }
}

extern "C" __declspec(dllexport) void CleanupSpoofer() {
    if (!g_bInitialized) return;

    try {
        LogMessage("Cleaning up spoofer...");

        if (g_hKeyboardHook) {
            UnhookWindowsHookEx(g_hKeyboardHook);
            g_hKeyboardHook = nullptr;
        }

        if (g_hMouseHook) {
            UnhookWindowsHookEx(g_hMouseHook);
            g_hMouseHook = nullptr;
        }

        g_bInitialized = false;
        LogMessage("Spoofer cleaned up");

    }
    catch (...) {
        LogMessage("Error during spoofer cleanup");
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_PROCESS_DETACH:
        CleanupSpoofer();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) const char* GetSpooferVersion() {
    return "RustSpoofer v1.0";
}

extern "C" __declspec(dllexport) bool IsInitialized() {
    return g_bInitialized;
}

extern "C" __declspec(dllexport) void TestSpoof() {
    MessageBoxA(nullptr, "Spoofer DLL loaded successfully!", "Test", MB_OK);
}
