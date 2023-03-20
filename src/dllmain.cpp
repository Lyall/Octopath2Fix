#include "stdafx.h"
#include "helper.hpp"

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

inipp::Ini<char> ini;

// INI Variables
bool bAspectFix;
bool bFOVFix;
bool bHUDFix;
int iCustomResX;
int iCustomResY;
int iInjectionDelay;
float fAdditionalFOV;
int iAspectFix;
int iFOVFix;
int iHUDFix;

// Variables
float fNewX;
float fNewY;
float fNativeAspect = 1.777777791f;
float fPi = 3.14159265358979323846f;
float fNewAspect;
string sExeName;
string sGameName;
string sExePath;
string sGameVersion;
string sFixVer = "1.0.0";

// CurrResolution Hook
DWORD64 CurrResolutionReturnJMP;
void __declspec(naked) CurrResolution_CC()
{
    __asm
    {
        mov r12d, r9d                          // Original code
        mov rbx, rdx                           // Original code
        mov rdi, [rax + r8 * 0x8]              // Original code
        add rdi, rcx                           // Original code
        mov eax, [rdi]                         // Original code

        mov[iCustomResX], r15d                 // Grab current resX
        mov[iCustomResY], r12d                 // Grab current resY
        cvtsi2ss xmm14, r15d
        cvtsi2ss xmm15, r12d
        divss xmm14, xmm15
        movss[fNewAspect], xmm14               // Grab current aspect ratio
        xorps xmm14, xmm14
        xorps xmm15, xmm15
        jmp[CurrResolutionReturnJMP]
    }
}

// Aspect Ratio/FOV Hook
DWORD64 AspectFOVFixReturnJMP;
float FOVPiDiv;
float FOVDivPi;
float FOVFinalValue;
void __declspec(naked) AspectFOVFix_CC()
{
    __asm
    {
        mov eax, [fNewAspect]                  // Move new aspect to eax
        cmp eax, [fNativeAspect]               // Compare new aspect to native
        jle originalCode                       // Skip FOV fix if fNewAspect<=fNativeAspect
        cmp[iFOVFix], 1                        // Check if FOVFix is enabled
        je modifyFOV                           // jmp to FOV fix
        jmp originalCode                       // jmp to originalCode

        modifyFOV :
            fld dword ptr[rbx + 0x1F8]         // Push original FOV to FPU register st(0)
            fmul[FOVPiDiv]                     // Multiply st(0) by Pi/360
            fptan                              // Get partial tangent. Store result in st(1). Store 1.0 in st(0)
            fxch st(1)                         // Swap st(1) to st(0)
            fdiv[fNativeAspect]                // Divide st(0) by 1.778~
            fmul[fNewAspect]                   // Multiply st(0) by new aspect ratio
            fxch st(1)                         // Swap st(1) to st(0)
            fpatan                             // Get partial arc tangent from st(0), st(1)
            fmul[FOVDivPi]                     // Multiply st(0) by 360/Pi
            fadd[fAdditionalFOV]               // Add additional FOV
            fstp[FOVFinalValue]                // Store st(0) 
            movss xmm0, [FOVFinalValue]        // Copy final FOV value to xmm0
            jmp originalCode

        originalCode :
            movss[rdi + 0x18], xmm0            // Original code
            cmp[iAspectFix], 1
            je modifyAspect
            mov eax, [rbx + 0x00000208]        // Original code
            mov[rdi + 0x2C], eax               // Original code
            jmp[AspectFOVFixReturnJMP]

        modifyAspect:
            mov eax, [fNewAspect]
            mov[rdi + 0x2C], eax               // Original code
            jmp[AspectFOVFixReturnJMP]
    }
}

