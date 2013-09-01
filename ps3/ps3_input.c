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
#include <stdlib.h>

#ifndef __PSL1GHT__
#include <sdk_version.h>
#endif

#include <sys/memory.h>

#include "sdk_defines.h"

#include "ps3_input.h"
#include "../driver.h"
#include "../libretro.h"
#include "../general.h"

/*============================================================
  PS3 MOUSE
  ============================================================ */

#ifdef HAVE_MOUSE

#ifndef __PSL1GHT__
#define MAX_MICE 7
#endif

static void ps3_mouse_input_deinit(void)
{
   cellMouseEnd();
}

static uint32_t ps3_mouse_input_mice_connected(void)
{
   CellMouseInfo mouse_info;
   cellMouseGetInfo(&mouse_info);
   return mouse_info.now_connect;
}

CellMouseData ps3_mouse_input_poll_device(uint32_t id)
{
   CellMouseData mouse_data;

   // Get new pad data
   cellMouseGetData(id, &mouse_data);

   return mouse_data;
}

#endif

/*============================================================
  PS3 PAD
  ============================================================ */

const struct platform_bind platform_keys[] = {
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_B), "Cross button" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_Y), "Square button" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_SELECT), "Select button" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_START), "Start button" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_UP), "D-Pad Up" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN), "D-Pad Down" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT), "D-Pad Left" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT), "D-Pad Right" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_A), "Circle button" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_X), "Triangle button" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_L), "L1 button" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_R), "R1 button" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_L2), "L2 button" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_R2), "R2 button" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_L3), "L3 button" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_R3), "R3 button" },
   { (1ULL << RARCH_TURBO_ENABLE), "Turbo button (Unmapped)" },
   { (1ULL << RARCH_ANALOG_LEFT_X_PLUS), "LStick Left" },
   { (1ULL << RARCH_ANALOG_LEFT_X_MINUS), "LStick Right" },
   { (1ULL << RARCH_ANALOG_LEFT_Y_PLUS), "LStick Up" },
   { (1ULL << RARCH_ANALOG_LEFT_Y_MINUS), "LStick Down" },
   { (1ULL << RARCH_ANALOG_RIGHT_X_PLUS), "RStick Left" },
   { (1ULL << RARCH_ANALOG_RIGHT_X_MINUS), "RStick Right" },
   { (1ULL << RARCH_ANALOG_RIGHT_Y_PLUS), "RStick Up" },
   { (1ULL << RARCH_ANALOG_RIGHT_Y_MINUS), "RStick Down" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT) | (1ULL << RARCH_ANALOG_LEFT_X_DPAD_LEFT), "LStick D-Pad Left" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT) | (1ULL << RARCH_ANALOG_LEFT_X_DPAD_RIGHT), "LStick D-Pad Right" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_UP) | (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_UP), "LStick D-Pad Up" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN) | (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_DOWN), "LStick D-Pad Down" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT) | (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_LEFT), "RStick D-Pad Left" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT) | (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_RIGHT), "RStick D-Pad Right" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_UP) | (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_UP), "RStick D-Pad Up" },
   { (1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN) | (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_DOWN), "RStick D-Pad Down" },
};

static uint64_t state[MAX_PADS];
static unsigned pads_connected;
#ifdef HAVE_MOUSE
static unsigned mice_connected;
#endif

