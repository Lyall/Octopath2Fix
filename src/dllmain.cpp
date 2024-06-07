#include "stdafx.h"
#include "helper.hpp"
#include <inipp/inipp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <safetyhook.hpp>

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE thisModule;

// Logger and config setup
inipp::Ini<char> ini;
std::shared_ptr<spdlog::logger> logger;
std::string sFixName = "Octopath2Fix";
std::string sFixVer = "0.9.0";
std::string sLogFile = "Octopath2Fix.log";
std::string sConfigFile = "Octopath2Fix.ini";
std::string sExeName;
std::filesystem::path sExePath;
std::filesystem::path sThisModulePath;
std::pair DesktopDimensions = { 0,0 };

// Ini variables
bool bFixAspect;
bool bFixHUD;
bool bFixFOV;
bool bIntroSkip;
bool bUncapFPS;

// Aspect ratio + HUD stuff
float fPi = (float)3.141592653;
float fAspectRatio;
float fNativeAspect = (float)16 / 9;
float fAspectMultiplier;
float fDefaultHUDWidth = (float)1920;
float fDefaultHUDHeight = (float)1080;
float fHUDWidth;
float fHUDHeight;
float fHUDWidthOffset;
float fHUDHeightOffset;

// Variables
int iCurrentResX;
int iCurrentResY;

void Logging()
{
    // Get this module path
    WCHAR thisModulePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(thisModule, thisModulePath, MAX_PATH);
    sThisModulePath = thisModulePath;
    sThisModulePath = sThisModulePath.remove_filename();

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // spdlog initialisation
    {
        try
        {
            logger = spdlog::basic_logger_st(sFixName.c_str(), sThisModulePath.string() + sLogFile, true);
            spdlog::set_default_logger(logger);

            spdlog::flush_on(spdlog::level::debug);
            spdlog::info("----------");
            spdlog::info("{} v{} loaded.", sFixName.c_str(), sFixVer.c_str());
            spdlog::info("----------");
            spdlog::info("Path to logfile: {}", sThisModulePath.string() + sLogFile);
            spdlog::info("----------");

            // Log module details
            spdlog::info("Module Name: {0:s}", sExeName.c_str());
            spdlog::info("Module Path: {0:s}", sExePath.string());
            spdlog::info("Module Address: 0x{0:x}", (uintptr_t)baseModule);
            spdlog::info("Module Timestamp: {0:d}", Memory::ModuleTimestamp(baseModule));
            spdlog::info("----------");
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
        }
    }
}

void ReadConfig()
{
    // Initialise config
    std::ifstream iniFile(sThisModulePath.string() + sConfigFile);
    if (!iniFile)
    {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVer.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sThisModulePath.string().c_str() << std::endl;
    }
    else
    {
        spdlog::info("Path to config file: {}", sThisModulePath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Read ini file
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bFixAspect);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bFixHUD);
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFixFOV);
    inipp::get_value(ini.sections["Intro Skip"], "Enabled", bIntroSkip);
    inipp::get_value(ini.sections["Uncap FPS"], "Enabled", bUncapFPS);

    // Log config parse
    spdlog::info("Config Parse: bFixAspect: {}", bFixAspect);
    spdlog::info("Config Parse: bFixHUD: {}", bFixHUD);
    spdlog::info("Config Parse: bFixFOV: {}", bFixFOV);
    spdlog::info("Config Parse: bIntroSkip: {}", bIntroSkip);
    spdlog::info("Config Parse: bUncapFPS: {}", bUncapFPS);
    spdlog::info("----------");

    // Grab desktop resolution/aspect
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();
    iCurrentResX = DesktopDimensions.first;
    iCurrentResY = DesktopDimensions.second;

    // Calculate aspect ratio
    fAspectRatio = (float)iCurrentResX / (float)iCurrentResY;
    fAspectMultiplier = fAspectRatio / fNativeAspect;

    // HUD variables
    fHUDWidth = iCurrentResY * fNativeAspect;
    fHUDHeight = (float)iCurrentResY;
    fHUDWidthOffset = (float)(iCurrentResX - fHUDWidth) / 2;
    fHUDHeightOffset = 0;
    if (fAspectRatio < fNativeAspect)
    {
        fHUDWidth = (float)iCurrentResX;
        fHUDHeight = (float)iCurrentResX / fNativeAspect;
        fHUDWidthOffset = 0;
        fHUDHeightOffset = (float)(iCurrentResY - fHUDHeight) / 2;
    }
}

