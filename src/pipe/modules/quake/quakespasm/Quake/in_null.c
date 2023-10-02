/*
Copyright (C) 2022 johannes hanika

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// null input which does: nothing
// vkdt has their own input system which is already inited etc.
#include "quakedef.h"

void IN_Init (void) {}

void IN_Shutdown (void) {}

void IN_Commands (void) {}
// oportunity for devices to stick commands on the script buffer

// mouse moved by dx and dy pixels
void IN_MouseMotion(int dx, int dy) {}


void IN_SendKeyEvents (void) {}
// used as a callback for Sys_SendKeyEvents() by some drivers

void IN_UpdateInputMode (void) {}
// do stuff if input mode (text/non-text) changes matter to the keyboard driver

// vkdt needs to implement this:
// void IN_Move (usercmd_t *cmd) {}
// add additional movement on top of the keyboard move cmd

void IN_ClearStates (void) {}
// restores all button and position states to defaults

// called when the app becomes active
void IN_Activate () {}

// called when the app becomes inactive
void IN_Deactivate (qboolean free_cursor) {}