static void ps3_input_poll(void *data)
{
   CellPadInfo2 pad_info;
   (void)data;

   for (unsigned i = 0; i < MAX_PADS; i++)
   {
      static CellPadData state_tmp;
      cellPadGetData(i, &state_tmp);

      if (state_tmp.len != 0)
      {
         uint64_t *state_cur = &state[i];
         bool dpad_emulation = (g_settings.input.dpad_emulation[i] != ANALOG_DPAD_NONE);
         *state_cur = 0;
#ifdef __PSL1GHT__
         *state_cur |= (state_tmp.BTN_LEFT)     ? (1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT) : 0;
         *state_cur |= (state_tmp.BTN_DOWN)     ? (1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN) : 0;
         *state_cur |= (state_tmp.BTN_RIGHT)    ? (1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT) : 0;
         *state_cur |= (state_tmp.BTN_UP)       ? (1ULL << RETRO_DEVICE_ID_JOYPAD_UP) : 0;
         *state_cur |= (state_tmp.BTN_START)    ? (1ULL << RETRO_DEVICE_ID_JOYPAD_START) : 0;
         *state_cur |= (state_tmp.BTN_R3)       ? (1ULL << RETRO_DEVICE_ID_JOYPAD_R3) : 0;
         *state_cur |= (state_tmp.BTN_L3)       ? (1ULL << RETRO_DEVICE_ID_JOYPAD_L3) : 0;
         *state_cur |= (state_tmp.BTN_SELECT)   ? (1ULL << RETRO_DEVICE_ID_JOYPAD_SELECT) : 0;
         *state_cur |= (state_tmp.BTN_TRIANGLE) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_X) : 0;
         *state_cur |= (state_tmp.BTN_SQUARE)   ? (1ULL << RETRO_DEVICE_ID_JOYPAD_Y) : 0;
         *state_cur |= (state_tmp.BTN_CROSS)    ? (1ULL << RETRO_DEVICE_ID_JOYPAD_B) : 0;
         *state_cur |= (state_tmp.BTN_CIRCLE)   ? (1ULL << RETRO_DEVICE_ID_JOYPAD_A) : 0;
         *state_cur |= (state_tmp.BTN_R1)       ? (1ULL << RETRO_DEVICE_ID_JOYPAD_R) : 0;
         *state_cur |= (state_tmp.BTN_L1)       ? (1ULL << RETRO_DEVICE_ID_JOYPAD_L) : 0;
         *state_cur |= (state_tmp.BTN_R2)       ? (1ULL << RETRO_DEVICE_ID_JOYPAD_R2) : 0;
         *state_cur |= (state_tmp.BTN_L2)       ? (1ULL << RETRO_DEVICE_ID_JOYPAD_L2) : 0;

         if (dpad_emulation)
         {
            *state_cur |= (state_tmp.ANA_L_H <= DEADZONE_LOW) ? (1ULL << RARCH_ANALOG_LEFT_X_DPAD_LEFT) : 0;
            *state_cur |= (state_tmp.ANA_L_H >= DEADZONE_HIGH) ? (1ULL << RARCH_ANALOG_LEFT_X_DPAD_RIGHT) : 0;
            *state_cur |= (state_tmp.ANA_L_V <= DEADZONE_LOW) ? (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_UP) : 0;
            *state_cur |= (state_tmp.ANA_L_V >= DEADZONE_HIGH) ? (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_DOWN) : 0;
            *state_cur |= (state_tmp.ANA_R_H <= DEADZONE_LOW) ? (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_LEFT) : 0;
            *state_cur |= (state_tmp.ANA_R_H >= DEADZONE_HIGH) ? (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_RIGHT) : 0;
            *state_cur |= (state_tmp.ANA_R_V <= DEADZONE_LOW) ? (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_UP) : 0;
            *state_cur |= (state_tmp.ANA_R_V >= DEADZONE_HIGH) ? (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_DOWN) : 0;
         }
#else
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_LEFT) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_DOWN) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_RIGHT) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_UP) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_UP) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_START) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_START) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_R3) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_R3) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_L3) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_SELECT) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_SELECT) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_TRIANGLE) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_X) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_SQUARE) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_Y) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CROSS) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_B) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_A) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R1) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_R) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L1) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_L) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_R2) : 0;
         *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_L2) : 0;
         if (dpad_emulation)
         {
            *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X] <= DEADZONE_LOW) ? (1ULL << RARCH_ANALOG_LEFT_X_DPAD_LEFT) : 0;
            *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X] >= DEADZONE_HIGH) ? (1ULL << RARCH_ANALOG_LEFT_X_DPAD_RIGHT) : 0;
            *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y] <= DEADZONE_LOW) ? (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_UP) : 0;
            *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y] >= DEADZONE_HIGH) ? (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_DOWN) : 0;
            *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] <= DEADZONE_LOW) ? (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_LEFT) : 0;
            *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] >= DEADZONE_HIGH) ? (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_RIGHT) : 0;
            *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] <= DEADZONE_LOW) ? (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_UP) : 0;
            *state_cur |= (state_tmp.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] >= DEADZONE_HIGH) ? (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_DOWN) : 0;
         }
