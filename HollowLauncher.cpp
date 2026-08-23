#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <string>

std::vector<BYTE> ReadFileToBytes(const std::string& path) {
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return {};
    DWORD size = GetFileSize(hFile, NULL);
    std::vector<BYTE> buffer(size);
    DWORD read = 0;
    ReadFile(hFile, buffer.data(), size, &read, NULL);
    CloseHandle(hFile);
    return buffer;
}

bool HollowIntoProcess(const std::string& targetPath, const std::vector<BYTE>& peData) {
    // 1. Create suspended process
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    if (!CreateProcessA(targetPath.c_str(), NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        return false;
    }

    // 2. Get thread context
    CONTEXT ctx = { sizeof(ctx) };
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(pi.hThread, &ctx);

    // 3. Unmap original process memory
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    auto pNtUnmapViewOfSection = (LONG(NTAPI*)(HANDLE, PVOID))GetProcAddress(hNtDll, "NtUnmapViewOfSection");
    pNtUnmapViewOfSection(pi.hProcess, (PVOID)ctx.Rdx);

    // 4. Allocate new memory
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)peData.data();
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(peData.data() + dos->e_lfanew);
    
    PVOID pBase = VirtualAllocEx(pi.hProcess, (PVOID)nt->OptionalHeader.ImageBase, nt->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!pBase) {
        pBase = VirtualAllocEx(pi.hProcess, NULL, nt->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!pBase) { TerminateProcess(pi.hProcess, 1); return false; }
    }

    // 5. Write PE headers and sections
    WriteProcessMemory(pi.hProcess, pBase, peData.data(), peData.size(), NULL);

    // 6. Set entry point
    ctx.Rcx = (DWORD64)pBase + nt->OptionalHeader.AddressOfEntryPoint;
    SetThreadContext(pi.hThread, &ctx);

    // 7. Resume
    ResumeThread(pi.hThread);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    // ---- STEP A: Create a fake "Intel" process in Temp folder ----
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string fakeIntelPath = std::string(tempPath) + "IntelService.exe";
    
    // Copy svchost.exe to Temp as IntelService.exe
    CopyFileA("C:\\Windows\\System32\\svchost.exe", fakeIntelPath.c_str(), FALSE);

    // ---- STEP B: Read your aimbot EXE bytes ----
    std::vector<BYTE> peData1 = ReadFileToBytes("GforceFpsStable.exe");
    std::vector<BYTE> peData2 = ReadFileToBytes("NvidiaColorRgb.exe");

    if (peData1.empty() || peData2.empty()) {
        MessageBoxA(NULL, "Failed to read aimbot EXE files.", "Error", MB_OK);
        return 1;
    }

    // ---- STEP C: Inject both into separate IntelService processes ----
    // Spawn two instances of IntelService.exe and hollow them
    HollowIntoProcess(fakeIntelPath, peData1);
    HollowIntoProcess(fakeIntelPath, peData2);

    // ---- STEP D: Clean up the temp file (optional, can delete after injection) ----
    // DeleteFileA(fakeIntelPath.c_str()); // Uncomment if you want zero disk trace

    return 0;
}