// --- ABSOLUTE JANK ALERT! ---
// There is a better way of doing this, but for now this will suffice
// CenterHUD Hook
DWORD64 CenterHUDReturnJMP;
float UIWidth;
float UIOffset;
float UI1;
float UI2;
float UI3;
float UI4;
float fZero = (float)0;
float fOne = (float)1;
float fTwo = (float)2;
void __declspec(naked) CenterHUD_CC()
{
    __asm
    {
        // Get 16:9 HUD values
        movd xmm15, [iCustomResY]
        cvtdq2ps xmm15, xmm15
        mulss xmm15, [fNativeAspect]
        movss [UIWidth], xmm15
        movd xmm15, [iCustomResX]
        cvtdq2ps xmm15, xmm15
        subss xmm15, [UIWidth]
        divss xmm15, [fTwo]
        movss [UIOffset], xmm15

        // magic stuff
        movd xmm14, [iCustomResX]
        cvtdq2ps xmm14, xmm14
        divss xmm15, xmm14
        movss [UI1], xmm15
        xorps xmm15, xmm15
        movss[UI2], xmm15
        movss xmm15, [fOne]
        subss xmm15, [UI1]
        movss [UI3], xmm15
        movss xmm15, [fOne]
        movss [UI4], xmm15
        xorps xmm14, xmm14
        xorps xmm15, xmm15

        movups xmm0, [rcx + 0x00000210]         // Original code
        mov rax, rdx                            // Original code
        movups xmm0, [UI1]
        movups[rdx], xmm0                       // Original code
        ret                                     // Original code
        jmp[CenterHUDReturnJMP]                 // Just in case
    }
}

// HUDMarkers Hook
DWORD64 HUDMarkersReturnJMP;
void __declspec(naked) HUDMarkers_CC()
{
    __asm
    {
        cvtdq2ps xmm0, xmm0                     // Original code
        movd xmm1, eax                          // Original code
        cvtdq2ps xmm1, xmm1                     // Original code
        movss xmm0, [UIOffset]
        subss xmm3, xmm0                        // Original code
        jmp[HUDMarkersReturnJMP]
    }
}

// BattleCursor Hook
DWORD64 BattleCursorReturnJMP;
void __declspec(naked) BattleCursor_CC()
{
    __asm
    {
        movd xmm0, ecx                          // Original code
        cvtdq2ps xmm0, xmm0                     // Original code
        movss xmm0, [UIOffset]
        shr rcx, 32                             // Original code
        subss xmm1, xmm0                        // Original code
        jmp[BattleCursorReturnJMP]
    }
}

void Logging()
{
    loguru::add_file("Octopath2Fix.log", loguru::Truncate, loguru::Verbosity_MAX);
    loguru::set_thread_name("Main");

    LOG_F(INFO, "Octopath2Fix v%s loaded", sFixVer.c_str());
}