#endif
      }
   }

   uint64_t *state_p1 = &state[0];
   uint64_t *lifecycle_state = &g_extern.lifecycle_state;
   bool dpad_emulation = (g_settings.input.dpad_emulation[0] != ANALOG_DPAD_NONE);

   *lifecycle_state &= ~(
         (1ULL << RARCH_FAST_FORWARD_HOLD_KEY) | 
         (1ULL << RARCH_LOAD_STATE_KEY) | 
         (1ULL << RARCH_SAVE_STATE_KEY) | 
         (1ULL << RARCH_STATE_SLOT_PLUS) | 
         (1ULL << RARCH_STATE_SLOT_MINUS) | 
         (1ULL << RARCH_REWIND) |
         (1ULL << RARCH_QUIT_KEY) |
         (1ULL << RARCH_MENU_TOGGLE));

   if (dpad_emulation)
   {
      if ((*state_p1 & (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_DOWN)) && !(*state_p1 & (1ULL << RETRO_DEVICE_ID_JOYPAD_R2)))
         *lifecycle_state |= (1ULL << RARCH_FAST_FORWARD_HOLD_KEY);
      if ((*state_p1 & (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_UP)) && (*state_p1 & (1ULL << RETRO_DEVICE_ID_JOYPAD_R2)))
         *lifecycle_state |= (1ULL << RARCH_LOAD_STATE_KEY);
      if ((*state_p1 & (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_DOWN)) && (*state_p1 & (1ULL << RETRO_DEVICE_ID_JOYPAD_R2)))
         *lifecycle_state |= (1ULL << RARCH_SAVE_STATE_KEY);
      if ((*state_p1 & (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_RIGHT)) && (*state_p1 & (1ULL << RETRO_DEVICE_ID_JOYPAD_R2)))
         *lifecycle_state |= (1ULL << RARCH_STATE_SLOT_PLUS);
      if ((*state_p1 & (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_LEFT)) && (*state_p1 & (1ULL << RETRO_DEVICE_ID_JOYPAD_R2)))
         *lifecycle_state |= (1ULL << RARCH_STATE_SLOT_MINUS);
      if ((*state_p1 & (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_UP)) && !(*state_p1 & (1ULL << RETRO_DEVICE_ID_JOYPAD_R2)))
         *lifecycle_state |= (1ULL << RARCH_REWIND);
   }

   if ((*state_p1 & (1ULL << RETRO_DEVICE_ID_JOYPAD_L3)) && (*state_p1 & (1ULL << RETRO_DEVICE_ID_JOYPAD_R3)))
      *lifecycle_state |= (1ULL << RARCH_MENU_TOGGLE);

   cellPadGetInfo2(&pad_info);
   pads_connected = pad_info.now_connect; 
#ifdef HAVE_MOUSE
   mice_connected = ps3_mouse_input_mice_connected();
#endif
}

#ifdef HAVE_MOUSE

static int16_t ps3_mouse_device_state(void *data, unsigned player, unsigned id)
{
   CellMouseData mouse_state = ps3_mouse_input_poll_device(player);

   switch (id)
   {
      case RETRO_DEVICE_ID_MOUSE_LEFT:
         return (!mice_connected ? 0 : mouse_state.buttons & CELL_MOUSE_BUTTON_1);
      case RETRO_DEVICE_ID_MOUSE_RIGHT:
         return (!mice_connected ? 0 : mouse_state.buttons & CELL_MOUSE_BUTTON_2);
      case RETRO_DEVICE_ID_MOUSE_X:
         return (!mice_connected ? 0 : mouse_state.x_axis);
      case RETRO_DEVICE_ID_MOUSE_Y:
         return (!mice_connected ? 0 : mouse_state.y_axis);
      default:
         return 0;
   }
}

#endif

static int16_t ps3_input_state(void *data, const struct retro_keybind **binds,
      unsigned port, unsigned device,
      unsigned index, unsigned id)
{
   (void)data;

   unsigned player = port;
   uint64_t button = binds[player][id].joykey;
   int16_t retval = 0;

   if (player < pads_connected)
   {
      switch (device)
      {
         case RETRO_DEVICE_JOYPAD:
            retval = (state[player] & button) ? 1 : 0;
            break;
#ifdef HAVE_MOUSE
         case RETRO_DEVICE_MOUSE:
            retval = ps3_mouse_device_state(data, player, id);
            break;
#endif
      }
   }

   return retval;
}

/*============================================================
  ON-SCREEN KEYBOARD UTILITY
  ============================================================ */

#ifdef HAVE_OSKUTIL

#define OSK_IN_USE 1

void oskutil_init(oskutil_params *params, unsigned containersize)
{
   params->flags = 0;
   if (containersize)
      params->osk_memorycontainer =  containersize; 
   else
      params->osk_memorycontainer =  1024*1024*2;
}

static bool oskutil_enable_key_layout (void)
{
   int ret = pOskSetKeyLayoutOption(CELL_OSKDIALOG_10KEY_PANEL | \
         CELL_OSKDIALOG_FULLKEY_PANEL);
   if (ret < 0)
      return (false);
   else
      return (true);
}

