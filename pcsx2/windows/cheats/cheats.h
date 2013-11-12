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
#ifndef CHEATS_H_INCLUDED
#define CHEATS_H_INCLUDED

#ifndef __cplusplus
//typedef enum ebool
//{
//	false,
//	true
//} bool;
#define bool unsigned __int8
#define false 0
#define true 1
#endif

extern HINSTANCE pInstance;
extern bool FirstShow;

void AddCheat(HINSTANCE hInstance, HWND hParent);
void ShowFinder(HINSTANCE hInstance, HWND hParent);
void ShowCheats(HINSTANCE hInstance, HWND hParent);

#endif//CHEATS_H_INCLUDED
