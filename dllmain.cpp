#define WIN32_LEAN_AND_MEAN
#include <vector>
#include <windows.h>

using std::vector;

typedef struct {
    DWORD address;
    vector<BYTE> bytes;

} patch_t;

bool IsLargeAddressAware(DWORD modBase)
{
    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)modBase + ((PIMAGE_DOS_HEADER)modBase)->e_lfanew);

    return (ntHeader->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0;
}

void WriteToMemory(DWORD modBase, const patch_t &patch)
{
    DWORD oldProtection;
    BYTE *pAddress = reinterpret_cast<BYTE *>(modBase + patch.address);
    size_t length = patch.bytes.size();

    VirtualProtect(pAddress, length, PAGE_EXECUTE_READWRITE, &oldProtection);
    memcpy(pAddress, patch.bytes.data(), length);
    VirtualProtect(pAddress, length, oldProtection, NULL);
}

DWORD WINAPI MainThread(LPVOID param)
{
    const DWORD modBase = reinterpret_cast<DWORD>(GetModuleHandleA(NULL));

    if (IsLargeAddressAware(modBase))
    {
        const vector<patch_t> patches =
        {
            //inside CleanupThingsRelatedToEvictedFamily()
            //replace IsDeletedByEvict() with false
            {0x00AFBE56, {0x31, 0xC0, 0x90, 0x90, 0x90, 0x90}},

            //inside GetLotFileInfo()
            //remove the evicted flag check, always add the lot objects value to the total
            {0x0044844A, {0x90, 0x90}}
        };

        for(const patch_t &patch: patches)
        {
            WriteToMemory(modBase, patch);
        }
    }

    FreeLibraryAndExitThread((HMODULE)param, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(0, 0, MainThread, hModule, 0, 0);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}