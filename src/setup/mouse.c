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

#include <stdlib.h>

#include "textscreen.h"
#include "doomtype.h"
#include "m_config.h"
#include "m_controls.h"

#include "execute.h"
#include "txt_mouseinput.h"

#include "mode.h"
#include "mouse.h"

#define WINDOW_HELP_URL "https://www.chocolate-doom.org/setup-mouse"

static int usemouse = 1;

static int mouseSensitivity = 5;
static float mouse_acceleration = 2.0;
static int mouse_threshold = 10;
static int grabmouse = 1;

int novert = 0;

static int *all_mouse_buttons[] = {
    &mousebfire,
    &mousebstrafe,
    &mousebforward,
    &mousebstrafeleft,
    &mousebstraferight,
    &mousebbackward,
    &mousebuse,
    &mousebjump,
    &mousebprevweapon,
    &mousebnextweapon
};

static void MouseSetCallback(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(variable))
{
    TXT_CAST_ARG(int, variable);
    unsigned int i;

    // Check if the same mouse button is used for a different action
    // If so, set the other action(s) to -1 (unset)

    for (i=0; i<arrlen(all_mouse_buttons); ++i)
    {
        if (*all_mouse_buttons[i] == *variable
         && all_mouse_buttons[i] != variable)
        {
            *all_mouse_buttons[i] = -1;
        }
    }
}

static void AddMouseControl(TXT_UNCAST_ARG(table), char *label, int *var)
{
    TXT_CAST_ARG(txt_table_t, table);
    txt_mouse_input_t *mouse_input;

    TXT_AddWidget(table, TXT_NewLabel(label));

    mouse_input = TXT_NewMouseInput(var);
    TXT_AddWidget(table, mouse_input);

    TXT_SignalConnect(mouse_input, "set", MouseSetCallback, var);
}

static void ConfigExtraButtons(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(unused))
{
    txt_window_t *window;
    txt_table_t *buttons_table;

    window = TXT_NewWindow("äε∩εδφΦ≥σδⁿφ√σ Ωφε∩ΩΦ ∞√°Φ");

    TXT_SetWindowHelpURL(window, WINDOW_HELP_URL);

    TXT_AddWidgets(window,
                   buttons_table = TXT_NewTable(2),
                   NULL);

    TXT_SetColumnWidths(buttons_table, 24, 5);

    AddMouseControl(buttons_table, "äΓΦµσφΦσ φαταΣ", &mousebbackward);
    AddMouseControl(buttons_table, "ê±∩εδⁿτεΓα≥ⁿ", &mousebuse);
    AddMouseControl(buttons_table, "üεΩε∞ ΓδσΓε", &mousebstrafeleft);
    AddMouseControl(buttons_table, "üεΩε∞ Γ∩≡αΓε", &mousebstraferight);

    if (gamemission == hexen || gamemission == strife)
    {
        AddMouseControl(buttons_table, "Å≡√µεΩ", &mousebjump);
    }

    AddMouseControl(buttons_table, "Å≡σΣ√Σ≤∙σσ ε≡≤µΦσ", &mousebprevweapon);
    AddMouseControl(buttons_table, "æδσΣ≤■∙σσ ε≡≤µΦσ", &mousebnextweapon);
}

void ConfigMouse(void)
{
    txt_window_t *window;

    window = TXT_NewWindow("ìα±≥≡εΘΩΦ ∞√°Φ");

    TXT_SetTableColumns(window, 2);

    TXT_SetWindowAction(window, TXT_HORIZ_CENTER, TestConfigAction());
    TXT_SetWindowHelpURL(window, WINDOW_HELP_URL);

    TXT_AddWidgets(window,
                   TXT_NewCheckBox("Éατ≡σ°Φ≥ⁿ Φ±∩εδⁿτεΓαφΦσ ∞√°Φ", &usemouse),
                   TXT_TABLE_OVERFLOW_RIGHT,
                   TXT_NewInvertedCheckBox("éσ≡≥ΦΩαδⁿφ√Θ εßτε≡ ∞√°ⁿ■", 
                                           &novert),
                   TXT_TABLE_OVERFLOW_RIGHT,
                   TXT_NewCheckBox("çα⌡Γα≥ ∞√°Φ Γ εΩεφφε∞ ≡σµΦ∞σ", 
                                   &grabmouse),
                   TXT_TABLE_OVERFLOW_RIGHT,
                   TXT_NewCheckBox("äΓεΘφεΘ ΩδΦΩ αΩ≥ΦΓΦτΦ≡≤σ≥ \"Φ±∩εδⁿτεΓαφΦσ\"",
                                   &dclick_use),
                   TXT_TABLE_OVERFLOW_RIGHT,

                   TXT_NewSeparator("ìα±≥≡εΘΩα ∩σ≡σ∞σ∙σφΦ "),
                   TXT_NewLabel("æΩε≡ε±≥ⁿ"),
                   TXT_NewSpinControl(&mouseSensitivity, 1, 256),
                   TXT_NewLabel("ÇΩ±σδσ≡α÷Φ "),
                   TXT_NewFloatSpinControl(&mouse_acceleration, 1.0, 5.0),
                   TXT_NewLabel("Åε≡επ αΩ±σδσ≡α÷ΦΦ"),
                   TXT_NewSpinControl(&mouse_threshold, 0, 32),

                   TXT_NewSeparator("èφε∩ΩΦ"),
                   NULL);

    AddMouseControl(window, "Ç≥αΩα/±≥≡σδⁿßα", &mousebfire);
    AddMouseControl(window, "äΓΦµσφΦσ Γ∩σ≡σΣ", &mousebforward);
    AddMouseControl(window, "äΓΦµσφΦσ ßεΩε∞", &mousebstrafe);

    TXT_AddWidget(window,
                  TXT_NewButton2("äε∩εδφΦ≥σδⁿφε...", ConfigExtraButtons, NULL));
}

void BindMouseVariables(void)
{
    M_BindIntVariable("use_mouse",               &usemouse);
    M_BindIntVariable("novert",                  &novert);
    M_BindIntVariable("grabmouse",               &grabmouse);
    M_BindIntVariable("mouse_sensitivity",       &mouseSensitivity);
    M_BindIntVariable("mouse_threshold",         &mouse_threshold);
    M_BindFloatVariable("mouse_acceleration",    &mouse_acceleration);
}
