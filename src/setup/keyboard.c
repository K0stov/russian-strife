//
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#include "textscreen.h"
#include "doomtype.h"
#include "m_config.h"
#include "m_controls.h"
#include "m_misc.h"

#include "execute.h"
#include "txt_keyinput.h"

#include "mode.h"
#include "joystick.h"
#include "keyboard.h"

#define WINDOW_HELP_URL "https://www.chocolate-doom.org/setup-keyboard"

int vanilla_keyboard_mapping = 1;

static int always_run = 0;

// Keys within these groups cannot have the same value.

static int *controls[] = { &key_left, &key_right, &key_up, &key_down,
                           &key_strafeleft, &key_straferight, &key_fire,
                           &key_use, &key_strafe, &key_speed, &key_jump,
                           &key_flyup, &key_flydown, &key_flycenter,
                           &key_lookup, &key_lookdown, &key_lookcenter,
                           &key_invleft, &key_invright, &key_invquery,
                           &key_invuse, &key_invpop, &key_mission, &key_invkey,
                           &key_invhome, &key_invend, &key_invdrop,
                           &key_useartifact, &key_pause, &key_usehealth,
                           &key_weapon1, &key_weapon2, &key_weapon3,
                           &key_weapon4, &key_weapon5, &key_weapon6,
                           &key_weapon7, &key_weapon8,
                           &key_arti_all, &key_arti_health, &key_arti_poisonbag,
                           &key_arti_blastradius, &key_arti_teleport,
                           &key_arti_teleportother, &key_arti_egg,
                           &key_arti_invulnerability,
                           &key_prevweapon, &key_nextweapon, NULL };

static int *menu_nav[] = { &key_menu_activate, &key_menu_up, &key_menu_down,
                           &key_menu_left, &key_menu_right, &key_menu_back,
                           &key_menu_forward, NULL };

static int *shortcuts[] = { &key_menu_help, &key_menu_save, &key_menu_load,
                            &key_menu_volume, &key_menu_detail, &key_menu_qsave,
                            &key_menu_endgame, &key_menu_messages, &key_spy,
                            &key_menu_qload, &key_menu_quit, &key_menu_gamma,
                            &key_menu_incscreen, &key_menu_decscreen, 
                            &key_menu_screenshot,
                            &key_message_refresh, &key_multi_msg,
                            &key_multi_msgplayer[0], &key_multi_msgplayer[1],
                            &key_multi_msgplayer[2], &key_multi_msgplayer[3] };

static int *map_keys[] = { &key_map_north, &key_map_south, &key_map_east,
                           &key_map_west, &key_map_zoomin, &key_map_zoomout,
                           &key_map_toggle, &key_map_maxzoom, &key_map_follow,
                           &key_map_grid, &key_map_mark, &key_map_clearmark,
                           NULL };

static void UpdateJoybSpeed(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(var))
{
    if (always_run)
    {
        /*
         <Janizdreg> if you want to pick one for chocolate doom to use, 
                     pick 29, since that is the most universal one that 
                     also works with heretic, hexen and strife =P

         NB. This choice also works with original, ultimate and final exes.
        */

        joybspeed = 29;
    }
    else
    {
        joybspeed = 0;
    }
}

static int VarInGroup(int *variable, int **group)
{
    unsigned int i;

    for (i=0; group[i] != NULL; ++i)
    {
        if (group[i] == variable)
        {
            return 1;
        }
    }

    return 0;
}

static void CheckKeyGroup(int *variable, int **group)
{
    unsigned int i;

    // Don't check unless the variable is in this group.

    if (!VarInGroup(variable, group))
    {
        return;
    }

    // If another variable has the same value as the new value, reset it.

    for (i=0; group[i] != NULL; ++i)
    {
        if (*variable == *group[i] && group[i] != variable)
        {
            // A different key has the same value.  Clear the existing
            // value. This ensures that no two keys can have the same
            // value.

            *group[i] = 0;
        }
    }
}

// Callback invoked when a key control is set

static void KeySetCallback(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(variable))
{
    TXT_CAST_ARG(int, variable);

    CheckKeyGroup(variable, controls);
    CheckKeyGroup(variable, menu_nav);
    CheckKeyGroup(variable, shortcuts);
    CheckKeyGroup(variable, map_keys);
}

