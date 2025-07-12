#include <clocale>
#include <windows.h>
#include <cstdio>
#include <winerror.h>
#include "third_party/MinHook.h"
#include <dxgi.h>
#include <d3d11.h>
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_dx11.h"
#include "third_party/imgui/imgui_impl_win32.h"

#pragma comment(lib, "third_party/libMinHook.x64.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")


ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
ID3D11RenderTargetView* targetView = nullptr;
HWND window = nullptr;
WNDPROC originalWndProc = nullptr;
bool isInitialized = false;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

LRESULT __stdcall WndProc(const HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, w_param, l_param))
        return true;

    return CallWindowProc(originalWndProc, hwnd, msg, w_param, l_param);
}

typedef HRESULT(__stdcall* present) (IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags);
present originalPresent;

HRESULT hkPresent(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags)
{
    if (!isInitialized)
    {
        if (SUCCEEDED(swap_chain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device))))
        {
            device->GetImmediateContext(&context);
            DXGI_SWAP_CHAIN_DESC sd;
            swap_chain->GetDesc(&sd);
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&pBackBuffer));
            device->CreateRenderTargetView(pBackBuffer, nullptr, &targetView);
            pBackBuffer->Release();
            originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
            ImGui::CreateContext();
            ImGui_ImplWin32_Init(window);
            ImGui_ImplDX11_Init(device, context); 
            isInitialized = true;
        } 
        else
            return originalPresent(swap_chain, sync_interval, flags);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Test");
    ImGui::Text("Hello world!");
    ImGui::End();
    ImGui::EndFrame();
    ImGui::Render();
    context->OMSetRenderTargets(1, &targetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    return originalPresent(swap_chain, sync_interval, flags);
}
 
inline bool initFpsMonitorHook()
{
    auto fpsMonitorModule = GetModuleHandleA("fps-mon64.dll");
    if (!fpsMonitorModule)
        return false;
    
    const auto lib_base = reinterpret_cast<uintptr_t>(fpsMonitorModule);

    const auto present = lib_base + 0x12E50; // сигнатура для хука: ? 89 5c 24 08 ? 89 74 24 10 57 ? 83 ec 20 ? ? ? ? 07 00 00 41 8b d8 8b f2
    
    if (MH_CreateHook(reinterpret_cast<LPVOID>(present), &hkPresent, reinterpret_cast<LPVOID*>(&originalPresent)) != MH_OK)
        return false;
    
    if (MH_EnableHook(reinterpret_cast<LPVOID>(present)) != MH_OK)
        return false;

    return true;
}


BOOL APIENTRY DllMain(HMODULE module, DWORD  reason, LPVOID reserved)
{
    if(reason == DLL_PROCESS_ATTACH)
    { 
        if (MH_Initialize() != MH_OK)
            return 1;
        
        initFpsMonitorHook();
    }

    return true;
}