void ReadConfig()
{
    // Get game name and exe path
    LPWSTR exePath = new WCHAR[_MAX_PATH];
    GetModuleFileNameW(baseModule, exePath, _MAX_PATH);
    wstring exePathWString(exePath);
    sExePath = string(exePathWString.begin(), exePathWString.end());
    wstring wsGameName = Memory::GetVersionProductName();
    sExeName = sExePath.substr(sExePath.find_last_of("/\\") + 1);
    sGameName = string(wsGameName.begin(), wsGameName.end());

    LOG_F(INFO, "Game Name: %s", sGameName.c_str());
    LOG_F(INFO, "Game Path: %s", sExePath.c_str());

    // Initialize config
    // UE4 games use launchers so config path is relative to launcher
    sGameVersion = "Steam";
    std::ifstream iniFile(".\\Octopath_Traveler2\\Binaries\\Win64\\Octopath2Fix.ini");
    if (!iniFile)
    {
        LOG_F(ERROR, "Failed to load config file. (Steam)");
    }
    else
    {
        ini.parse(iniFile);
    }
    
    LOG_F(INFO, "Game Version: %s", sGameVersion.c_str());

    inipp::get_value(ini.sections["Octopath2Fix Parameters"], "InjectionDelay", iInjectionDelay);
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    iAspectFix = (int)bAspectFix;
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bHUDFix);
    iHUDFix = (int)bHUDFix;
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFOVFix);
    iFOVFix = (int)bFOVFix;
    inipp::get_value(ini.sections["Fix FOV"], "AdditionalFOV", fAdditionalFOV);

    // Custom resolution
    if (iCustomResX > 0 && iCustomResY > 0)
    {
        fNewX = (float)iCustomResX;
        fNewY = (float)iCustomResY;
        fNewAspect = (float)iCustomResX / (float)iCustomResY;
    }
    else
    {
        // Grab desktop resolution
        RECT desktop;
        GetWindowRect(GetDesktopWindow(), &desktop);
        fNewX = (float)desktop.right;
        fNewY = (float)desktop.bottom;
        iCustomResX = (int)desktop.right;
        iCustomResY = (int)desktop.bottom;
        fNewAspect = (float)desktop.right / (float)desktop.bottom;
    }

    // Log config parse
    LOG_F(INFO, "Config Parse: iInjectionDelay: %dms", iInjectionDelay);
    LOG_F(INFO, "Config Parse: bAspectFix: %d", bAspectFix);
    LOG_F(INFO, "Config Parse: bFOVFix: %d", bFOVFix);
    LOG_F(INFO, "Config Parse: bHUDFix: %d", bHUDFix);
    LOG_F(INFO, "Config Parse: fAdditionalFOV: %.2f", fAdditionalFOV);
    LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    LOG_F(INFO, "Config Parse: fNewX: %.2f", fNewX);
    LOG_F(INFO, "Config Parse: fNewY: %.2f", fNewY);
    LOG_F(INFO, "Config Parse: fNewAspect: %.4f", fNewAspect);
}

void AspectFOVFix()
{
    if (bAspectFix || bFOVFix)
    {
        uint8_t* CurrResolutionScanResult = Memory::PatternScan(baseModule, "33 ?? B9 ?? ?? ?? ?? 45 ?? ?? 48 ?? ?? 4A ?? ?? ?? 48 ?? ?? 8B ??");
        if (CurrResolutionScanResult)
        {
            DWORD64 CurrResolutionAddress = (uintptr_t)CurrResolutionScanResult + 0x7;
            int CurrResolutionHookLength = Memory::GetHookLength((char*)CurrResolutionAddress, 13);
            CurrResolutionReturnJMP = CurrResolutionAddress + CurrResolutionHookLength;
            Memory::DetourFunction64((void*)CurrResolutionAddress, CurrResolution_CC, CurrResolutionHookLength);

            LOG_F(INFO, "Current Resolution: Hook length is %d bytes", CurrResolutionHookLength);
            LOG_F(INFO, "Current Resolution: Hook address is 0x%" PRIxPTR, (uintptr_t)CurrResolutionAddress);
        }
        else if (!CurrResolutionScanResult)
        {
            LOG_F(INFO, "Current Resolution: Pattern scan failed.");
        }

        uint8_t* AspectFOVFixScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? 8B ?? ?? ?? ?? ?? 89 ?? ?? 0F ?? ?? ?? ?? ?? ?? 33 ?? ?? 83 ?? ??");
        if (AspectFOVFixScanResult)
        {
            FOVPiDiv = fPi / 360;
            FOVDivPi = 360 / fPi;

            DWORD64 AspectFOVFixAddress = (uintptr_t)AspectFOVFixScanResult + 0x8;
            int AspectFOVFixHookLength = Memory::GetHookLength((char*)AspectFOVFixAddress, 13);
            AspectFOVFixReturnJMP = AspectFOVFixAddress + AspectFOVFixHookLength;
            Memory::DetourFunction64((void*)AspectFOVFixAddress, AspectFOVFix_CC, AspectFOVFixHookLength);

            LOG_F(INFO, "Aspect Ratio/FOV: Hook length is %d bytes", AspectFOVFixHookLength);
            LOG_F(INFO, "Aspect Ratio/FOV: Hook address is 0x%" PRIxPTR, (uintptr_t)AspectFOVFixAddress);
        }
        else if (!AspectFOVFixScanResult)
        {
            LOG_F(INFO, "Aspect Ratio/FOV: Pattern scan failed.");
        }
    }
}