void IntroSkip()
{
    if (bIntroSkip)
    {
        // Intro Skip
        uint8_t* IntroSkipScanResult = Memory::PatternScan(baseModule, "0F ?? ?? ?? ?? 48 ?? ?? ?? 48 ?? ?? ?? ?? 88 ?? C1 ?? ?? ?? 48 ?? ?? ?? 5F C3") + 0x5;
        if (IntroSkipScanResult)
        {
            spdlog::info("Intro Skip: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)IntroSkipScanResult - (uintptr_t)baseModule);

            static bool bHasSkippedIntro = false;

            static SafetyHookMid IntroSkipMidHook{};
            IntroSkipMidHook = safetyhook::create_mid(IntroSkipScanResult,
                [](SafetyHookContext& ctx)
                {
                    // ctx.rax = 0 = EUITitleFlow::eLogo
                    // Double check for bHasSkippedIntro and title flow state just in-case
                    if (ctx.rdi && !bHasSkippedIntro && ctx.rax == 0)
                    {
                        // EndTitle
                        *reinterpret_cast<BYTE*>(ctx.rdi + 0x2F8) = 1;
                        bHasSkippedIntro = true;
                    }
                });
        }
        else if (!IntroSkipScanResult)
        {
            spdlog::error("Intro Skip: Pattern scan failed.");
        }
    }
}

void CurrentResolution()
{
    // Get current resolution
    uint8_t* CurrResolutionScanResult = Memory::PatternScan(baseModule, "F2 0F ?? ?? ?? ?? ?? ?? C6 ?? ?? ?? ?? ?? 01 48 ?? ?? ?? 5F C3");
    if (CurrResolutionScanResult)
    {
        spdlog::info("Current Resolution: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)CurrResolutionScanResult - (uintptr_t)baseModule);

        static SafetyHookMid CurrResolutionMidHook{};
        CurrResolutionMidHook = safetyhook::create_mid(CurrResolutionScanResult,
            [](SafetyHookContext& ctx)
            {
                // Get ResX and ResY
                iCurrentResX = (int)ctx.xmm0.f32[0];
                iCurrentResY = (int)ctx.xmm0.f32[1];

                // Calculate aspect ratio
                fAspectRatio = (float)iCurrentResX / (float)iCurrentResY;
                fAspectMultiplier = fAspectRatio / fNativeAspect;

                // HUD variables
                fHUDWidth = iCurrentResY * fNativeAspect;
                fHUDHeight = (float)iCurrentResY;
                fHUDWidthOffset = (float)(iCurrentResX - fHUDWidth) / 2;
                fHUDHeightOffset = 0;
                if (fAspectRatio < fNativeAspect)
                {
                    fHUDWidth = (float)iCurrentResX;
                    fHUDHeight = (float)iCurrentResX / fNativeAspect;
                    fHUDWidthOffset = 0;
                    fHUDHeightOffset = (float)(iCurrentResY - fHUDHeight) / 2;
                }

                // Log details about current resolution
                spdlog::info("----------");
                spdlog::info("Current Resolution: Resolution: {}x{}", iCurrentResX, iCurrentResY);
                spdlog::info("Current Resolution: fAspectRatio: {}", fAspectRatio);
                spdlog::info("Current Resolution: fAspectMultiplier: {}", fAspectMultiplier);
                spdlog::info("Current Resolution: fHUDWidth: {}", fHUDWidth);
                spdlog::info("Current Resolution: fHUDHeight: {}", fHUDHeight);
                spdlog::info("Current Resolution: fHUDWidthOffset: {}", fHUDWidthOffset);
                spdlog::info("Current Resolution: fHUDHeightOffset: {}", fHUDHeightOffset);
                spdlog::info("----------");
            });
    }
    else if (!CurrResolutionScanResult)
    {
        spdlog::error("Current Resolution: Pattern scan failed.");
    }
}

void AspectFOV()
{
    // Aspect Ratio / FOV
    uint8_t* FOVScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? 8B ?? ?? ?? ?? ?? 89 ?? ?? 0F ?? ?? ?? ?? ?? ?? 33 ?? ?? 83 ?? ??") + 0x8;
    if (FOVScanResult)
    {
        spdlog::info("Aspect Ratio/FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FOVScanResult - (uintptr_t)baseModule);
        
        if (bFixFOV)
        {
            static SafetyHookMid FOVMidHook{};
            FOVMidHook = safetyhook::create_mid(FOVScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio > fNativeAspect)
                    {
                        ctx.xmm0.f32[0] = atanf(tanf(ctx.xmm0.f32[0] * (fPi / 360)) / fNativeAspect * fAspectRatio) * (360 / fPi);
                    }
                });
        }
   
        if (bFixAspect)
        {
            static SafetyHookMid AspectRatioMidHook{};
            AspectRatioMidHook = safetyhook::create_mid(FOVScanResult + 0xB,
                [](SafetyHookContext& ctx)
                {
                    ctx.rax = *(uint32_t*)&fAspectRatio;
                });
        }
    }
    else if (!FOVScanResult)
    {
        spdlog::error("Aspect Ratio/FOV: Pattern scan failed.");
    }
}

void HUD()
{
    if (bFixHUD)
    {
        // Center HUD
        uint8_t* HUDPositionScanResult = Memory::PatternScan(baseModule, "FF ?? 48 ?? ?? ?? ?? 0F ?? ?? 48 ?? ?? 0F ?? ?? 48 ?? ?? ?? 5F C3") + 0xA;
        if (HUDPositionScanResult)
        {
            spdlog::info("HUD: HUD Position: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)HUDPositionScanResult - (uintptr_t)baseModule);

            static SafetyHookMid HUDPositionMidHook{};
            HUDPositionMidHook = safetyhook::create_mid(HUDPositionScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (ctx.xmm0.f32[0] == 0.00f && ctx.xmm0.f32[1] == 0.00f && ctx.xmm0.f32[2] == 1.00f && ctx.xmm0.f32[3] == 1.00f)
                    {
                        // Check for left padding marker and don't center the UI element.
                        if (ctx.rcx + 0x190)
                        {
                            if (*reinterpret_cast<float*>(ctx.rcx + 0x190) == 12345.00f)
                            {
                                return;
                            }
                        }

                        if (fAspectRatio > fNativeAspect)
                        {
                            ctx.xmm0.f32[0] = (float)fHUDWidthOffset / iCurrentResX;
                            ctx.xmm0.f32[2] = 1.00f - ctx.xmm0.f32[0];
                        }
                        else if (fAspectRatio < fNativeAspect)
                        {
                            ctx.xmm0.f32[1] = (float)fHUDHeightOffset / iCurrentResY;
                            ctx.xmm0.f32[3] = 1.00f - ctx.xmm0.f32[1];
                        }
                    }
                });
        }
        else if (!HUDPositionScanResult)
        {
            spdlog::error("HUD: HUD Position: Pattern scan failed.");
        }
        
        // Fix offset markers (i.e map icons etc)
        uint8_t* MarkersScanResult = Memory::PatternScan(baseModule, "0F ?? ?? 66 ?? ?? ?? 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? 4C") + 0xA;
        if (MarkersScanResult)
        {
            spdlog::info("HUD: Markers: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MarkersScanResult - (uintptr_t)baseModule);

            static SafetyHookMid MarkersMidHook{};
            MarkersMidHook = safetyhook::create_mid(MarkersScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio > fNativeAspect)
                    {
                        ctx.xmm0.f32[0] = fHUDWidthOffset;
                    }
                    else if (fAspectRatio < fNativeAspect)
                    {
                        ctx.xmm1.f32[0] = fHUDHeightOffset;
                    }
                });
        }
        else if (!MarkersScanResult)
        {
            spdlog::error("HUD: Markers: Pattern scan failed.");
        }

        // Fix offset tooltips
        uint8_t* TooltipsScanResult = Memory::PatternScan(baseModule, "48 ?? ?? 20 0F ?? ?? 66 0F ?? ?? 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? 40 ?? ??") + 0xE;
        if (TooltipsScanResult)
        {
            spdlog::info("HUD: Tooltips: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)TooltipsScanResult - (uintptr_t)baseModule);

            static SafetyHookMid TooltipsMidHook{};
            TooltipsMidHook = safetyhook::create_mid(TooltipsScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio > fNativeAspect)
                    {
                        ctx.xmm1.f32[0] = fHUDWidthOffset;
                    }
                    else if (fAspectRatio < fNativeAspect)
                    {
                        ctx.xmm0.f32[0] = fHUDHeightOffset;
                    }

                    if (ctx.rsi)
                    {
                        // Don't add offset for letterboxing
                        if (*reinterpret_cast<float*>(ctx.rsi + 0x380) == (float)iCurrentResX)
                        {
                            ctx.xmm0.f32[0] = 0.00f;
                            ctx.xmm1.f32[0] = 0.00f;
                        }
                    }
                });
        }
        else if (!TooltipsScanResult)
        {
            spdlog::error("HUD: Tooltips: Pattern scan failed.");
        }

        // Fix world map scrolling
        uint8_t* WorldMapScanResult = Memory::PatternScan(baseModule, "0F ?? ?? 66 ?? ?? ?? 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? E8 ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ??") + 0xA;
        if (WorldMapScanResult)
        {
            spdlog::info("HUD: WorldMap: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)WorldMapScanResult - (uintptr_t)baseModule);

            static SafetyHookMid WorldMapMidHook{};
            WorldMapMidHook = safetyhook::create_mid(WorldMapScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio > fNativeAspect)
                    {
                        ctx.xmm1.f32[0] = fHUDWidth;
                    }
                    else if (fAspectRatio < fNativeAspect)
                    {
                        ctx.xmm0.f32[0] = fHUDHeight;
                    }
                });
        }
        else if (!WorldMapScanResult)
        {
            spdlog::error("HUD: WorldMap: Pattern scan failed.");
        }

        // KSFade::FadeInit
        uint8_t* KSFadeScanResult = Memory::PatternScan(baseModule, "48 ?? ?? 48 ?? ?? 0F ?? ?? ?? ?? 33 ?? 0F ?? ?? 48 ?? ?? FF ?? ?? ?? ?? ?? 0F ?? ?? 0F ?? ?? ?? ?? ?? ??");
        if (KSFadeScanResult)
        {
            spdlog::info("HUD: KSFade: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)KSFadeScanResult - (uintptr_t)baseModule);

            static SafetyHookMid KSFadeMidHook{};
            KSFadeMidHook = safetyhook::create_mid(KSFadeScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (ctx.rcx + 0x190)
                    {
                        // Set marker in Padding.Left
                        *reinterpret_cast<float*>(ctx.rcx + 0x190) = 12345.00f;
                     }
                });
        }
        else if (!KSFadeScanResult)
        {
            spdlog::error("HUD: KSFade: Pattern scan failed.");
        }

        // UIEventBackgroundFadeBase::StartFadeOut
        uint8_t* EventBackgroundFadeScanResult = Memory::PatternScan(baseModule, "48 ?? ?? 48 ?? ?? ?? 04 F3 0F ?? ?? ?? ?? ?? ?? 49 ?? ?? C7 ?? ?? ?? ?? ?? 00 00 00 00");
        if (EventBackgroundFadeScanResult)
        {
            spdlog::info("HUD: EventBackgroundFade: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)EventBackgroundFadeScanResult - (uintptr_t)baseModule);

            static SafetyHookMid EventBackgroundFadeMidHook{};
            EventBackgroundFadeMidHook = safetyhook::create_mid(EventBackgroundFadeScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (ctx.rcx + 0x190)
                    {
                        // Set marker in Padding.Left
                        *reinterpret_cast<float*>(ctx.rcx + 0x190) = 12345.00f;
                    }
                });
        }
        else if (!EventBackgroundFadeScanResult)
        {
            spdlog::error("HUD: EventBackgroundFade: Pattern scan failed.");
        }

        // Battle Wipes
        uint8_t* BattleWipeScanResult = Memory::PatternScan(baseModule, "4C ?? ?? 4C ?? ?? ?? 0F ?? ?? C1 02 ?? ?? 41 ?? ?? C3"); // Bad pattern that includes offset.
        if (BattleWipeScanResult)
        {
            spdlog::info("HUD: BattleWipe: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)BattleWipeScanResult - (uintptr_t)baseModule);

            static SafetyHookMid BattleWipeMidHook{};
            BattleWipeMidHook = safetyhook::create_mid(BattleWipeScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (ctx.rcx + 0x190)
                    {
                        // Set marker in Padding.Left
                        *reinterpret_cast<float*>(ctx.rcx + 0x190) = 12345.00f;
                    }
                });
        }
        else if (!BattleWipeScanResult)
        {
            spdlog::error("HUD: BattleWipe: Pattern scan failed.");
        }
    } 
}

void FPSCap()
{
    if (bUncapFPS)
    {
        // Uncap FPS
        uint8_t* FPSCapScanResult = Memory::PatternScan(baseModule, "73 ?? 80 ?? ?? ?? ?? ?? 00 74 ?? FF ?? ?? ?? ?? ?? 3B ?? ?? ?? ?? ??");
        if (FPSCapScanResult)
        {
            spdlog::info("FPS Cap: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FPSCapScanResult - (uintptr_t)baseModule);
            Memory::Write((uintptr_t)FPSCapScanResult, (BYTE)0xEB); // No cap fr fr
            spdlog::info("FPS Cap: Patched instruction.");
        }
        else if (!FPSCapScanResult)
        {
            spdlog::error("FPS Cap: Pattern scan failed.");
        }
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    IntroSkip();
    CurrentResolution();
    AspectFOV();
    HUD();
    FPSCap();
    return true; //end thread
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
        thisModule = hModule;
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

