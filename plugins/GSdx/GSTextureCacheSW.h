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

#include "GSRenderer.h"

#define MAX_PAGES 512

class GSTextureCacheSW
{
public:
	class GSTexture;
	class GSTexturePage;

	class GSTexturePage
	{
	public:
		GSTexture* t;
		DWORD row, col;
	};

	class GSTexturePageEntry
	{
	public:
		CAtlList<GSTexturePage*>* p2t;
		POSITION pos;
	};

	class GSTexture
	{
	public:
		GSState* m_state;
		GIFRegTEX0 m_TEX0;
		GIFRegTEXA m_TEXA;
		void* m_buff;
		DWORD m_tw;
		DWORD m_valid[32];
		DWORD m_maxpages;
		DWORD m_pages;
		CAtlList<GSTexturePageEntry*> m_p2te;
		POSITION m_pos;
		DWORD m_age;

		explicit GSTexture(GSState* state);
		virtual ~GSTexture();

		bool Update(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const CRect* r = NULL);
	};

protected:
	GSState* m_state;
	CAtlList<GSTexture*> m_textures;
	CAtlList<GSTexturePage*> m_p2t[MAX_PAGES];

public:
	GSTextureCacheSW(GSState* state);
	virtual ~GSTextureCacheSW();

	const GSTexture* Lookup(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const CRect* r = NULL);

	void RemoveAll();
	void IncAge();
	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r);
};
