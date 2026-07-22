#include "osys_n64.h"
#include "base/main.h"
#include "common/system.h"
#include <libdragon.h>

int main(void) {
    debug_init_isviewer();
    debug_init_usblog();
    debugf("\n[FT64] FullThrottle64 / real ScummVM bring-up\n");

    if (get_memory_size() < 8u*1024u*1024u) {
        console_init();
        printf("FULL THROTTLE 64\n\nEXPANSION PAK REQUIRED\n");
        while (1) console_render();
    }

    g_system = new OSystem_N64();
    if (!g_system) {
        while (1) {}
    }

    /*
     * Bypass the launcher.  This is still ScummVM's real main path; we simply
     * provide the same arguments a desktop user would:
     *     scummvm -p rom:/game scumm:ft
     */
    char arg0[] = "scummvm";
    char arg1[] = "-p";
    char arg2[] = "rom:/game";
    char arg3[] = "--subtitles";
    char arg4[] = "scumm:ft";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };

    debugf("[FT64] entering scummvm_main(scumm:ft)\n");
    int result = scummvm_main(5, argv);
    debugf("[FT64] scummvm_main returned %d\n", result);
    g_system->quit();
    return result;
}
