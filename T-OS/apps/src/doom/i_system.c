#include <stdint.h>
#include <stdarg.h>

typedef uint8_t byte;
extern volatile uint64_t timer_ticks;

int I_GetTime(void) {
    // 35Hz DOOM timer derived from 1000Hz T-OS PIT
    return (int)((timer_ticks * 35) / 1000);
}

void I_Init(void) {
    // Already handled in kernel/main
}

void I_Quit(void) {
    // Reboot or Halt
    __asm__ volatile("cli; hlt");
}

void I_Error(char *error, ...) {
    // Simple kernel panic for DOOM
    va_list argptr;
    va_start(argptr, error);
    // Print error would go here
    va_end(argptr);
    I_Quit();
}

void I_WaitVBL(int count) {
    uint64_t target = timer_ticks + (count * 1000 / 35);
    while (timer_ticks < target);
}

byte* I_ZoneBase(int* size) {
    static byte zone[16 * 1024 * 1024];
    *size = sizeof(zone);
    return zone;
}

void I_BeginRead(void) {}
void I_EndRead(void) {}
void I_SubmitSound(void) {}
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch) {}
void I_ShutdownSound(void) {}
void I_InitSound(void) {}
void I_SetMusicVolume(int volume) {}
void I_PauseSong(int handle) {}
void I_ResumeSong(int handle) {}
void I_RegisterSong(void* data) {}
void I_PlaySong(int handle, int looping) {}
void I_UnRegisterSong(int handle) {}
void I_StopSong(int handle) {}
void I_StopSound(int handle) {}
int I_GetSfxLumpNum(void* sfx) { return 0; }
int I_StartSound(int id, int vol, int sep, int pitch, int priority) { return 0; }
void I_InitMusic(void) {}
void I_ShutdownMusic(void) {}