static void oskutil_create_activation_parameters(oskutil_params *params)
{
   params->dialogParam.controlPoint.x = 0.0;
   params->dialogParam.controlPoint.y = 0.0;

   int32_t LayoutMode = CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_CENTER | CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_TOP;
   pOskSetLayoutMode(LayoutMode);

   params->dialogParam.osk_allowed_panels = 
      CELL_OSKDIALOG_PANELMODE_ALPHABET |
      CELL_OSKDIALOG_PANELMODE_NUMERAL | 
      CELL_OSKDIALOG_PANELMODE_NUMERAL_FULL_WIDTH |
      CELL_OSKDIALOG_PANELMODE_ENGLISH;

   params->dialogParam.firstViewPanel = CELL_OSKDIALOG_PANELMODE_ENGLISH;
   params->dialogParam.osk_prohibit_flags = 0;
}

void oskutil_write_message(oskutil_params *params, const wchar_t* msg)
{
   params->inputFieldInfo.osk_inputfield_message = (uint16_t*)msg;
}

void oskutil_write_initial_message(oskutil_params *params, const wchar_t* msg)
{
   params->inputFieldInfo.osk_inputfield_starttext = (uint16_t*)msg;
}

bool oskutil_start(oskutil_params *params) 
{
   if (params->flags & OSK_IN_USE)
   {
      RARCH_WARN("OSK util already initialized and in use\n");
      return true;
   }

   int ret = 0;

   ret = sys_memory_container_create(&params->containerid, params->osk_memorycontainer);

   if (ret < 0)
      goto do_deinit;

   params->outputInfo.osk_callback_return_param = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
   params->outputInfo.osk_callback_num_chars = 256;
   params->outputInfo.osk_callback_return_string = (uint16_t *)params->text_buf;

   memset(params->text_buf, 0, sizeof(*params->text_buf));

   params->inputFieldInfo.osk_inputfield_max_length = CELL_OSKDIALOG_STRING_SIZE;	

   oskutil_create_activation_parameters(params);

   if (!oskutil_enable_key_layout())
      return (false);

   ret = pOskLoadAsync(params->containerid, &params->dialogParam, &params->inputFieldInfo);

   if (ret < 0)
      goto do_deinit;

   params->flags |= OSK_IN_USE;

   return true;

do_deinit:
   RARCH_ERR("Could not properly initialize OSK util.\n");
   return false;
}
#endif

/*============================================================
  RetroArch PS3 INPUT DRIVER 
  ============================================================ */

static void ps3_input_free_input(void *data)
{
   (void)data;
   //cellPadEnd();
}

