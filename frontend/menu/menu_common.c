/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2013 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2013 - Daniel De Matteis
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "menu_common.h"

#include "../../performance.h"
#include "../../file.h"
#ifdef HAVE_FILEBROWSER
#include "utils/file_browser.h"
#else
#include "utils/file_list.h"
#endif

#include "../../compat/posix_string.h"

rgui_handle_t *rgui;

#ifdef HAVE_SHADER_MANAGER
void shader_manager_init(rgui_handle_t *rgui)
{
   config_file_t *conf = NULL;
   char cgp_path[PATH_MAX];

   const char *ext = path_get_extension(g_settings.video.shader_path);
   if (strcmp(ext, "glslp") == 0 || strcmp(ext, "cgp") == 0)
   {
      conf = config_file_new(g_settings.video.shader_path);
      if (conf)
      {
         if (gfx_shader_read_conf_cgp(conf, &rgui->shader))
            gfx_shader_resolve_relative(&rgui->shader, g_settings.video.shader_path);
         config_file_free(conf);
      }
   }
   else if (strcmp(ext, "glsl") == 0 || strcmp(ext, "cg") == 0)
   {
      strlcpy(rgui->shader.pass[0].source.cg, g_settings.video.shader_path,
            sizeof(rgui->shader.pass[0].source.cg));
      rgui->shader.passes = 1;
   }
   else
   {
      const char *shader_dir = *g_settings.video.shader_dir ?
         g_settings.video.shader_dir : g_settings.system_directory;

      fill_pathname_join(cgp_path, shader_dir, "rgui.glslp", sizeof(cgp_path));
      conf = config_file_new(cgp_path);

      if (!conf)
      {
         fill_pathname_join(cgp_path, shader_dir, "rgui.cgp", sizeof(cgp_path));
         conf = config_file_new(cgp_path);
      }

      if (conf)
      {
         if (gfx_shader_read_conf_cgp(conf, &rgui->shader))
            gfx_shader_resolve_relative(&rgui->shader, cgp_path);
         config_file_free(conf);
      }
   }
}

void shader_manager_get_str(struct gfx_shader *shader,
      char *type_str, size_t type_str_size, unsigned type)
{
   if (type == RGUI_SETTINGS_SHADER_APPLY)
      *type_str = '\0';
   else if (type == RGUI_SETTINGS_SHADER_PASSES)
      snprintf(type_str, type_str_size, "%u", shader->passes);
   else
   {
      unsigned pass = (type - RGUI_SETTINGS_SHADER_0) / 3;
      switch ((type - RGUI_SETTINGS_SHADER_0) % 3)
      {
         case 0:
            if (*shader->pass[pass].source.cg)
               fill_pathname_base(type_str,
                     shader->pass[pass].source.cg, type_str_size);
            else
               strlcpy(type_str, "N/A", type_str_size);
            break;

         case 1:
            switch (shader->pass[pass].filter)
            {
               case RARCH_FILTER_LINEAR:
                  strlcpy(type_str, "Linear", type_str_size);
                  break;

               case RARCH_FILTER_NEAREST:
                  strlcpy(type_str, "Nearest", type_str_size);
                  break;

               case RARCH_FILTER_UNSPEC:
                  strlcpy(type_str, "Don't care", type_str_size);
                  break;
            }
            break;

         case 2:
         {
            unsigned scale = shader->pass[pass].fbo.scale_x;
            if (!scale)
               strlcpy(type_str, "Don't care", type_str_size);
            else
               snprintf(type_str, type_str_size, "%ux", scale);
            break;
         }
      }
   }
}
#endif

#ifdef HAVE_FILEBROWSER

