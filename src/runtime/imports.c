/* Imports that must stay in the exe's import table even though no code calls them: the SDL based replacements for DirectDraw / DirectSound (src\platform\ddraw, dsound)
 * are named ddraw.dll / dsound.dll and get loaded because the exe imports DirectDrawCreate / DirectSoundCreate from them (the original exe did via two jmp thunks). */
#include "../rr.h"

__declspec(dllimport) long __stdcall DirectDrawCreate(void *guid, void **dd, void *outer);
__declspec(dllimport) long __stdcall DirectSoundCreate(void *guid, void **ds, void *outer);

void *volatile rr_keep_imports[2] = { (void *)DirectDrawCreate, (void *)DirectSoundCreate };
