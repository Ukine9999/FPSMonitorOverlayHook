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
    
    std::string filename = std::string(name) + ".bin";
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        if (file_size > 0 && file_size <= size)
        {
            file.read(static_cast<char*>(address), file_size);
            if (file.good())
            {
                std::cout << "Successfully loaded " << file_size << " bytes from '" 
                          << filename << "' into mapping '" << name << "'" << std::endl;
            }
            else
            {
                std::cout << "Failed to read data from file: " << filename << std::endl;
            }
        }
        else if (file_size > size)
        {
            std::cout << "Warning: File '" << filename << "' is larger than mapping size. "
                      << "Only first " << size << " bytes will be loaded." << std::endl;
            file.read(static_cast<char*>(address), size);
        }
        else
            std::cout << "File '" << filename << "' is empty or invalid." << std::endl;
        
        
        file.close();
    }
    else
        std::cout << "File '" << filename << "' not found or cannot be opened. "
                  << "Memory mapping created with zero-initialized data." << std::endl;
}
static void dump_file_mapping(const char* name)
{
    HANDLE handle = OpenFileMappingA(FILE_MAP_READ, FALSE, name);
    if (handle == nullptr)
    {
        std::cout << "Failed to open file mapping: " << name << " (Error: " << GetLastError() << ")" << std::endl;
        return;
    }
    
    void* address = MapViewOfFile(handle, FILE_MAP_READ, 0, 0, 0);
    if (address == nullptr)
    {
        CloseHandle(handle);
        std::cout << "Failed to map view of file: " << name << " (Error: " << GetLastError() << ")" << std::endl;
        return;
    }
    
    // Получаем реальный размер маппинга
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(address, &mbi, sizeof(mbi)) == 0)
    {
        UnmapViewOfFile(address);
        CloseHandle(handle);
        std::cout << "Failed to query memory info for: " << name << std::endl;
        return;
    }
    
    SIZE_T actual_size = mbi.RegionSize;
    
    std::string filename = std::string(name) + ".bin";
    std::ofstream file(filename, std::ios::binary);
    
    if (!file.is_open())
    {
        UnmapViewOfFile(address);
        CloseHandle(handle);
        std::cout << "Failed to create dump file: " << filename << std::endl;
        return;
    }
    
    file.write(static_cast<const char*>(address), actual_size);
    
    if (file.good())
    {
        std::cout << "Successfully dumped " << actual_size << " bytes from mapping '" 
                  << name << "' to file '" << filename << "'" << std::endl;
    }
    else
    {
        std::cout << "Failed to write data to dump file: " << filename 
                  << " (failbit: " << file.fail() << ", badbit: " << file.bad() << ")" << std::endl;
    }
    
    file.close();
    UnmapViewOfFile(address);
    CloseHandle(handle);
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
