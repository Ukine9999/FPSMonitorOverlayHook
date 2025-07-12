#include <fstream>
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

std::vector<HANDLE> handles;
std::vector<void*> addresses;
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
    
    
    handles.push_back(handle);
    addresses.push_back(address);
}

static bool dump_file_mapping_to_file(const std::string& mapping_name)
{
    std::string filename = mapping_name + ".bin";
    HANDLE mapping_handle = OpenFileMappingA(FILE_MAP_READ, FALSE, mapping_name.c_str());
    if (mapping_handle == NULL)
    {
        std::cout << "Failed to open file mapping: " << mapping_name << " Error: " << GetLastError() << std::endl;
        return false;
    }

    void* mapped_view = MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);
    if (mapped_view == NULL)
    {
        std::cout << "Failed to map view of file: " << mapping_name << " Error: " << GetLastError() << std::endl;
        CloseHandle(mapping_handle);
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(mapped_view, &mbi, sizeof(mbi)) == 0)
    {
        std::cout << "Failed to query memory info for: " << mapping_name << " Error: " << GetLastError() << std::endl;
        UnmapViewOfFile(mapped_view);
        CloseHandle(mapping_handle);
        return false;
    }

    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        std::cout << "Failed to create file: " << filename << std::endl;
        UnmapViewOfFile(mapped_view);
        CloseHandle(mapping_handle);
        return false;
    }

    file.write(static_cast<const char*>(mapped_view), mbi.RegionSize);
    
    if (file.fail())
    {
        std::cout << "Failed to write data to file: " << filename << std::endl;
        file.close();
        UnmapViewOfFile(mapped_view);
        CloseHandle(mapping_handle);
        return false;
    }

    file.close();
    UnmapViewOfFile(mapped_view);
    CloseHandle(mapping_handle);

    std::cout << "Successfully dumped " << mbi.RegionSize << " bytes from mapping '" 
              << mapping_name << "' to file '" << filename << "'" << std::endl;
    
    return true;
}

int main(int argc, char* argv[])
{
    CreateEventA(nullptr, TRUE, FALSE, "FPSMonitorGlobalShutdownEvent_64");
    CreateMutexA(nullptr, FALSE, "FPSMonitor_FrameTimesBufferExMutex");
    CreateMutexA(nullptr, FALSE, "FPSMonitorMultirunMutex");

    dump_file_mapping_to_file("FPSMonitor_Memory_CaptureInfo");
    dump_file_mapping_to_file("FPSMonitor_FrameTimesBuffer");
    dump_file_mapping_to_file("FPSMonitor_FrameTimesBufferEx");
    dump_file_mapping_to_file("FPSMonitor_Memory_FrameBuffer");
    dump_file_mapping_to_file("FPSMonitorSteamInterfaceData");
    dump_file_mapping_to_file("FPSMonitorOverlayIndexBuffer");
    dump_file_mapping_to_file("FPSMonitorOverlayBuffer_2");
    dump_file_mapping_to_file("FPSMonitorOverlayBuffer_3");
    dump_file_mapping_to_file("FPSMonitorOverlayBuffer_4");
    dump_file_mapping_to_file("FPSMonitorOverlayBuffer_5");
    
    create_file_mapping("FPSMonitor_Memory_CaptureInfo");
    create_file_mapping("FPSMonitor_FrameTimesBuffer");
    create_file_mapping("FPSMonitor_FrameTimesBufferEx");
    create_file_mapping("FPSMonitor_Memory_FrameBuffer");
    create_file_mapping("FPSMonitorSteamInterfaceData");
    create_file_mapping("FPSMonitorOverlayIndexBuffer");
    create_file_mapping("FPSMonitorOverlayBuffer_2"); // буффер отрисовки
    create_file_mapping("FPSMonitorOverlayBuffer_3"); // буффер отрисовки
    create_file_mapping("FPSMonitorOverlayBuffer_4"); // буффер отрисовки 
    create_file_mapping("FPSMonitorOverlayBuffer_5"); // буффер отрисовки
    system("fps-mon64.exe /start");  // NOLINT(concurrency-mt-unsafe)
    return getchar();
}
