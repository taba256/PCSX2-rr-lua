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

#include "stdafx.h"
#include "GSTexture7.h"

GSTexture7::GSTexture7()
	: m_type(GSTexture::None)
{
	memset(&m_desc, 0, sizeof(m_desc));
}

GSTexture7::GSTexture7(int type, IDirectDrawSurface7* system)
	: m_type(type)
	, m_system(system)
{
	memset(&m_desc, 0, sizeof(m_desc));

	m_desc.dwSize = sizeof(m_desc);

	system->GetSurfaceDesc(&m_desc);
}

GSTexture7::GSTexture7(int type, IDirectDrawSurface7* system, IDirectDrawSurface7* video)
	: m_type(type)
	, m_system(system)
	, m_video(video)
{
	memset(&m_desc, 0, sizeof(m_desc));

	m_desc.dwSize = sizeof(m_desc);

	video->GetSurfaceDesc(&m_desc);
}

GSTexture7::~GSTexture7()
{
}

GSTexture7::operator bool()
{
	return !!m_system;
}

int GSTexture7::GetType() const
{
	return m_type;
}

int GSTexture7::GetWidth() const 
{
	return m_desc.dwWidth;
}

int GSTexture7::GetHeight() const 
{
	return m_desc.dwHeight;
}

int GSTexture7::GetFormat() const 
{
	return (int)m_desc.ddpfPixelFormat.dwFourCC;
}

bool GSTexture7::Update(const CRect& r, const void* data, int pitch)
{
	HRESULT hr;

	CRect r2 = r;

	DDSURFACEDESC2 desc;

	memset(&desc, 0, sizeof(desc));

	desc.dwSize = sizeof(desc);

	if(SUCCEEDED(hr = m_system->Lock(&r2, &desc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR | DDLOCK_WRITEONLY, NULL)))
	{
		BYTE* src = (BYTE*)data;
		BYTE* dst = (BYTE*)desc.lpSurface;

		int bytes = min(pitch, desc.lPitch);

		for(int i = 0, j = r.Height(); i < j; i++, src += pitch, dst += desc.lPitch)
		{
			// memcpy(dst, src, bytes);

			// HACK!!!

			GSVector4i* s = (GSVector4i*)src;
			GSVector4i* d = (GSVector4i*)dst;

			int w = bytes >> 4;

			for(int x = 0; x < w; x++)
			{
				GSVector4i c = s[x];

				c = (c & 0xff00ff00) | ((c & 0x00ff0000) >> 16) | ((c & 0x000000ff) << 16);

				d[x] = c;
			}
		}

		hr = m_system->Unlock(&r2);

		if(m_video)
		{
			hr = m_video->Blt(&r2, m_system, &r2, DDBLT_WAIT, NULL);
		}

		return true;
	}

	return false;
}

bool GSTexture7::Map(BYTE** bits, int& pitch, const RECT* r)
{
	HRESULT hr;

	CRect r2 = r;

	DDSURFACEDESC2 desc;

	if(SUCCEEDED(hr = m_system->Lock(&r2, &desc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL)))
	{
		*bits = (BYTE*)desc.lpSurface;
		pitch = (int)desc.lPitch;

		m_lr = r;

		return true;
	}

	return false;
}

void GSTexture7::Unmap()
{
	HRESULT hr;

	hr = m_system->Unlock(NULL);

	if(m_video)
	{
		hr = m_video->Blt(&m_lr, m_system, &m_lr, DDBLT_WAIT, NULL);
	}
}

bool GSTexture7::Save(CString fn, bool dds)
{
	// TODO

	return false;
}

IDirectDrawSurface7* GSTexture7::operator->()
{
	return m_video ? m_video : m_system;
}

GSTexture7::operator IDirectDrawSurface7*()
{
	return m_video ? m_video : m_system;
}