static bool directory_parse(void *data, const char *path)
{
   filebrowser_t *filebrowser = (filebrowser_t*)data;

   struct string_list *list = dir_list_new(path,
         filebrowser->current_dir.extensions, true);
   if(!list || list->size < 1)
      return false;
   
   dir_list_sort(list, true);

   filebrowser->current_dir.ptr   = 0;
   strlcpy(filebrowser->current_dir.directory_path,
         path, sizeof(filebrowser->current_dir.directory_path));

   if (filebrowser->list)
      dir_list_free(filebrowser->list);

   filebrowser->list = list;

   return true;

}

static void filebrowser_free(void *data)
{
   filebrowser_t *filebrowser = (filebrowser_t*)data;

   dir_list_free(filebrowser->list);
   filebrowser->list = NULL;
   filebrowser->current_dir.ptr   = 0;
   free(filebrowser);
}

void filebrowser_set_root_and_ext(void *data, const char *ext, const char *root_dir)
{
   filebrowser_t *filebrowser = (filebrowser_t*)data;
   
   if (ext)
      strlcpy(filebrowser->current_dir.extensions, ext,
            sizeof(filebrowser->current_dir.extensions));

   strlcpy(filebrowser->current_dir.root_dir,
         root_dir, sizeof(filebrowser->current_dir.root_dir));
   filebrowser_iterate(filebrowser, FILEBROWSER_ACTION_RESET);
}

#define GET_CURRENT_PATH(browser) (browser->list->elems[browser->current_dir.ptr].data)

bool filebrowser_iterate(void *data, unsigned action)
{
   filebrowser_t *filebrowser = (filebrowser_t*)data;
   bool ret = true;
   unsigned entries_to_scroll = 19;

   switch(action)
   {
      case FILEBROWSER_ACTION_UP:
         filebrowser->current_dir.ptr--;
         if (filebrowser->current_dir.ptr >= filebrowser->list->size)
            filebrowser->current_dir.ptr = filebrowser->list->size - 1;
         break;
      case FILEBROWSER_ACTION_DOWN:
         filebrowser->current_dir.ptr++;
         if (filebrowser->current_dir.ptr >= filebrowser->list->size)
            filebrowser->current_dir.ptr = 0;
         break;
      case FILEBROWSER_ACTION_LEFT:
         if (filebrowser->current_dir.ptr <= 5)
            filebrowser->current_dir.ptr = 0;
         else
            filebrowser->current_dir.ptr -= 5;
         break;
      case FILEBROWSER_ACTION_RIGHT:
         filebrowser->current_dir.ptr = (min(filebrowser->current_dir.ptr + 5, 
         filebrowser->list->size-1));
         break;
      case FILEBROWSER_ACTION_SCROLL_UP:
         if (filebrowser->current_dir.ptr <= entries_to_scroll)
            filebrowser->current_dir.ptr= 0;
         else
            filebrowser->current_dir.ptr -= entries_to_scroll;
         break;
      case FILEBROWSER_ACTION_SCROLL_DOWN:
         filebrowser->current_dir.ptr = (min(filebrowser->current_dir.ptr + 
         entries_to_scroll, filebrowser->list->size-1));
         break;
      case FILEBROWSER_ACTION_OK:
         ret = directory_parse(filebrowser, GET_CURRENT_PATH(filebrowser));
         break;
      case FILEBROWSER_ACTION_CANCEL:
         fill_pathname_parent_dir(filebrowser->current_dir.directory_path,
               filebrowser->current_dir.directory_path,
               sizeof(filebrowser->current_dir.directory_path));

         ret = directory_parse(filebrowser, filebrowser->current_dir.directory_path);
         break;
      case FILEBROWSER_ACTION_RESET:
         ret = directory_parse(filebrowser, filebrowser->current_dir.root_dir);
         break;
      case FILEBROWSER_ACTION_RESET_CURRENT_DIR:
         ret = directory_parse(filebrowser, filebrowser->current_dir.directory_path);
         break;
      case FILEBROWSER_ACTION_PATH_ISDIR:
         ret = filebrowser->list->elems[filebrowser->current_dir.ptr].attr.b;
         break;
      case FILEBROWSER_ACTION_NOOP:
      default:
         break;
   }

   if (ret)
      strlcpy(filebrowser->current_dir.path, GET_CURRENT_PATH(filebrowser),
         sizeof(filebrowser->current_dir.path));

   return ret;
}

