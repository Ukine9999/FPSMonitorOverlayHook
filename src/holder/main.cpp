#include <fstream>
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

const DWORD size = 0x100000;

static void create_file_mapping(const char* name)
{
    HANDLE handle = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, name);
    void* address = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (address == nullptr)
    {
        CloseHandle(handle);
        std::cout << "failed to map view of file: " << name << " " << std::endl;
        return;
    }
}

int main(int argc, char* argv[]) 
{
    CreateEventA(nullptr, TRUE, FALSE, "FPSMonitorGlobalShutdownEvent_64");
    CreateMutexA(nullptr, FALSE, "FPSMonitor_FrameTimesBufferExMutex");
    CreateMutexA(nullptr, FALSE, "FPSMonitorMultirunMutex");
    
    create_file_mapping("FPSMonitor_Memory_CaptureInfo");
    create_file_mapping("FPSMonitor_FrameTimesBuffer");
    create_file_mapping("FPSMonitor_FrameTimesBufferEx");
    create_file_mapping("FPSMonitor_Memory_FrameBuffer");
    create_file_mapping("FPSMonitorSteamInterfaceData");
    create_file_mapping("FPSMonitorOverlayIndexBuffer");
    system("fps-mon64.exe /start");  // NOLINT(concurrency-mt-unsafe)
    return getchar();
}