// Add a label and keyboard input to the specified table.

static void AddKeyControl(TXT_UNCAST_ARG(table), char *name, int *var)
{
    TXT_CAST_ARG(txt_table_t, table);
    txt_key_input_t *key_input;

    TXT_AddWidget(table, TXT_NewLabel(name));
    key_input = TXT_NewKeyInput(var);
    TXT_AddWidget(table, key_input);

    TXT_SignalConnect(key_input, "set", KeySetCallback, var);
}

static void AddSectionLabel(TXT_UNCAST_ARG(table), char *title,
                            boolean add_space)
{
    TXT_CAST_ARG(txt_table_t, table);
    char buf[64];

    if (add_space)
    {
        TXT_AddWidgets(table,
                       TXT_NewStrut(0, 1),
                       TXT_TABLE_EOL,
                       NULL);
    }

    M_snprintf(buf, sizeof(buf), " - %s - ", title);

    TXT_AddWidgets(table,
                   TXT_NewLabel(buf),
                   TXT_TABLE_EOL,
                   NULL);
}
static void ConfigExtraKeys(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(unused))
{
    txt_window_t *window;
    txt_scrollpane_t *scrollpane;
    txt_table_t *table;
    boolean extra_keys = gamemission == heretic
                      || gamemission == hexen
                      || gamemission == strife;

    window = TXT_NewWindow("äε∩εδφΦ≥σδⁿφεσ ≤∩≡αΓδσφΦσ");

    TXT_SetWindowHelpURL(window, WINDOW_HELP_URL);

    table = TXT_NewTable(2);

    TXT_SetColumnWidths(table, 21, 9);

    if (extra_keys)
    {
        // When we have extra controls, a scrollable pane must be used.

        scrollpane = TXT_NewScrollPane(0, 13, table);
        TXT_AddWidget(window, scrollpane);

        AddSectionLabel(table, "éτπδ Σ", false);

        AddKeyControl(table, "æ∞ε≥≡σ≥ⁿ ΓΓσ≡⌡", &key_lookup);
        AddKeyControl(table, "æ∞ε≥≡σ≥ⁿ ΓφΦτ", &key_lookdown);
        AddKeyControl(table, "ûσφ≥≡Φ≡εΓα≥ⁿ", &key_lookcenter);

        if (gamemission == heretic || gamemission == hexen)
        {
            AddSectionLabel(table, "Flying", true);

            AddKeyControl(table, "Fly up", &key_flyup);
            AddKeyControl(table, "Fly down", &key_flydown);
            AddKeyControl(table, "Fly center", &key_flycenter);
        }

        AddSectionLabel(table, "êφΓσφ≥α≡ⁿ", true);

        AddKeyControl(table, "Å≡εδΦ±≥α≥ⁿ ΓδσΓε", &key_invleft);
        AddKeyControl(table, "Å≡εδΦ±≥α≥ⁿ Γ∩≡αΓε", &key_invright);

        if (gamemission == strife)
        {
            AddKeyControl(table, "Å≡εδΦ±≥α≥ⁿ Γ φα≈αδε", &key_invhome);
            AddKeyControl(table, "Å≡εδΦ±≥α≥ⁿ Γ Ωεφσ÷", &key_invend);
            AddKeyControl(table, "Query", &key_invquery);
            AddKeyControl(table, "ü≡ε±Φ≥ⁿ ∩≡σΣ∞σ≥", &key_invdrop);
            AddKeyControl(table, "ÅεΩατα≥ⁿ ε≡≤µΦσ", &key_invpop);
            AddKeyControl(table, "ÅεΩατα≥ⁿ ταΣαφΦ", &key_mission);
            AddKeyControl(table, "ÅεΩατα≥ⁿ Ωδ■≈Φ", &key_invkey);
            AddKeyControl(table, "ê±∩εδⁿτεΓα≥ⁿ ∩≡σΣ∞σ≥", &key_invuse);
            AddKeyControl(table, "ü√±≥≡εσ δσ≈σφΦσ", &key_usehealth);
        }
        else
        {
            AddKeyControl(table, "Use artifact", &key_useartifact);
        }

        if (gamemission == hexen)
        {
            AddSectionLabel(table, "Artifacts", true);

            AddKeyControl(table, "One of each", &key_arti_all);
            AddKeyControl(table, "Quartz Flask", &key_arti_health);
            AddKeyControl(table, "Flechette", &key_arti_poisonbag);
            AddKeyControl(table, "Disc of Repulsion", &key_arti_blastradius);
            AddKeyControl(table, "Chaos Device", &key_arti_teleport);
            AddKeyControl(table, "Banishment Device", &key_arti_teleportother);
            AddKeyControl(table, "Porkalator", &key_arti_egg);
            AddKeyControl(table, "Icon of the Defender",
                          &key_arti_invulnerability);
        }
    }
    else
    {
        TXT_AddWidget(window, table);
    }

    AddSectionLabel(table, "Ä≡≤µΦσ", extra_keys);

    AddKeyControl(table, "Ä≡≤µΦσ 1", &key_weapon1);
    AddKeyControl(table, "Ä≡≤µΦσ 2", &key_weapon2);
    AddKeyControl(table, "Ä≡≤µΦσ 3", &key_weapon3);
    AddKeyControl(table, "Ä≡≤µΦσ 4", &key_weapon4);
    AddKeyControl(table, "Ä≡≤µΦσ 5", &key_weapon5);
    AddKeyControl(table, "Ä≡≤µΦσ 6", &key_weapon6);
    AddKeyControl(table, "Ä≡≤µΦσ 7", &key_weapon7);
    AddKeyControl(table, "Ä≡≤µΦσ 8", &key_weapon8);
    AddKeyControl(table, "Å≡σΣ√Σ≤∙σσ ε≡≤µΦσ", &key_prevweapon);
    AddKeyControl(table, "æδσΣ≤■∙σσ ε≡≤µΦσ", &key_nextweapon);
}