void filebrowser_update(void *data, uint64_t input, const char *extensions)
{
   filebrowser_action_t action = FILEBROWSER_ACTION_NOOP;
   bool ret = true;

   if (input & (1ULL << DEVICE_NAV_DOWN))
      action = FILEBROWSER_ACTION_DOWN;
   else if (input & (1ULL << DEVICE_NAV_UP))
      action = FILEBROWSER_ACTION_UP;
   else if (input & (1ULL << DEVICE_NAV_RIGHT))
      action = FILEBROWSER_ACTION_RIGHT;
   else if (input & (1ULL << DEVICE_NAV_LEFT))
      action = FILEBROWSER_ACTION_LEFT;
   else if (input & (1ULL << DEVICE_NAV_R2))
      action = FILEBROWSER_ACTION_SCROLL_DOWN;
   else if (input & (1ULL << DEVICE_NAV_L2))
      action = FILEBROWSER_ACTION_SCROLL_UP;
   else if (input & (1ULL << DEVICE_NAV_A))
   {
      char tmp_str[PATH_MAX];
      fill_pathname_parent_dir(tmp_str, rgui->browser->current_dir.directory_path, sizeof(tmp_str));

      if (tmp_str[0] != '\0')
         action = FILEBROWSER_ACTION_CANCEL;
   }
   else if (input & (1ULL << DEVICE_NAV_START))
   {
      action = FILEBROWSER_ACTION_RESET;
      filebrowser_set_root_and_ext(rgui->browser, NULL, default_paths.filesystem_root_dir);
      strlcpy(rgui->browser->current_dir.extensions, extensions,
            sizeof(rgui->browser->current_dir.extensions));
#ifdef HAVE_RMENU_XUI
      filebrowser_fetch_directory_entries(1ULL << DEVICE_NAV_B);
#endif
   }

   if (action != FILEBROWSER_ACTION_NOOP)
      ret = filebrowser_iterate(rgui->browser, action);

   if (!ret)
      msg_queue_push(g_extern.msg_queue, "ERROR - Failed to open directory.", 1, 180);
}

#else

struct rgui_file
{
   char *path;
   unsigned type;
   size_t directory_ptr;
};

void rgui_list_push(void *userdata,
      const char *path, unsigned type, size_t directory_ptr)
{
   rgui_list_t *list = (rgui_list_t*)userdata;

   if (!list)
      return;

   if (list->size >= list->capacity)
   {
      list->capacity++;
      list->capacity *= 2;
      list->list = (struct rgui_file*)realloc(list->list, list->capacity * sizeof(struct rgui_file));
   }

   list->list[list->size].path = strdup(path);
   list->list[list->size].type = type;
   list->list[list->size].directory_ptr = directory_ptr;
   list->size++;
}

void rgui_list_pop(rgui_list_t *list, size_t *directory_ptr)
{
   if (!(list->size == 0))
      free(list->list[--list->size].path);

   if (directory_ptr)
      *directory_ptr = list->list[list->size].directory_ptr;
}

void rgui_list_free(rgui_list_t *list)
{
   for (size_t i = 0; i < list->size; i++)
      free(list->list[i].path);
   free(list->list);
   free(list);
}

void rgui_list_clear(rgui_list_t *list)
{
   for (size_t i = 0; i < list->size; i++)
      free(list->list[i].path);
   list->size = 0;
}

void rgui_list_get_at_offset(const rgui_list_t *list, size_t index,
      const char **path, unsigned *file_type)
{
   if (path)
      *path = list->list[index].path;
   if (file_type)
      *file_type = list->list[index].type;
}

void rgui_list_get_last(const rgui_list_t *list,
      const char **path, unsigned *file_type)
{
   if (list->size)
      rgui_list_get_at_offset(list, list->size - 1, path, file_type);
}

