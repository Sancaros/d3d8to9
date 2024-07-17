/**
 * Copyright (C) 2015 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/d3d8to9#license
 */

#include "d3dx9.hpp"
#include "d3d8to9.hpp"

PFN_D3DXAssembleShader D3DXAssembleShader = nullptr;
PFN_D3DXDisassembleShader D3DXDisassembleShader = nullptr;
PFN_D3DXLoadSurfaceFromSurface D3DXLoadSurfaceFromSurface = nullptr;

#ifndef D3D8TO9NOLOG
 // Very simple logging for the purpose of debugging only.
std::ofstream LOG;
#endif

// Array of VendorIds representing integrated graphics
const DWORD integratedGraphicsVendorIds[] = {
    0x8086,  // Intel
    0x1002,  // AMD (some integrated)
    0x10DE,  // NVIDIA (some integrated)
    // Add more VendorIds as needed for other integrated graphics types
};

// Check if the system is using integrated graphics
BOOL IsIntegratedGraphics(const D3DADAPTER_IDENTIFIER9& adapterIdentifier)
{
    DWORD vendorId = adapterIdentifier.VendorId;

    // Check against the array of integrated graphics VendorIds
    for (DWORD i = 0; i < sizeof(integratedGraphicsVendorIds) / sizeof(integratedGraphicsVendorIds[0]); ++i)
    {
        if (vendorId == integratedGraphicsVendorIds[i])
        {
            return TRUE;
        }
    }

    return FALSE;
}

extern "C" IDirect3D8 * WINAPI Direct3DCreate8(UINT SDKVersion)
{
    // Check if the system is using integrated graphics
    BOOL isIntegrated = FALSE;
    IDirect3D9Ex* d3d = nullptr;

    // Create Direct3D object
    HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d);
    if (FAILED(hr) || d3d == nullptr)
    {
        return nullptr;
    }

    D3DADAPTER_IDENTIFIER9 adapterIdentifier;
    hr = d3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterIdentifier);
    if (FAILED(hr))
    {
        d3d->Release();
        return nullptr;
    }

    // Check if it's an integrated GPU
    isIntegrated = IsIntegratedGraphics(adapterIdentifier);

    d3d->Release();

    // If it's integrated graphics, return nullptr to indicate failure to create Direct3D8
    if (isIntegrated)
    {
        return nullptr;
    }

#ifndef D3D8TO9NOLOG
    static bool LogMessageFlag = true;

    if (!LOG.is_open())
    {
        LOG.open("d3d8.log", std::ios::trunc);
    }

    if (!LOG.is_open() && LogMessageFlag)
    {
        LogMessageFlag = false;
        MessageBox(nullptr, TEXT("Failed to open debug log file \"d3d8.log\"!"), nullptr, MB_ICONWARNING);
    }

    LOG << "Redirecting 'Direct3DCreate8(" << SDKVersion << ")' ..." << std::endl;
    LOG << "> Passing on to 'Direct3DCreate9':" << std::endl;
#endif

    IDirect3D9* const d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

    if (d3d9 == nullptr)
    {
        return nullptr;
    }

    // Load D3DX
    if (!D3DXAssembleShader || !D3DXDisassembleShader || !D3DXLoadSurfaceFromSurface)
    {
        const HMODULE module = LoadLibrary(TEXT("d3dx9_43.dll"));

        if (module != nullptr)
        {
            D3DXAssembleShader = reinterpret_cast<PFN_D3DXAssembleShader>(GetProcAddress(module, "D3DXAssembleShader"));
            D3DXDisassembleShader = reinterpret_cast<PFN_D3DXDisassembleShader>(GetProcAddress(module, "D3DXDisassembleShader"));
            D3DXLoadSurfaceFromSurface = reinterpret_cast<PFN_D3DXLoadSurfaceFromSurface>(GetProcAddress(module, "D3DXLoadSurfaceFromSurface"));
        }
        else
        {
#ifndef D3D8TO9NOLOG
            LOG << "Failed to load d3dx9_43.dll! Some features will not work correctly." << std::endl;
#endif
            if (MessageBox(nullptr, TEXT(
                "Failed to load d3dx9_43.dll! Some features will not work correctly.\n\n"
                "It's required to install the \"Microsoft DirectX End-User Runtime\" in order to use d3d8to9, or alternatively get the DLLs from this NuGet package:\nhttps://www.nuget.org/packages/Microsoft.DXSDK.D3DX\n\n"
                "Please click \"OK\" to open the official download page or \"Cancel\" to continue anyway."), nullptr, MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND | MB_OKCANCEL | MB_DEFBUTTON1) == IDOK)
            {
                ShellExecute(nullptr, TEXT("open"), TEXT("https://www.microsoft.com/download/details.aspx?id=35"), nullptr, nullptr, SW_SHOW);

                return nullptr;
            }
        }
    }

    return new Direct3D8(d3d9);
}