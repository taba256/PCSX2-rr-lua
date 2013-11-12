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

#include "GSDevice.h"
#include "GSTexture7.h"

class GSDevice7 : public GSDevice<GSTexture7>
{
public:
	typedef GSTexture7 Texture;

private:
	CComPtr<IDirectDraw7> m_dd;
	CComPtr<IDirectDrawSurface7> m_primary;
	CComPtr<IDirectDrawSurface7> m_backbuffer;	

	bool Create(int type, Texture& t, int w, int h, int format);
	void DoMerge(Texture* st, GSVector4* sr, GSVector4* dr, Texture& dt, bool slbg, bool mmod, GSVector4& c);
	void DoInterlace(Texture& st, Texture& dt, int shader, bool linear, float yoffset = 0);

public:
	GSDevice7();
	virtual ~GSDevice7();

	bool Create(HWND hWnd, bool vsync);
	bool Reset(int w, int h, bool fs);
	bool IsLost() {return false;}
	void Present(const CRect& r);
	void BeginScene() {}
	void EndScene() {}
	void Draw(LPCTSTR str) {}
	bool CopyOffscreen(Texture& src, const GSVector4& sr, Texture& dst, int w, int h, int format = 0) {return false;}

	void ClearRenderTarget(Texture& t, const GSVector4& c) {}
	void ClearRenderTarget(Texture& t, DWORD c) {}
	void ClearDepth(Texture& t, float c) {}
	void ClearStencil(Texture& t, BYTE c) {}
};