void HUDFix()
{
    if (bHUDFix)
    {
        uint8_t* CenterHUDScanResult = Memory::PatternScan(baseModule, "4C 03 ?? 4C 89 ?? ?? 48 8D ?? ?? ?? E8 ?? ?? ?? ?? 0F 10 ?? 0F 11 ?? 48 83 C4 ?? 5B C3 CC CC CC CC CC CC CC CC CC CC CC CC 48 89 ?? ?? ?? 48 89");
        if (CenterHUDScanResult)
        {
            DWORD64 CenterHUDAddress = Memory::GetAbsolute((uintptr_t)CenterHUDScanResult + 0xD);
            int CenterHUDHookLength = Memory::GetHookLength((char*)CenterHUDAddress, 13);
            CenterHUDReturnJMP = CenterHUDAddress + CenterHUDHookLength;
            Memory::DetourFunction64((void*)CenterHUDAddress, CenterHUD_CC, CenterHUDHookLength);

            LOG_F(INFO, "Center HUD: Hook length is %d bytes", CenterHUDHookLength);
            LOG_F(INFO, "Center HUD: Hook address is 0x%" PRIxPTR, (uintptr_t)CenterHUDAddress);
        }
        else if (!CenterHUDScanResult)
        {
            LOG_F(INFO, "Center HUD: Pattern scan failed.");
        }

        uint8_t* HUDMarkersScanResult = Memory::PatternScan(baseModule, "0F ?? ?? 66 ?? ?? ?? 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? 4C");
        if (HUDMarkersScanResult)
        {
            DWORD64 HUDMarkersAddress = (uintptr_t)HUDMarkersScanResult;
            int HUDMarkersHookLength = Memory::GetHookLength((char*)HUDMarkersAddress, 13);
            HUDMarkersReturnJMP = HUDMarkersAddress + HUDMarkersHookLength;
            Memory::DetourFunction64((void*)HUDMarkersAddress, HUDMarkers_CC, HUDMarkersHookLength);

            LOG_F(INFO, "HUD Markers: Hook length is %d bytes", HUDMarkersHookLength);
            LOG_F(INFO, "HUD Markers: Hook address is 0x%" PRIxPTR, (uintptr_t)HUDMarkersAddress);
        }
        else if (!HUDMarkersScanResult)
        {
            LOG_F(INFO, "HUD Markers: Pattern scan failed.");
        }

        uint8_t* BattleCursorScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? 66 ?? ?? ?? 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? 84 ??");
        if (BattleCursorScanResult)
        {
            DWORD64 BattleCursorAddress = (uintptr_t)BattleCursorScanResult - 0xB;
            int BattleCursorHookLength = Memory::GetHookLength((char*)BattleCursorAddress, 13);
            BattleCursorReturnJMP = BattleCursorAddress + BattleCursorHookLength;
            Memory::DetourFunction64((void*)BattleCursorAddress, BattleCursor_CC, BattleCursorHookLength);

            LOG_F(INFO, "Battle Cursor: Hook length is %d bytes", BattleCursorHookLength);
            LOG_F(INFO, "Battle Cursor: Hook address is 0x%" PRIxPTR, (uintptr_t)BattleCursorAddress);
        }
        else if (!BattleCursorScanResult)
        {
            LOG_F(INFO, "Battle Cursor: Pattern scan failed.");
        }
    }
    
}


DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    Sleep(iInjectionDelay);
    AspectFOVFix();
    HUDFix();
    return true; // end thread
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);

        if (mainHandle)
        {
            CloseHandle(mainHandle);
        }
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

