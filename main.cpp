#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

int getProcId(const char *target) {
    DWORD pID = 0;
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    do {
        if (strcmp(pe32.szExeFile, target) == 0) {
            pID = pe32.th32ProcessID;
            break;
        }
    } while (Process32Next(hSnapshot, &pe32));
    CloseHandle(hSnapshot);
    return pID;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <processName> <dllName>" << std::endl;
        return 1;
    }

    int pID = getProcId(argv[1]);
    if (pID == 0) {
        std::cerr << "Can't find process: " << argv[1] << ". get pid: " << pID << std::endl;
        return 1;
    }

    std::cout << "injected to pid: " << pID << std::endl;
    char dllPath[MAX_PATH] = {0};
    GetFullPathNameA(argv[2], MAX_PATH, dllPath, NULL);
    if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES) {
        std::cerr << "dll file " << dllPath << " doesn't exist. " << std::endl;
        return 1;
    }

    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        NULL, pID);
    LPVOID pszLibFileRemote = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE,
                                             PAGE_READWRITE);
    WriteProcessMemory(hProcess, pszLibFileRemote, dllPath, strlen(dllPath) + 1, NULL);
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE) LoadLibraryA,
                                        pszLibFileRemote, NULL, NULL);

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, dllPath, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return 0;
}
