#pragma once
#include <d3d12.h>
#include <dxgidebug.h>
#include <wrl/client.h>

struct D3DResourceLeakChecker {
    ~D3DResourceLeakChecker()
    {
        Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
        typedef HRESULT(WINAPI * PFN_DXGI_GET_DEBUG_INTERFACE1)(UINT, REFIID, void**);

        HMODULE dxgidebug = LoadLibraryExW(L"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (dxgidebug) {
            auto dxgiGetDebugInterface1 = reinterpret_cast<PFN_DXGI_GET_DEBUG_INTERFACE1>(
                GetProcAddress(dxgidebug, "DXGIGetDebugInterface1"));

            if (dxgiGetDebugInterface1) {
                if (SUCCEEDED(dxgiGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
                    debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
                    debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
                    debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
                }
            }

            FreeLibrary(dxgidebug);
        }
    }
};