/* 
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSLocalMemory.h"

class GSDirtyRect
{
	DWORD m_psm;
	CRect m_rect;

public:
	GSDirtyRect();
	GSDirtyRect(DWORD psm, CRect rect);
	CRect GetDirtyRect(const GIFRegTEX0& TEX0);
};

class GSDirtyRectList : public CAtlList<GSDirtyRect>
{
public:
	GSDirtyRectList() {}
	CRect GetDirtyRect(const GIFRegTEX0& TEX0, CSize size);
};