#endif

void menu_rom_history_push(const char *path,
      const char *core_path,
      const char *core_name)
{
   if (rgui->history)
      rom_history_push(rgui->history, path, core_path, core_name);
}

void menu_rom_history_push_current(void)
{
   // g_extern.fullpath can be relative here.
   // Ensure we're pushing absolute path.

   char tmp[PATH_MAX];

   // We loaded a zip, and fullpath points to the extracted file.
   // Look at basename instead.
   if (g_extern.rom_file_temporary)
      snprintf(tmp, sizeof(tmp), "%s.zip", g_extern.basename);
   else
      strlcpy(tmp, g_extern.fullpath, sizeof(tmp));

   if (*tmp)
      path_resolve_realpath(tmp, sizeof(tmp));

   menu_rom_history_push(*tmp ? tmp : NULL,
         g_settings.libretro,
         g_extern.system.info.library_name);
}

void load_menu_game_prepare(void)
{
   if (*g_extern.fullpath || rgui->load_no_rom)
   {
      if (*g_extern.fullpath &&
            g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
      {
         char tmp[PATH_MAX];
         char str[PATH_MAX];

         fill_pathname_base(tmp, g_extern.fullpath, sizeof(tmp));
         snprintf(str, sizeof(str), "INFO - Loading %s ...", tmp);
         msg_queue_push(g_extern.msg_queue, str, 1, 1);
      }

      menu_rom_history_push(*g_extern.fullpath ? g_extern.fullpath : NULL,
            g_settings.libretro,
            rgui->info.library_name ? rgui->info.library_name : "");
   }

#ifdef HAVE_RGUI
   // redraw RGUI frame
   rgui->old_input_state = rgui->trigger_state = 0;
   rgui->do_held = false;
   rgui->msg_force = true;
   rgui_iterate(rgui);
#endif

   // Draw frame for loading message
   if (driver.video_poke && driver.video_poke->set_texture_enable)
      driver.video_poke->set_texture_enable(driver.video_data, rgui->frame_buf_show, MENU_TEXTURE_FULLSCREEN);

   if (driver.video)
      rarch_render_cached_frame();

   if (driver.video_poke && driver.video_poke->set_texture_enable)
      driver.video_poke->set_texture_enable(driver.video_data, false,
            MENU_TEXTURE_FULLSCREEN);
}

void load_menu_game_history(unsigned game_index)
{
   const char *path = NULL;
   const char *core_path = NULL;
   const char *core_name = NULL;

   rom_history_get_index(rgui->history,
         game_index, &path, &core_path, &core_name);

   rarch_environment_cb(RETRO_ENVIRONMENT_SET_LIBRETRO_PATH, (void*)core_path);

   if (path)
      rgui->load_no_rom = false;
   else
      rgui->load_no_rom = true;

   rarch_environment_cb(RETRO_ENVIRONMENT_EXEC, (void*)path);

#if defined(HAVE_DYNAMIC)
   libretro_free_system_info(&rgui->info);
   libretro_get_system_info(g_settings.libretro, &rgui->info, NULL);
#endif
}

bool load_menu_game(void)
{
   if (g_extern.main_is_init)
      rarch_main_deinit();

   struct rarch_main_wrap args = {0};

   args.verbose       = g_extern.verbose;
   args.config_path   = *g_extern.config_path ? g_extern.config_path : NULL;
   args.sram_path     = *g_extern.savefile_dir ? g_extern.savefile_dir : NULL;
   args.state_path    = *g_extern.savestate_dir ? g_extern.savestate_dir : NULL;
   args.rom_path      = *g_extern.fullpath ? g_extern.fullpath : NULL;
   args.libretro_path = g_settings.libretro;
   args.no_rom        = rgui->load_no_rom;
   rgui->load_no_rom  = false;

   if (rarch_main_init_wrap(&args) == 0)
   {
      RARCH_LOG("rarch_main_init_wrap() succeeded.\n");
      return true;
   }
   else
   {
      char name[PATH_MAX];
      char msg[PATH_MAX];
      fill_pathname_base(name, g_extern.fullpath, sizeof(name));
      snprintf(msg, sizeof(msg), "Failed to load %s.\n", name);
      msg_queue_push(g_extern.msg_queue, msg, 1, 90);
      rgui->msg_force = true;
      RARCH_ERR("rarch_main_init_wrap() failed.\n");
      return false;
   }
}

void menu_init(void)
{
   rgui = rgui_init();

   if (rgui == NULL)
   {
      RARCH_ERR("Could not initialize menu.\n");
      rarch_fail(1, "menu_init()");
   }

   rgui->trigger_state = 0;
   rgui->old_input_state = 0;
   rgui->do_held = false;
   rgui->frame_buf_show = true;
   rgui->current_pad = 0;

#ifdef HAVE_DYNAMIC
   if (path_is_directory(g_settings.libretro))
      strlcpy(rgui->libretro_dir, g_settings.libretro, sizeof(rgui->libretro_dir));
   else if (*g_settings.libretro)
   {
      fill_pathname_basedir(rgui->libretro_dir, g_settings.libretro, sizeof(rgui->libretro_dir));
      libretro_get_system_info(g_settings.libretro, &rgui->info, NULL);
   }
#else
   retro_get_system_info(&rgui->info);
#endif

#ifdef HAVE_FILEBROWSER
   if (!(strlen(g_settings.rgui_browser_directory) > 0))
      strlcpy(g_settings.rgui_browser_directory, default_paths.filebrowser_startup_dir,
            sizeof(g_settings.rgui_browser_directory));

   rgui->browser =  (filebrowser_t*)calloc(1, sizeof(*(rgui->browser)));

   if (rgui->browser == NULL)
   {
      RARCH_ERR("Could not initialize filebrowser.\n");
      rarch_fail(1, "menu_init()");
   }

   strlcpy(rgui->browser->current_dir.extensions, rgui->info.valid_extensions,
         sizeof(rgui->browser->current_dir.extensions));

   // Look for zips to extract as well.
   if (*rgui->info.valid_extensions)
   {
      strlcat(rgui->browser->current_dir.extensions, "|zip",
         sizeof(rgui->browser->current_dir.extensions));
   }

   strlcpy(rgui->browser->current_dir.root_dir, g_settings.rgui_browser_directory,
         sizeof(rgui->browser->current_dir.root_dir));

   filebrowser_iterate(rgui->browser, FILEBROWSER_ACTION_RESET);
#endif

#ifdef HAVE_SHADER_MANAGER
   shader_manager_init(rgui);
#endif

   if (*g_extern.config_path)
   {
      char history_path[PATH_MAX];
      if (*g_settings.game_history_path)
         strlcpy(history_path, g_settings.game_history_path, sizeof(history_path));
      else
      {
         fill_pathname_resolve_relative(history_path, g_extern.config_path,
               ".retroarch-game-history.txt", sizeof(history_path));
      }

      RARCH_LOG("[RGUI]: Opening history: %s.\n", history_path);
      rgui->history = rom_history_init(history_path, g_settings.game_history_size);
   }

   rgui->last_time = rarch_get_time_usec();
}

void menu_free(void)
{
   rgui_free(rgui);

#ifdef HAVE_FILEBROWSER
   filebrowser_free(rgui->browser);
#endif

   rom_history_free(rgui->history);

   free(rgui);
}

void menu_ticker_line(char *buf, size_t len, unsigned index, const char *str, bool selected)
{
   size_t str_len = strlen(str);
   if (str_len <= len)
   {
      strlcpy(buf, str, len + 1);
      return;
   }

   if (!selected)
   {
      strlcpy(buf, str, len + 1 - 3);
      strlcat(buf, "...", len + 1);
   }
   else
   {
      // Wrap long strings in options with some kind of ticker line.
      unsigned ticker_period = 2 * (str_len - len) + 4;
      unsigned phase = index % ticker_period;

      unsigned phase_left_stop = 2;
      unsigned phase_left_moving = phase_left_stop + (str_len - len);
      unsigned phase_right_stop = phase_left_moving + 2;

      unsigned left_offset = phase - phase_left_stop;
      unsigned right_offset = (str_len - len) - (phase - phase_right_stop);

      // Ticker period: [Wait at left (2 ticks), Progress to right (type_len - w), Wait at right (2 ticks), Progress to left].
      if (phase < phase_left_stop)
         strlcpy(buf, str, len + 1);
      else if (phase < phase_left_moving)
         strlcpy(buf, str + left_offset, len + 1);
      else if (phase < phase_right_stop)
         strlcpy(buf, str + str_len - len, len + 1);
      else
         strlcpy(buf, str + right_offset, len + 1);
   }
}

#ifndef HAVE_RMENU_XUI
#if defined(HAVE_RMENU) || defined(HAVE_RGUI)

static const struct retro_keybind _menu_nav_binds[] = {
#if defined(HW_RVL)
   { 0, 0, NULL, 0, GX_GC_UP | GX_GC_LSTICK_UP | GX_GC_RSTICK_UP | GX_CLASSIC_UP | GX_CLASSIC_LSTICK_UP | GX_CLASSIC_RSTICK_UP | GX_WIIMOTE_UP | GX_NUNCHUK_UP, 0 },
   { 0, 0, NULL, 0, GX_GC_DOWN | GX_GC_LSTICK_DOWN | GX_GC_RSTICK_DOWN | GX_CLASSIC_DOWN | GX_CLASSIC_LSTICK_DOWN | GX_CLASSIC_RSTICK_DOWN | GX_WIIMOTE_DOWN | GX_NUNCHUK_DOWN, 0 },
   { 0, 0, NULL, 0, GX_GC_LEFT | GX_GC_LSTICK_LEFT | GX_GC_RSTICK_LEFT | GX_CLASSIC_LEFT | GX_CLASSIC_LSTICK_LEFT | GX_CLASSIC_RSTICK_LEFT | GX_WIIMOTE_LEFT | GX_NUNCHUK_LEFT, 0 },
   { 0, 0, NULL, 0, GX_GC_RIGHT | GX_GC_LSTICK_RIGHT | GX_GC_RSTICK_RIGHT | GX_CLASSIC_RIGHT | GX_CLASSIC_LSTICK_RIGHT | GX_CLASSIC_RSTICK_RIGHT | GX_WIIMOTE_RIGHT | GX_NUNCHUK_RIGHT, 0 },
   { 0, 0, NULL, 0, GX_GC_A | GX_CLASSIC_A | GX_WIIMOTE_A | GX_WIIMOTE_2, 0 },
   { 0, 0, NULL, 0, GX_GC_B | GX_CLASSIC_B | GX_WIIMOTE_B | GX_WIIMOTE_1, 0 },
   { 0, 0, NULL, 0, GX_GC_START | GX_CLASSIC_PLUS | GX_WIIMOTE_PLUS, 0 },
   { 0, 0, NULL, 0, GX_GC_Z_TRIGGER | GX_CLASSIC_MINUS | GX_WIIMOTE_MINUS, 0 },
   { 0, 0, NULL, 0, GX_WIIMOTE_HOME | GX_CLASSIC_HOME, 0 },
#elif defined(HW_DOL)
   { 0, 0, NULL, 0, GX_GC_UP | GX_GC_LSTICK_UP | GX_GC_RSTICK_UP, 0 },
   { 0, 0, NULL, 0, GX_GC_DOWN | GX_GC_LSTICK_DOWN | GX_GC_RSTICK_DOWN, 0 },
   { 0, 0, NULL, 0, GX_GC_LEFT | GX_GC_LSTICK_LEFT | GX_GC_RSTICK_LEFT, 0 },
   { 0, 0, NULL, 0, GX_GC_RIGHT | GX_GC_LSTICK_RIGHT | GX_GC_RSTICK_RIGHT, 0 },
   { 0, 0, NULL, 0, GX_GC_A, 0 },
   { 0, 0, NULL, 0, GX_GC_B, 0 },
   { 0, 0, NULL, 0, GX_GC_START, 0 },
   { 0, 0, NULL, 0, GX_GC_Z_TRIGGER, 0 },
   { 0, 0, NULL, 0, GX_WIIMOTE_HOME, 0 },
#elif defined(__CELLOS_LV2__) || defined(_XBOX1)
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_UP) | (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_UP), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN) | (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_DOWN), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT) | (1ULL << RARCH_ANALOG_LEFT_X_DPAD_LEFT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT) | (1ULL << RARCH_ANALOG_LEFT_X_DPAD_RIGHT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_UP), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_DOWN), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_LEFT_X_DPAD_LEFT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_LEFT_X_DPAD_RIGHT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_UP), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_DOWN), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_LEFT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_RIGHT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_B), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_A), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_X), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_Y), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_START), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_SELECT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_L), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_R), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_L2), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_R2), 0 },
#else
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_UP), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_A), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_B), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_START), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_SELECT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_MENU_TOGGLE), 0 },
#endif
};