static void ps3_input_set_keybinds(void *data, unsigned device,
      unsigned port, unsigned id, unsigned keybind_action)
{
   uint64_t *key = &g_settings.input.binds[port][id].joykey;
   uint64_t joykey = *key;
   size_t arr_size = sizeof(platform_keys) / sizeof(platform_keys[0]);

   (void)device;

   if (keybind_action & (1ULL << KEYBINDS_ACTION_DECREMENT_BIND))
   {
      if (joykey == NO_BTN)
         *key = platform_keys[arr_size - 1].joykey;
      else if (platform_keys[0].joykey == joykey)
         *key = NO_BTN;
      else
      {
         *key = NO_BTN;
         for (size_t i = 1; i < arr_size; i++)
         {
            if (platform_keys[i].joykey == joykey)
            {
               *key = platform_keys[i - 1].joykey;
               break;
            }
         }
      }
   }

   if (keybind_action & (1ULL << KEYBINDS_ACTION_INCREMENT_BIND))
   {
      if (joykey == NO_BTN)
         *key = platform_keys[0].joykey;
      else if (platform_keys[arr_size - 1].joykey == joykey)
         *key = NO_BTN;
      else
      {
         *key = NO_BTN;
         for (size_t i = 0; i < arr_size - 1; i++)
         {
            if (platform_keys[i].joykey == joykey)
            {
               *key = platform_keys[i + 1].joykey;
               break;
            }
         }
      }
   }

   if (keybind_action & (1ULL << KEYBINDS_ACTION_SET_DEFAULT_BIND))
      *key = g_settings.input.binds[port][id].def_joykey;

   if (keybind_action & (1ULL << KEYBINDS_ACTION_SET_DEFAULT_BINDS))
   {
      for (int i = 0; i < RARCH_CUSTOM_BIND_LIST_END; i++)
      {
         g_settings.input.binds[port][i].id = i;
         g_settings.input.binds[port][i].def_joykey = platform_keys[i].joykey;
         g_settings.input.binds[port][i].joykey = g_settings.input.binds[port][i].def_joykey;
      }
      g_settings.input.dpad_emulation[port] = ANALOG_DPAD_LSTICK;
   }
   
   if (keybind_action & (1ULL << KEYBINDS_ACTION_SET_ANALOG_DPAD_NONE))
   {
      g_settings.input.dpad_emulation[port] = ANALOG_DPAD_NONE;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_UP].joykey	= platform_keys[RETRO_DEVICE_ID_JOYPAD_UP].joykey;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_DOWN].joykey	= platform_keys[RETRO_DEVICE_ID_JOYPAD_DOWN].joykey;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_LEFT].joykey	= platform_keys[RETRO_DEVICE_ID_JOYPAD_LEFT].joykey;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_RIGHT].joykey	= platform_keys[RETRO_DEVICE_ID_JOYPAD_RIGHT].joykey;
   }

   if (keybind_action & (1ULL << KEYBINDS_ACTION_SET_ANALOG_DPAD_LSTICK))
   {
      g_settings.input.dpad_emulation[port] = ANALOG_DPAD_LSTICK;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_UP].joykey	= platform_keys[RARCH_ANALOG_LEFT_Y_DPAD_UP].joykey;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_DOWN].joykey	= platform_keys[RARCH_ANALOG_LEFT_Y_DPAD_DOWN].joykey;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_LEFT].joykey	= platform_keys[RARCH_ANALOG_LEFT_X_DPAD_LEFT].joykey;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_RIGHT].joykey	= platform_keys[RARCH_ANALOG_LEFT_X_DPAD_RIGHT].joykey;
   }

   if (keybind_action & (1ULL << KEYBINDS_ACTION_SET_ANALOG_DPAD_RSTICK))
   {
      g_settings.input.dpad_emulation[port] = ANALOG_DPAD_RSTICK;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_UP].joykey	= platform_keys[RARCH_ANALOG_RIGHT_Y_DPAD_UP].joykey;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_DOWN].joykey	= platform_keys[RARCH_ANALOG_RIGHT_Y_DPAD_DOWN].joykey;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_LEFT].joykey	= platform_keys[RARCH_ANALOG_RIGHT_X_DPAD_LEFT].joykey;
      g_settings.input.binds[port][RETRO_DEVICE_ID_JOYPAD_RIGHT].joykey	= platform_keys[RARCH_ANALOG_RIGHT_X_DPAD_RIGHT].joykey;
   }

   if (keybind_action & (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL))
   {
      struct platform_bind *ret = (struct platform_bind*)data;

      if (ret->joykey == NO_BTN)
         strlcpy(ret->desc, "No button", sizeof(ret->desc));
      else
      {
         for (size_t i = 0; i < arr_size; i++)
         {
            if (platform_keys[i].joykey == ret->joykey)
            {
               strlcpy(ret->desc, platform_keys[i].desc, sizeof(ret->desc));
               return;
            }
         }
         strlcpy(ret->desc, "Unknown", sizeof(ret->desc));
      }
   }
}

static void* ps3_input_init(void)
{
   cellPadInit(MAX_PADS);
#ifdef HAVE_MOUSE
   cellMouseInit(MAX_MICE);
#endif

   for(unsigned i = 0; i < MAX_PLAYERS; i++)
      if (driver.input->set_keybinds)
         driver.input->set_keybinds(driver.input_data, 0, i, 0,
               (1ULL << KEYBINDS_ACTION_SET_DEFAULT_BINDS));

   for(unsigned i = 0; i < MAX_PADS; i++)
   {
      unsigned keybind_action = 0;

      switch (g_settings.input.dpad_emulation[i])
      {
         case ANALOG_DPAD_LSTICK:
            keybind_action = (1ULL << KEYBINDS_ACTION_SET_ANALOG_DPAD_LSTICK);
            break;
         case ANALOG_DPAD_RSTICK:
            keybind_action = (1ULL << KEYBINDS_ACTION_SET_ANALOG_DPAD_RSTICK);
            break;
         case ANALOG_DPAD_NONE:
            keybind_action = (1ULL << KEYBINDS_ACTION_SET_ANALOG_DPAD_NONE);
            break;
         default:
            break;
      }

      if (keybind_action)
         if (driver.input->set_keybinds)
            driver.input->set_keybinds(driver.input_data, 0, i, 0,
                  keybind_action);
   }

   return (void*)-1;
}

static bool ps3_input_key_pressed(void *data, int key)
{
   return (g_extern.lifecycle_state & (1ULL << key));
}

const input_driver_t input_ps3 = {
   .init = ps3_input_init,
   .poll = ps3_input_poll,
   .input_state = ps3_input_state,
   .key_pressed = ps3_input_key_pressed,
   .free = ps3_input_free_input,
   .set_keybinds = ps3_input_set_keybinds,
   .ident = "ps3",
};