static void OtherKeysDialog(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(unused))
{
    txt_window_t *window;
    txt_table_t *table;
    txt_scrollpane_t *scrollpane;

    window = TXT_NewWindow("ä≡≤πΦσ ΩδαΓΦ°Φ");

    TXT_SetWindowHelpURL(window, WINDOW_HELP_URL);

    table = TXT_NewTable(2);

    TXT_SetColumnWidths(table, 25, 9);

    AddSectionLabel(table, "ìαΓΦπα÷Φ  Γ ∞σφ■", false);

    AddKeyControl(table, "ÇΩ≥ΦΓΦ≡εΓα≥ⁿ ∞σφ■",         &key_menu_activate);
    AddKeyControl(table, "è≤≡±ε≡ ΓΓσ≡⌡",        &key_menu_up);
    AddKeyControl(table, "è≤≡±ε≡ ΓφΦτ",      &key_menu_down);
    AddKeyControl(table, "Åεδτ≤φεΩ ΓδσΓε",      &key_menu_left);
    AddKeyControl(table, "Åεδτ≤φεΩ Γ∩≡αΓε",     &key_menu_right);
    AddKeyControl(table, "Å≡σΣ√Σ≤∙ΦΘ ²Ω≡αφ",   &key_menu_back);
    AddKeyControl(table, "ÇΩ≥ΦΓΦ≡εΓα≥ⁿ ∩≤φΩ≥ ∞σφ■",    &key_menu_forward);
    AddKeyControl(table, "ÅεΣ≥Γσ≡ΣΦ≥ⁿ ΣσΘ±≥ΓΦσ",        &key_menu_confirm);
    AddKeyControl(table, "Ä≥∞σφΦ≥ⁿ ΣσΘ±≥ΓΦσ",         &key_menu_abort);

    AddSectionLabel(table, "èδαΓΦ°Φ ß√±≥≡επε Σε±≥≤∩α", true);

    AddKeyControl(table, "Åα≤τα",                         &key_pause);
    AddKeyControl(table, "Åε∞ε∙ⁿ",                        &key_menu_help);
    AddKeyControl(table, "æε⌡≡αφσφΦσ",                    &key_menu_save);
    AddKeyControl(table, "çαπ≡≤τΩα",                      &key_menu_load);
    AddKeyControl(table, "â≡ε∞Ωε±≥ⁿ",                     &key_menu_volume);
    AddKeyControl(table, "äσ≥αδΦτα÷Φ ",                    &key_menu_detail);
    AddKeyControl(table, "ü√±≥≡εσ ±ε⌡≡αφσφΦσ",            &key_menu_qsave);
    AddKeyControl(table, "çαΩεφ≈Φ≥ⁿ Φπ≡≤",                &key_menu_endgame);
    AddKeyControl(table, "æεεß∙σφΦ",                      &key_menu_messages);
    AddKeyControl(table, "ü√±≥≡α  ταπ≡≤τΩα",               &key_menu_qload);
    AddKeyControl(table, "é√⌡εΣ Φτ Φπ≡√",                 &key_menu_quit);
    AddKeyControl(table, "âα∞∞α-Ωε≡≡σΩ÷Φ ",                &key_menu_gamma);
    AddKeyControl(table, "éΦΣ Σ≡≤πεπε Φπ≡εΩα",            &key_spy);

    AddKeyControl(table, "ôΓσδΦ≈Φ≥ⁿ ²Ω≡αφ",               &key_menu_incscreen);
    AddKeyControl(table, "ô∞σφⁿ°Φ≥ⁿ ²Ω≡αφ",               &key_menu_decscreen);
    AddKeyControl(table, "æΩ≡Φφ°ε≥",                      &key_menu_screenshot);

    AddKeyControl(table, "ÅεΩατα≥ⁿ ∩ε±δσΣφσσ ±εεß∙σφΦσ",  &key_message_refresh);
    AddKeyControl(table, "çαΩεφ≈Φ≥ⁿ τα∩Φ±ⁿ Σσ∞ε",         &key_demo_quit);

    AddSectionLabel(table, "èα≡≥α", true);
    AddKeyControl(table, "Ä≥Ω≡√≥ⁿ Ωα≡≥≤",                 &key_map_toggle);
    AddKeyControl(table, "Å≡ΦßδΦτΦ≥ⁿ",                    &key_map_zoomin);
    AddKeyControl(table, "Ä≥ΣαδΦ≥ⁿ",                      &key_map_zoomout);
    AddKeyControl(table, "Åεδφ√Θ ∞α±°≥αß",                &key_map_maxzoom);
    AddKeyControl(table, "ÉσµΦ∞ ±δσΣεΓαφΦ ",               &key_map_follow);
    AddKeyControl(table, "Å≡εΩ≡≤≥Φ≥ⁿ ΓΓσ≡⌡",              &key_map_north);
    AddKeyControl(table, "Å≡εΩ≡≤≥Φ≥ⁿ ΓφΦτ",               &key_map_south);
    AddKeyControl(table, "Å≡εΩ≡≤≥Φ≥ⁿ Γ∩≡αΓε",             &key_map_east);
    AddKeyControl(table, "Å≡εΩ≡≤≥Φ≥ⁿ ΓδσΓε",              &key_map_west);
    AddKeyControl(table, "Ä≥εß≡ατΦ≥ⁿ ±σ≥Ω≤",              &key_map_grid);
    AddKeyControl(table, "Åε±≥αΓΦ≥ⁿ ε≥∞σ≥Ω≤",             &key_map_mark);
    AddKeyControl(table, "ôß≡α≥ⁿ ε≥∞σ≥ΩΦ",                &key_map_clearmark);

    AddSectionLabel(table, "æσ≥σΓα  Φπ≡α", true);

    AddKeyControl(table, "Ä≥∩≡αΓΦ≥ⁿ ±εεß∙σφΦσ",          &key_multi_msg);
    AddKeyControl(table, "- Φπ≡εΩ≤ 1",         &key_multi_msgplayer[0]);
    AddKeyControl(table, "- Φπ≡εΩ≤ 2",         &key_multi_msgplayer[1]);
    AddKeyControl(table, "- Φπ≡εΩ≤ 3",         &key_multi_msgplayer[2]);
    AddKeyControl(table, "- Φπ≡εΩ≤ 4",         &key_multi_msgplayer[3]);

    if (gamemission == hexen || gamemission == strife)
    {
        AddKeyControl(table, "- Φπ≡εΩ≤ 5",     &key_multi_msgplayer[4]);
        AddKeyControl(table, "- Φπ≡εΩ≤ 6",     &key_multi_msgplayer[5]);
        AddKeyControl(table, "- Φπ≡εΩ≤ 7",     &key_multi_msgplayer[6]);
        AddKeyControl(table, "- Φπ≡εΩ≤ 8",     &key_multi_msgplayer[7]);
    }

    scrollpane = TXT_NewScrollPane(0, 13, table);

    TXT_AddWidget(window, scrollpane);
}