#if defined(RARCH_CONSOLE) || defined(ANDROID)
static const struct retro_keybind *menu_nav_binds[] = {
   _menu_nav_binds
};
#endif

uint64_t rgui_input(void)
{
   uint64_t input_state = 0;

   // FIXME: Very ugly. Should do something more uniform.
#if defined(RARCH_CONSOLE) || defined(ANDROID)
   for (unsigned i = 0; i < DEVICE_NAV_LAST; i++)
      input_state |= driver.input->input_state(driver.input_data, menu_nav_binds, 0,
            RETRO_DEVICE_JOYPAD, 0, i) ? (1ULL << i) : 0;

   input_state |= driver.input->key_pressed(driver.input_data, RARCH_MENU_TOGGLE) ? (1ULL << DEVICE_NAV_MENU) : 0;

#ifdef HAVE_OVERLAY
   for (unsigned i = 0; i < DEVICE_NAV_LAST; i++)
      input_state |= driver.overlay_state & menu_nav_binds[0][i].joykey ? (1ULL << i) : 0;
#endif
#else
   static const int maps[] = {
      RETRO_DEVICE_ID_JOYPAD_UP,     DEVICE_NAV_UP,
      RETRO_DEVICE_ID_JOYPAD_DOWN,   DEVICE_NAV_DOWN,
      RETRO_DEVICE_ID_JOYPAD_LEFT,   DEVICE_NAV_LEFT,
      RETRO_DEVICE_ID_JOYPAD_RIGHT,  DEVICE_NAV_RIGHT,
      RETRO_DEVICE_ID_JOYPAD_A,      DEVICE_NAV_A,
      RETRO_DEVICE_ID_JOYPAD_B,      DEVICE_NAV_B,
      RETRO_DEVICE_ID_JOYPAD_START,  DEVICE_NAV_START,
      RETRO_DEVICE_ID_JOYPAD_SELECT, DEVICE_NAV_SELECT,
   };

   static const struct retro_keybind *binds[] = { g_settings.input.binds[0] };

   for (unsigned i = 0; i < ARRAY_SIZE(maps); i += 2)
   {
      input_state |= input_input_state_func(binds,
            0, RETRO_DEVICE_JOYPAD, 0, maps[i + 0]) ? (1ULL << maps[i + 1]) : 0;
#ifdef HAVE_OVERLAY
      input_state |= (driver.overlay_state & (UINT64_C(1) << maps[i + 0])) ? (1ULL << maps[i + 1]) : 0;
#endif
   }

   input_state |= input_key_pressed_func(RARCH_MENU_TOGGLE) ? (1ULL << DEVICE_NAV_MENU) : 0;
#endif

   rgui->trigger_state = input_state & ~rgui->old_input_state;

#if defined(HAVE_RGUI)
   rgui->do_held = (input_state & (
         (1ULL << DEVICE_NAV_UP) |
         (1ULL << DEVICE_NAV_DOWN) |
         (1ULL << DEVICE_NAV_LEFT) | 
         (1ULL << DEVICE_NAV_RIGHT))) &&
      !(input_state & (1ULL << DEVICE_NAV_MENU));
#elif defined(HAVE_RMENU)
   rgui->do_held = (input_state & (
            (1ULL << DEVICE_NAV_LEFT) |
            (1ULL << DEVICE_NAV_RIGHT) |
            (1ULL << DEVICE_NAV_UP) |
            (1ULL << DEVICE_NAV_DOWN) |
            (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_UP) |
            (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_DOWN) |
            (1ULL << RARCH_ANALOG_LEFT_X_DPAD_LEFT) |
            (1ULL << RARCH_ANALOG_LEFT_X_DPAD_RIGHT) |
            (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_UP) |
            (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_DOWN) |
            (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_LEFT) |
            (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_RIGHT) |
            (1ULL << DEVICE_NAV_L2) |
            (1ULL << DEVICE_NAV_R2)
            )) && !(input_state & (1ULL << DEVICE_NAV_MENU));
#endif

   return input_state;
}

