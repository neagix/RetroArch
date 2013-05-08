/* RetroArch - A frontend for libretro.
 * Copyright (C) 2010-2013 - Hans-Kristian Arntzen
 * Copyright (C) 2011-2013 - Daniel De Matteis
 *
 * RetroArch is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with RetroArch.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <pspkernel.h>
#include <pspdebug.h>

#include <stdint.h>
#include "../../boolean.h"
#include <stddef.h>
#include <string.h>

#include "../../psp/sdk_defines.h"

PSP_MODULE_INFO("RetroArch PSP", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);
PSP_HEAP_SIZE_MAX();

static int exit_callback(int arg1, int arg2, void *common)
{
   g_extern.verbose = false;

#ifdef HAVE_FILE_LOGGER
   if (g_extern.log_file)
      fclose(g_extern.log_file);
   g_extern.log_file = NULL;
#endif

   sceKernelExitGame();
   return 0;
}

static void get_environment_settings(int argc, char *argv[])
{
   (void)argc;
   (void)argv;

#ifdef HAVE_FILE_LOGGER
   g_extern.log_file = fopen("ms0:/retroarch-log.txt", "w");
#endif

   fill_pathname_basedir(default_paths.port_dir, argv[0], sizeof(default_paths.port_dir));
   RARCH_LOG("port dir: [%s]\n", default_paths.port_dir);

   snprintf(default_paths.core_dir, sizeof(default_paths.core_dir), "%s/cores", default_paths.port_dir);
   snprintf(default_paths.savestate_dir, sizeof(default_paths.savestate_dir), "%s/savestates", default_paths.core_dir);
   snprintf(default_paths.filesystem_root_dir, sizeof(default_paths.filesystem_root_dir), "/");
   snprintf(default_paths.filebrowser_startup_dir, sizeof(default_paths.filebrowser_startup_dir), default_paths.filesystem_root_dir);
   snprintf(default_paths.sram_dir, sizeof(default_paths.sram_dir), "%s/savefiles", default_paths.core_dir);

   snprintf(default_paths.system_dir, sizeof(default_paths.system_dir), "%s/system", default_paths.core_dir);

   /* now we fill in all the variables */
   snprintf(default_paths.menu_border_file, sizeof(default_paths.menu_border_file), "%s/borders/Menu/main-menu.png", default_paths.core_dir);
   snprintf(default_paths.input_presets_dir, sizeof(default_paths.input_presets_dir), "%s/presets", default_paths.core_dir);
   snprintf(default_paths.border_dir, sizeof(default_paths.border_dir), "%s/borders", default_paths.core_dir);
   snprintf(g_extern.config_path, sizeof(g_extern.config_path), "%s/retroarch.cfg", default_paths.port_dir);
}

int callback_thread(SceSize args, void *argp)
{
   int cbid = sceKernelCreateCallback("Exit callback", exit_callback, NULL);
   sceKernelRegisterExitCallback(cbid);
   sceKernelSleepThreadCB();

   return 0;
}

static int setup_callback(void)
{
   int thread_id = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);

   if (thread_id >= 0)
      sceKernelStartThread(thread_id, 0, 0);

   return thread_id;
}

static void system_init(void)
{
   //initialize debug screen
   pspDebugScreenInit();
   pspDebugScreenClear();

   setup_callback();
}

static int system_process_args(int argc, char *argv[])
{
   (void)argc;
   (void)argv;
   return 0;
}

static void system_deinit(void)
{
   sceKernelExitGame();
}

static void system_exitspawn(void)
{
}