void ConfigKeyboard(void)
{
    txt_window_t *window;
    txt_checkbox_t *run_control;

    always_run = joybspeed >= 20;

    window = TXT_NewWindow("ìα±≥≡εΘΩΦ ΩδαΓΦα≥≤≡√");

    TXT_SetWindowHelpURL(window, WINDOW_HELP_URL);

    // The window is on a 5-column grid layout that looks like:
    // Label | Control | | Label | Control
    // There is a small gap between the two conceptual "columns" of
    // controls, just for spacing.
    TXT_SetTableColumns(window, 5);
    TXT_SetColumnWidths(window, 15, 8, 2, 15, 8);

    TXT_AddWidget(window, TXT_NewSeparator("äΓΦµσφΦσ"));
    AddKeyControl(window, "äΓΦµσφΦσ Γ∩σ≡σΣ", &key_up);
    TXT_AddWidget(window, TXT_TABLE_EMPTY);
    AddKeyControl(window, "üεΩε∞ ΓδσΓε", &key_strafeleft);

    AddKeyControl(window, "äΓΦµσφΦσ φαταΣ", &key_down);
    TXT_AddWidget(window, TXT_TABLE_EMPTY);
    AddKeyControl(window, "üεΩε∞ Γ∩≡αΓε", &key_straferight);

    AddKeyControl(window, "ÅεΓε≡ε≥ φαδσΓε", &key_left);
    TXT_AddWidget(window, TXT_TABLE_EMPTY);
    AddKeyControl(window, "üσπ", &key_speed);

    AddKeyControl(window, "ÅεΓε≡ε≥ φα∩≡αΓε", &key_right);
    TXT_AddWidget(window, TXT_TABLE_EMPTY);
    AddKeyControl(window, "äΓΦµσφΦσ ßεΩε∞", &key_strafe);

    if (gamemission == hexen || gamemission == strife)
    {
        AddKeyControl(window, "Å≡√µεΩ", &key_jump);
    }

    TXT_AddWidget(window, TXT_NewSeparator("äσΘ±≥ΓΦ "));
    AddKeyControl(window, "Ç≥αΩα/±≥≡σδⁿßα", &key_fire);
    TXT_AddWidget(window, TXT_TABLE_EMPTY);
    AddKeyControl(window, "ê±∩εδⁿτεΓα≥ⁿ", &key_use);

    TXT_AddWidgets(window,
                   TXT_NewButton2("äε∩εδφΦ≥σδⁿφε...", ConfigExtraKeys, NULL),
                   TXT_TABLE_OVERFLOW_RIGHT,
                   TXT_TABLE_EMPTY,
                   TXT_NewButton2("ä≡≤πΦσ ΩδαΓΦ°Φ...", OtherKeysDialog, NULL),
                   TXT_TABLE_OVERFLOW_RIGHT,

                   TXT_NewSeparator("äε∩εδφΦ≥σδⁿφε"),
                   run_control = TXT_NewCheckBox("ÉσµΦ∞ ∩ε±≥ε φφεπε ßσπα", &always_run),
                   TXT_TABLE_EOL,
                   TXT_NewInvertedCheckBox("ê±∩εδⁿτεΓα≥ⁿ φα≥ΦΓφ≤■ ≡α±ΩδαΣΩ≤",
                                           &vanilla_keyboard_mapping),
                   TXT_TABLE_EOL,
                   NULL);

    TXT_SignalConnect(run_control, "changed", UpdateJoybSpeed, NULL);
    TXT_SetWindowAction(window, TXT_HORIZ_CENTER, TestConfigAction());
}

void BindKeyboardVariables(void)
{
    M_BindIntVariable("vanilla_keyboard_mapping", &vanilla_keyboard_mapping);
}
