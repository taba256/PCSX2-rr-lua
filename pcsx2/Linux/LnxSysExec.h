/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
 
 #ifndef __LNXSYSEXEC_H__
#define __LNXSYSEXEC_H__

#include "Linux.h"
#include "GS.h"
#include <sys/mman.h>
#include "x86/iR5900.h"

extern void StartGui();
extern void CheckSlots();

extern void SignalExit(int sig);
extern const char* g_pRunGSState;
extern int efile;
extern GtkWidget *pStatusBar;

#endif