#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <retro_inline.h>

#ifndef HAVE_NO_LANGEXTRA
#include "libretro_core_options_intl.h"
#endif

/*
 ********************************
 * VERSION: 2.0
 ********************************
 *
 * - 2.0: Add support for core options v2 interface
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition */

//struct retro_core_option_v2_category option_cats_us[] = {
//   { NULL, NULL, NULL },
//};

struct retro_core_option_v2_category option_cats_us[] = {
   {
      "audio",
      "Audio",
      "Configure pokey driver used and filter settings."
   },
   {
      "input",
      "Input",
      "Configure controller hack, Digital and Analog Joystick settings."
   },
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {
   {
      "a5200_bios",
      "BIOS (Restart)",
      NULL,
      "Set BIOS image. 'Official' loads an original console dump provided by the user, named '5200.rom' and placed in the frontend 'System' directory. 'Internal (Altirra OS)' uses an inbuilt open-source alternative with reduced compatibility. The 'Official' BIOS is recommended for correct operation.",
      NULL,
      NULL,
      {
         { "official", "Official" },
         { "internal", "Internal (Altirra OS)" },
         { NULL, NULL },
      },
      "official"
   },
   {
      "a5200_mix_frames",
      "Interframe Blending",
      NULL,
      "Simulate CRT phosphor ghosting effects. 'Simple' performs a 50:50 mix of the current and previous frames. 'Ghosting' accumulates pixels from multiple successive frames. May be used to alleviate screen flicker.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "mix",      "Simple" },
         { "ghost_65", "Ghosting (65%)" },
         { "ghost_75", "Ghosting (75%)" },
         { "ghost_85", "Ghosting (85%)" },
         { "ghost_95", "Ghosting (95%)" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "a5200_artifacting_mode",
      "Hi-Res Artifacting Mode",
      NULL,
      "Set Hi-Res Artifacting mode used.  Typically dependant on the actual emulated system.  Pick the color combination that pleases you.  None disables artifacting.  Good for games like A.E., Backgammon, Miniature Golf and several Atari 800 to Atari 5200 conversions ",
      NULL,
      NULL,
      {
         { "none", "None"},
         { "blue/brown 1", NULL },
         { "blue/brown 2",  NULL },
         { "GTIA",  NULL },
         { "CTIA",  NULL },
         { NULL, NULL },
      },
      "none"
   },
   {
      "a5200_enable_new_pokey",
      "High Fidelity POKEY (Restart)",
      NULL,
      "Enable High Fidelity Pokey for better quality sound.  Disable for games that use digital sound (Berzerk)",
      NULL,
      "audio",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "a5200_low_pass_filter",
      "Audio Filter",
      NULL,
      "Enable a low pass audio filter to soften the 'harsh' sound produced by the Atari 5200's POKEY chip.",
      NULL,
      "audio",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "a5200_low_pass_range",
      "Audio Filter Level",
      NULL,
      "Specify the cut-off frequency of the low pass audio filter. A higher value increases the perceived 'strength' of the filter, since a wider range of the high frequency spectrum is attenuated.",
      NULL,
      "audio",
      {
         { "5",  "5%" },
         { "10", "10%" },
         { "15", "15%" },
         { "20", "20%" },
         { "25", "25%" },
         { "30", "30%" },
         { "35", "35%" },
         { "40", "40%" },
         { "45", "45%" },
         { "50", "50%" },
         { "55", "55%" },
         { "60", "60%" },
         { "65", "65%" },
         { "70", "70%" },
         { "75", "75%" },
         { "80", "80%" },
         { "85", "85%" },
         { "90", "90%" },
         { "95", "95%" },
         { NULL, NULL },
      },
      "60"
   },
   {
      "a5200_input_hack",
      "Controller Hacks",
      NULL,
      "Apply gamepad input hacks required for specific games. 'Dual Stick' maps Player 2's joystick to the right analog stick of Player 1's RetroPad, enabling dual stick control in 'Robotron 2084' and 'Space Dungeon'. 'Swap Ports' maps Player 1 to port 2 and Player 2 to port 1 of the emulated console, correcting the swapped inputs of 'Wizard of Wor'.",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "dual_stick",  "Dual Stick" },
         { "swap_ports",  "Swap Ports" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "a5200_pause_is_reset",
      "Pause acts as Reset",
      NULL,
      "Pressing the button assigned to Pause will send a reset instead.  Some games like Kangaroo and Pole Position need this in order to change game difficulty or number of players.",
      NULL,
      "input",
      {
         { "disabled", "Disabled" },
         { "enabled",  "Enabled" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "a5200_digital_sensitivity",
      "Digital Joystick Sensitivity",
      NULL,
      "Set the effective range of the emulated analog joystick when using the gamepad's digital D-Pad for movement. Lower values equate to slower speeds. 'Auto' sets value based on cartridge checksum (requires good ROM dumps).",
      NULL,
      "input",
      {
         { "auto", "Auto" },
         { "5",    "5%" },
         { "7",    "7%" },
         { "10",   "10%" },
         { "12",   "12%" },
         { "15",   "15%" },
         { "17",   "17%" },
         { "20",   "20%" },
         { "22",   "22%" },
         { "25",   "25%" },
         { "27",   "27%" },
         { "30",   "30%" },
         { "32",   "32%" },
         { "35",   "35%" },
         { "37",   "37%" },
         { "40",   "40%" },
         { "42",   "42%" },
         { "45",   "45%" },
         { "47",   "47%" },
         { "50",   "50%" },
         { "52",   "52%" },
         { "55",   "55%" },
         { "57",   "57%" },
         { "60",   "60%" },
         { "62",   "62%" },
         { "65",   "65%" },
         { "67",   "67%" },
         { "70",   "70%" },
         { "72",   "72%" },
         { "75",   "75%" },
         { "77",   "77%" },
         { "80",   "80%" },
         { "82",   "82%" },
         { "85",   "85%" },
         { "87",   "87%" },
         { "90",   "90%" },
         { "92",   "92%" },
         { "95",   "95%" },
         { "97",   "97%" },
         { "100",  "100%" },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "a5200_analog_sensitivity",
      "Analog Joystick Sensitivity",
      NULL,
      "Set the effective range of the emulated analog joystick when using the gamepad's left analog stick for movement. Lower values equate to slower speeds. 'Auto' sets value based on cartridge checksum (requires good ROM dumps).",
      NULL,
      "input",
      {
         { "auto", "Auto" },
         { "5",    "5%" },
         { "7",    "7%" },
         { "10",   "10%" },
         { "12",   "12%" },
         { "15",   "15%" },
         { "17",   "17%" },
         { "20",   "20%" },
         { "22",   "22%" },
         { "25",   "25%" },
         { "27",   "27%" },
         { "30",   "30%" },
         { "32",   "32%" },
         { "35",   "35%" },
         { "37",   "37%" },
         { "40",   "40%" },
         { "42",   "42%" },
         { "45",   "45%" },
         { "47",   "47%" },
         { "50",   "50%" },
         { "52",   "52%" },
         { "55",   "55%" },
         { "57",   "57%" },
         { "60",   "60%" },
         { "62",   "62%" },
         { "65",   "65%" },
         { "67",   "67%" },
         { "70",   "70%" },
         { "72",   "72%" },
         { "75",   "75%" },
         { "77",   "77%" },
         { "80",   "80%" },
         { "82",   "82%" },
         { "85",   "85%" },
         { "87",   "87%" },
         { "90",   "90%" },
         { "92",   "92%" },
         { "95",   "95%" },
         { "97",   "97%" },
         { "100",  "100%" },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "a5200_analog_response",
      "Analog Joystick Response",
      NULL,
      "Configure movement speed response when tilting the gamepad's analog stick. 'Linear': Speed is directly proportional to stick displacement. 'Quadratic': Speed increases quadratically with stick displacement, enabling greater precision when making small movements without sacrificing maximum speed at full range.",
      NULL,
      "input",
      {
         { "linear",    "Linear" },
         { "quadratic", "Quadratic" },
         { NULL, NULL },
      },
      "linear"
   },
   {
      "a5200_analog_deadzone",
      "Analog Joystick Deadzone",
      NULL,
      "Set the deadzone of the gamepad's analog sticks. May be used to eliminate controller drift.",
      NULL,
      "input",
      {
         { "0",  "0%" },
         { "3",  "3%" },
         { "5",  "5%" },
         { "7",  "7%" },
         { "10", "10%" },
         { "13", "13%" },
         { "15", "15%" },
         { "17", "17%" },
         { "20", "20%" },
         { "23", "23%" },
         { "25", "25%" },
         { "27", "27%" },
         { "30", "30%" },
         { NULL, NULL },
      },
      "15"
   },
   {
      "a5200_analog_device",
      "Analog Device",
      NULL,
      "Set the device used for analog input.",
      NULL,
      "input",
      {
         { "analog_stick", "Analog Stick" },
         { "mouse", "Mouse" },
         { NULL, NULL },
      },
      "analog_stick"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

struct retro_core_options_v2 options_us = {
   option_cats_us,
   option_defs_us
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_options_v2 *options_intl[RETRO_LANGUAGE_LAST] = {
   &options_us, /* RETRO_LANGUAGE_ENGLISH */
   NULL,        /* RETRO_LANGUAGE_JAPANESE */
   NULL,        /* RETRO_LANGUAGE_FRENCH */
   NULL,        /* RETRO_LANGUAGE_SPANISH */
   NULL,        /* RETRO_LANGUAGE_GERMAN */
   NULL,        /* RETRO_LANGUAGE_ITALIAN */
   NULL,        /* RETRO_LANGUAGE_DUTCH */
   NULL,        /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,        /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,        /* RETRO_LANGUAGE_RUSSIAN */
   NULL,        /* RETRO_LANGUAGE_KOREAN */
   NULL,        /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,        /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,        /* RETRO_LANGUAGE_ESPERANTO */
   NULL,        /* RETRO_LANGUAGE_POLISH */
   NULL,        /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,        /* RETRO_LANGUAGE_ARABIC */
   NULL,        /* RETRO_LANGUAGE_GREEK */
   NULL,        /* RETRO_LANGUAGE_TURKISH */
   NULL,        /* RETRO_LANGUAGE_SLOVAK */
   NULL,        /* RETRO_LANGUAGE_PERSIAN */
   NULL,        /* RETRO_LANGUAGE_HEBREW */
   NULL,        /* RETRO_LANGUAGE_ASTURIAN */
   NULL,        /* RETRO_LANGUAGE_FINNISH */
};
#endif

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should be called as early as possible - ideally inside
 * retro_set_environment(), and no later than retro_load_game()
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static INLINE void libretro_set_core_options(retro_environment_t environ_cb,
      bool *categories_supported)
{
   unsigned version  = 0;
#ifndef HAVE_NO_LANGEXTRA
   unsigned language = 0;
#endif

   if (!environ_cb || !categories_supported)
      return;

   *categories_supported = false;

   if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
      version = 0;

   if (version >= 2)
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_v2_intl core_options_intl;

      core_options_intl.us    = &options_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = options_intl[language];

      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL,
            &core_options_intl);
#else
      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
            &options_us);
#endif
   }
   else
   {
      size_t i, j;
      size_t option_index              = 0;
      size_t num_options               = 0;
      struct retro_core_option_definition
            *option_v1_defs_us         = NULL;
#ifndef HAVE_NO_LANGEXTRA
      size_t num_options_intl          = 0;
      struct retro_core_option_v2_definition
            *option_defs_intl          = NULL;
      struct retro_core_option_definition
            *option_v1_defs_intl       = NULL;
      struct retro_core_options_intl
            core_options_v1_intl;
#endif
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine total number of options */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      if (version >= 1)
      {
         /* Allocate US array */
         option_v1_defs_us = (struct retro_core_option_definition *)
               calloc(num_options + 1, sizeof(struct retro_core_option_definition));

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            struct retro_core_option_v2_definition *option_def_us = &option_defs_us[i];
            struct retro_core_option_value *option_values         = option_def_us->values;
            struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
            struct retro_core_option_value *option_v1_values      = option_v1_def_us->values;

            option_v1_def_us->key           = option_def_us->key;
            option_v1_def_us->desc          = option_def_us->desc;
            option_v1_def_us->info          = option_def_us->info;
            option_v1_def_us->default_value = option_def_us->default_value;

            /* Values must be copied individually... */
            while (option_values->value)
            {
               option_v1_values->value = option_values->value;
               option_v1_values->label = option_values->label;

               option_values++;
               option_v1_values++;
            }
         }

#ifndef HAVE_NO_LANGEXTRA
         if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
             (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH) &&
             options_intl[language])
            option_defs_intl = options_intl[language]->definitions;

         if (option_defs_intl)
         {
            /* Determine number of intl options */
            while (true)
            {
               if (option_defs_intl[num_options_intl].key)
                  num_options_intl++;
               else
                  break;
            }

            /* Allocate intl array */
            option_v1_defs_intl = (struct retro_core_option_definition *)
                  calloc(num_options_intl + 1, sizeof(struct retro_core_option_definition));

            /* Copy parameters from option_defs_intl array */
            for (i = 0; i < num_options_intl; i++)
            {
               struct retro_core_option_v2_definition *option_def_intl = &option_defs_intl[i];
               struct retro_core_option_value *option_values           = option_def_intl->values;
               struct retro_core_option_definition *option_v1_def_intl = &option_v1_defs_intl[i];
               struct retro_core_option_value *option_v1_values        = option_v1_def_intl->values;

               option_v1_def_intl->key           = option_def_intl->key;
               option_v1_def_intl->desc          = option_def_intl->desc;
               option_v1_def_intl->info          = option_def_intl->info;
               option_v1_def_intl->default_value = option_def_intl->default_value;

               /* Values must be copied individually... */
               while (option_values->value)
               {
                  option_v1_values->value = option_values->value;
                  option_v1_values->label = option_values->label;

                  option_values++;
                  option_v1_values++;
               }
            }
         }

         core_options_v1_intl.us    = option_v1_defs_us;
         core_options_v1_intl.local = option_v1_defs_intl;

         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_v1_intl);
#else
         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
#endif
      }
      else
      {
         /* Allocate arrays */
         variables  = (struct retro_variable *)calloc(num_options + 1,
               sizeof(struct retro_variable));
         values_buf = (char **)calloc(num_options, sizeof(char *));

         if (!variables || !values_buf)
            goto error;

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            const char *key                        = option_defs_us[i].key;
            const char *desc                       = option_defs_us[i].desc;
            const char *default_value              = option_defs_us[i].default_value;
            struct retro_core_option_value *values = option_defs_us[i].values;
            size_t buf_len                         = 3;
            size_t default_index                   = 0;

            values_buf[i] = NULL;

            if (desc)
            {
               size_t num_values = 0;

               /* Determine number of values */
               while (true)
               {
                  if (values[num_values].value)
                  {
                     /* Check if this is the default value */
                     if (default_value)
                        if (strcmp(values[num_values].value, default_value) == 0)
                           default_index = num_values;

                     buf_len += strlen(values[num_values].value);
                     num_values++;
                  }
                  else
                     break;
               }

               /* Build values string */
               if (num_values > 0)
               {
                  buf_len += num_values - 1;
                  buf_len += strlen(desc);

                  values_buf[i] = (char *)calloc(buf_len, sizeof(char));
                  if (!values_buf[i])
                     goto error;

                  strcpy(values_buf[i], desc);
                  strcat(values_buf[i], "; ");

                  /* Default value goes first */
                  strcat(values_buf[i], values[default_index].value);

                  /* Add remaining values */
                  for (j = 0; j < num_values; j++)
                  {
                     if (j != default_index)
                     {
                        strcat(values_buf[i], "|");
                        strcat(values_buf[i], values[j].value);
                     }
                  }
               }
            }

            variables[option_index].key   = key;
            variables[option_index].value = values_buf[i];
            option_index++;
         }

         /* Set variables */
         environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
      }

error:
      /* Clean up */

      if (option_v1_defs_us)
      {
         free(option_v1_defs_us);
         option_v1_defs_us = NULL;
      }

#ifndef HAVE_NO_LANGEXTRA
      if (option_v1_defs_intl)
      {
         free(option_v1_defs_intl);
         option_v1_defs_intl = NULL;
      }
#endif

      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
