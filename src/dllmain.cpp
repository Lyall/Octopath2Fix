#include "stdafx.h"
#include "helper.hpp"

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

inipp::Ini<char> ini;

// INI Variables
bool bAspectFix;
bool bFOVFix;
bool bHUDFix;
bool bHUDCenter;
int iCustomResX;
int iCustomResY;
int iInjectionDelay;
float fAdditionalFOV;
int iAspectFix;
int iFOVFix;
int iHUDFix;
int iHUDCenter;

// Variables
int iNarrowAspect;
float fNewX;
float fNewY;
float fNativeAspect = 1.777777791f;
float fPi = 3.14159265358979323846f;
float fNewAspect;
float fZero = (float)0;
float fOne = (float)1;
float fTwo = (float)2;

string sExeName;
string sGameName;
string sExePath;
string sGameVersion;
string sFixVer = "0.0.2";

// CurrResolution Hook
DWORD64 CurrResolutionReturnJMP;
float UIWidth;
float UIHeight;
float UIHorOffset;
float UIVertOffset;
void __declspec(naked) CurrResolution_CC()
{
    __asm
    {
        mov r12d, r9d                          // Original code
        mov rbx, rdx                           // Original code
        mov rdi, [rax + r8 * 0x8]              // Original code
        add rdi, rcx                           // Original code
        mov eax, [rdi]                         // Original code

        // Get current resolution + aspect ratio
        mov[iCustomResX], r15d                 // Grab current resX
        mov[iCustomResY], r12d                 // Grab current resY
        cvtsi2ss xmm14, r15d
        cvtsi2ss xmm15, r12d
        divss xmm14, xmm15
        movss[fNewAspect], xmm14               // Grab current aspect ratio

        // Get 16:9 HUD values
        movd xmm14, [iCustomResY]
        cvtdq2ps xmm14, xmm14
        mulss xmm14, [fNativeAspect]
        movss [UIWidth], xmm14
        movd xmm14, [iCustomResX]
        cvtdq2ps xmm14, xmm14
        subss xmm14, [UIWidth]
        divss xmm14, [fTwo]
        movss[UIHorOffset], xmm14
        movd xmm14, [iCustomResX]
        cvtdq2ps xmm14, xmm14
        divss xmm14, [fNativeAspect]
        movss [UIHeight], xmm14
        movd xmm14, [iCustomResY]
        cvtdq2ps xmm14, xmm14
        subss xmm14, [UIHeight]
        divss xmm14, [fTwo]
        movss [UIVertOffset], xmm14

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
        mov [iNarrowAspect], 1
        mov eax, [fNewAspect]                  // Move new aspect to eax
        cmp eax, [fNativeAspect]               // Compare new aspect to native
        jle originalCode                       // Skip FOV fix if fNewAspect<=fNativeAspect
        cmp[iFOVFix], 1                        // Check if FOVFix is enabled
        je modifyFOV                           // jmp to FOV fix
        jmp originalCode                       // jmp to originalCode

        modifyFOV:
            mov[iNarrowAspect], 0              // Note that aspect ratio is <= 1.78
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

        originalCode:
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

// FOV Culling Hook
DWORD64 FOVCullingReturnJMP;
void __declspec(naked) FOVCulling_CC()
{
    __asm
    {
        cmp iFOVFix, 1
        je cullingFix
        movss[rdx + 0x000002E8], xmm1      // Original code
        xor r8d, r8d                       // Original code
        movsd xmm0, [rbp + 0x000000D0]     // Original code
        jmp[FOVCullingReturnJMP]

        cullingFix:
            movss xmm1, [fOne]                 // 90/90, there is undoubtedly a smarter way of doing this
            movss[rdx + 0x000002E8], xmm1      // Original code
            xor r8d, r8d                       // Original code
            movsd xmm0, [rbp + 0x000000D0]     // Original code
            jmp[FOVCullingReturnJMP]
    }
}

// CenterHUD Hook
DWORD64 CenterHUDReturnJMP;
void __declspec(naked) CenterHUD_CC()
{
    __asm
    {
        movups xmm0, [rcx + 0x00000210]         // Original code
        mov rax, rdx                            // Original code
        movups[rdx], xmm0                       // Original code
        cmp [iNarrowAspect], 0
        je resizeHUDHor
        cmp [iNarrowAspect], 1
        je resizeHUDVert
        ret                                     // Original code
        jmp[CenterHUDReturnJMP]                 // Just in case

        resizeHUDHor:
            movss xmm0, [UIHorOffset]
            movd xmm15, [iCustomResX]
            cvtdq2ps xmm15, xmm15
            divss xmm0, xmm15
            movss [rdx], xmm0
            movss xmm0, [fOne]
            subss xmm0, [rdx]
            movss [rdx+0x8], xmm0
            xorps xmm15, xmm15
            ret                                     // Original code
            jmp[CenterHUDReturnJMP]                 // Just in case

        resizeHUDVert:
            movss xmm0, [UIVertOffset]
            movd xmm15, [iCustomResY]
            cvtdq2ps xmm15, xmm15
            divss xmm0, xmm15
            movss[rdx+0x4], xmm0
            movss xmm0, [fOne]
            subss xmm0, [rdx+0x4]
            movss [rdx+0xC], xmm0
            xorps xmm15, xmm15
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
        cmp iNarrowAspect, 1
        je markersVert
        cvtdq2ps xmm0, xmm0                     // Original code
        movd xmm1, eax                          // Original code
        cvtdq2ps xmm1, xmm1                     // Original code
        movss xmm0, [UIHorOffset]
        subss xmm3, xmm0                        // Original code
        jmp[HUDMarkersReturnJMP]

        markersVert:
            cvtdq2ps xmm0, xmm0                 // Original code
            movd xmm1, eax                      // Original code
            cvtdq2ps xmm1, xmm1                 // Original code
            movss xmm1, [UIVertOffset]
            subss xmm3, xmm0                    // Original code
            jmp[HUDMarkersReturnJMP]
    }
}

// BattleCursor Hook
DWORD64 BattleCursorReturnJMP;
void __declspec(naked) BattleCursor_CC()
{
    __asm
    {
        cmp iNarrowAspect, 1
        je cursorVert
        movss xmm0, [UIHorOffset]
        subss xmm1, xmm0                        // Original code
        movd xmm0, ecx                          // Original code
        cvtdq2ps xmm0, xmm0                     // Original code
        movss[rbx], xmm1                        // Original code
        jmp[BattleCursorReturnJMP]

        cursorVert:
            subss xmm1, xmm0                        // Original code
            movd xmm0, ecx                          // Original code
            cvtdq2ps xmm0, xmm0                     // Original code
            movss xmm0, [UIVertOffset]
            movss[rbx], xmm1                        // Original code
            jmp[BattleCursorReturnJMP]
    }
}

// HUDTooltips Hook
DWORD64 HUDTooltipsReturnJMP;
void __declspec(naked) HUDTooltips_CC()
{
    __asm
    {
        cmp iNarrowAspect, 1
        je tooltipsVert
        cvtdq2ps xmm1, xmm1                     // Original code
        movd xmm0, eax                          // Original code
        cvtdq2ps xmm0, xmm0                     // Original code
        movss xmm1, [UIHorOffset]
        movss[rbx], xmm1                        // Original code
        jmp[HUDTooltipsReturnJMP]

        tooltipsVert:
            cvtdq2ps xmm1, xmm1                     // Original code
            movd xmm0, eax                          // Original code
            cvtdq2ps xmm0, xmm0                     // Original code
            movss xmm0, [UIVertOffset]
            movss[rbx], xmm1                        // Original code
            jmp[HUDTooltipsReturnJMP]
    }
}

// HUDMap Hook
DWORD64 HUDMapReturnJMP;
void __declspec(naked) HUDMap_CC()
{
    __asm
    {
        cmp iNarrowAspect, 1
        je mapVert
        cvtdq2ps xmm1, xmm1                     // Original code
        movd xmm0, eax                          // Original code
        cvtdq2ps xmm0, xmm0                     // Original code
        movss xmm1, [UIWidth]
        movss[rbx], xmm1                        // Original code
        jmp[HUDMapReturnJMP]

        mapVert:
            cvtdq2ps xmm1, xmm1                     // Original code
            movd xmm0, eax                          // Original code
            cvtdq2ps xmm0, xmm0                     // Original code
            movss xmm0, [UIHeight]
            movss[rbx], xmm1                        // Original code
            jmp[HUDMapReturnJMP]
    }
}

// HUDBlackBars Hook
DWORD64 HUDBlackBarsReturnJMP;
void __declspec(naked) HUDBlackBars_CC()
{
    __asm
    {
        divss xmm2, xmm0                        // Original code
        movaps xmm1, xmm2                       // Original code
        mulss xmm2, [rbx + 0x04]                // Original code
        mulss xmm1, [rbx]                       // Original code
        cmp rcx, 264
        je disableBlackBars
        jmp[HUDBlackBarsReturnJMP]

        disableBlackBars:
            xorps xmm1, xmm1
            xorps xmm2, xmm2
            jmp[HUDBlackBarsReturnJMP]
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
    inipp::get_value(ini.sections["Center HUD"], "Enabled", bHUDCenter);
    iHUDCenter = (int)bHUDCenter;
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
    LOG_F(INFO, "Config Parse: bHUDCenter: %d", bHUDCenter);
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
        // Grab important values like current resolution/aspect ratio.
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

        // Set correct aspect ratio + FOV
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

    if (bFOVFix)
    {
        uint8_t* FOVCullingScanResult = Memory::PatternScan(baseModule, "8B ?? ?? ?? ?? ?? F2 ?? ?? ?? ?? ?? 89 ?? ?? ?? 84 ?? 75 ??");
        if (FOVCullingScanResult)
        {
            DWORD64 FOVCullingAddress = (uintptr_t)FOVCullingScanResult - 0x13;
            int FOVCullingHookLength = Memory::GetHookLength((char*)FOVCullingAddress, 13);
            FOVCullingReturnJMP = FOVCullingAddress + FOVCullingHookLength;
            Memory::DetourFunction64((void*)FOVCullingAddress, FOVCulling_CC, FOVCullingHookLength);

            LOG_F(INFO, "FOV Culling: Hook length is %d bytes", FOVCullingHookLength);
            LOG_F(INFO, "FOV Culling: Hook address is 0x%" PRIxPTR, (uintptr_t)FOVCullingAddress);           
        }
        else if (!FOVCullingScanResult)
        {
            LOG_F(INFO, "FOV Culling: Pattern scan failed.");
        }
    }
}

void HUDFix()
{
    if (bHUDCenter)
    {
        // Center HUD to 16:9
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
    }

    if (bHUDFix)
    {
        // Fix offset markers (i.e map icons etc)
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

        // Fix hand cursor icon during battle being offset
        // Battle cursor fix is unnecessary if the HUD is not centered
        if (bHUDCenter)
        {
            uint8_t* BattleCursorScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? 66 ?? ?? ?? 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? 84 ??");
            if (BattleCursorScanResult)
            {
                DWORD64 BattleCursorAddress = (uintptr_t)BattleCursorScanResult;
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

        // Fix HUD tooltips being offset
        if (bHUDCenter)
        {
            uint8_t* HUDTooltipsScanResult = Memory::PatternScan(baseModule, "0F 5B ?? 66 0F ?? ?? 0F 5B ?? F3 0F ?? ?? F3 0F ?? ?? ?? 40 84");
            if (HUDTooltipsScanResult)
            {
                DWORD64 HUDTooltipsAddress = (uintptr_t)HUDTooltipsScanResult;
                int HUDTooltipsHookLength = Memory::GetHookLength((char*)HUDTooltipsAddress, 13);
                HUDTooltipsReturnJMP = HUDTooltipsAddress + HUDTooltipsHookLength;
                Memory::DetourFunction64((void*)HUDTooltipsAddress, HUDTooltips_CC, HUDTooltipsHookLength);

                LOG_F(INFO, "HUD Tooltips: Hook length is %d bytes", HUDTooltipsHookLength);
                LOG_F(INFO, "HUD Tooltips: Hook address is 0x%" PRIxPTR, (uintptr_t)HUDTooltipsAddress);
            }
            else if (!HUDTooltipsScanResult)
            {
                LOG_F(INFO, "HUD Tooltips: Pattern scan failed.");
            }
        }

        // Fix HUD black bars with tooltip fix
        if (bHUDCenter)
        {
            uint8_t* HUDBlackBarsScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? 0F 28 ?? F3 0F ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? 4C 8D");
            if (HUDBlackBarsScanResult)
            {
                DWORD64 HUDBlackBarsAddress = (uintptr_t)HUDBlackBarsScanResult;
                int HUDBlackBarsHookLength = Memory::GetHookLength((char*)HUDBlackBarsAddress, 15);
                HUDBlackBarsReturnJMP = HUDBlackBarsAddress + HUDBlackBarsHookLength;
                Memory::DetourFunction64((void*)HUDBlackBarsAddress, HUDBlackBars_CC, HUDBlackBarsHookLength);

                LOG_F(INFO, "HUD Black bars: Hook length is %d bytes", HUDBlackBarsHookLength);
                LOG_F(INFO, "HUD Black bars: Hook address is 0x%" PRIxPTR, (uintptr_t)HUDBlackBarsAddress);
            }
            else if (!HUDBlackBarsScanResult)
            {
                LOG_F(INFO, "HUD Black bars: Pattern scan failed.");
            }
        }

        // Fix world map in-game and "new game"
        if (bHUDCenter)
        {
            uint8_t* HUDMapScanResult = Memory::PatternScan(baseModule, "0F ?? ?? 66 ?? ?? ?? 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? E8 ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ??");
            if (HUDMapScanResult)
            {
                DWORD64 HUDMapAddress = (uintptr_t)HUDMapScanResult;
                int HUDMapHookLength = Memory::GetHookLength((char*)HUDMapAddress, 13);
                HUDMapReturnJMP = HUDMapAddress + HUDMapHookLength;
                Memory::DetourFunction64((void*)HUDMapAddress, HUDMap_CC, HUDMapHookLength);

                LOG_F(INFO, "HUD Map: Hook length is %d bytes", HUDMapHookLength);
                LOG_F(INFO, "HUD Map: Hook address is 0x%" PRIxPTR, (uintptr_t)HUDMapAddress);
            }
            else if (!HUDMapScanResult)
            {
                LOG_F(INFO, "HUD Map: Pattern scan failed.");
            }
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