bool menu_iterate(void)
{
   rarch_time_t time, delta, target_msec, sleep_msec;
   static bool initial_held = true;
   static bool first_held = false;
   uint64_t input_state = 0;
   int input_entry_ret;

   if (g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_PREINIT))
   {
      rgui->need_refresh = true;
      g_extern.lifecycle_mode_state &= ~(1ULL << MODE_MENU_PREINIT);
      rgui->old_input_state |= 1ULL << DEVICE_NAV_MENU;
   }

   rarch_input_poll();
#ifdef HAVE_OVERLAY
   rarch_check_overlay();
#endif

   if (input_key_pressed_func(RARCH_QUIT_KEY) || !video_alive_func())
   {
      g_extern.lifecycle_mode_state |= (1ULL << MODE_GAME);
      goto deinit;
   }

   input_state = rgui_input();

   if (rgui->do_held)
   {
      if (!first_held)
      {
         first_held = true;
         rgui->delay_timer = initial_held ? 12 : 6;
         rgui->delay_count = 0;
      }

      if (rgui->delay_count >= rgui->delay_timer)
      {
         first_held = false;
         rgui->trigger_state = input_state;
      }

      initial_held = false;
   }
   else
   {
      first_held = false;
      initial_held = true;
   }

   rgui->delay_count++;
   rgui->old_input_state = input_state;
   input_entry_ret = rgui_iterate(rgui);

   if (driver.video_poke && driver.video_poke->set_texture_enable)
      driver.video_poke->set_texture_enable(driver.video_data, rgui->frame_buf_show, MENU_TEXTURE_FULLSCREEN);

   rarch_render_cached_frame();

   // Throttle in case VSync is broken (avoid 1000+ FPS RGUI).
   time = rarch_get_time_usec();
   delta = (time - rgui->last_time) / 1000;
   target_msec = 750 / g_settings.video.refresh_rate; // Try to sleep less, so we can hopefully rely on FPS logger.
   sleep_msec = target_msec - delta;
   if (sleep_msec > 0)
      rarch_sleep(sleep_msec);
   rgui->last_time = rarch_get_time_usec();

   if (driver.video_poke && driver.video_poke->set_texture_enable)
      driver.video_poke->set_texture_enable(driver.video_data, false,
            MENU_TEXTURE_FULLSCREEN);

   if (rgui_input_postprocess(rgui, rgui->old_input_state) || input_entry_ret)
      goto deinit;

   return true;

deinit:
   return false;
}
#endif
#endif
