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
#include "GSTexture9.h"

struct Direct3DSamplerState9
{
    D3DTEXTUREFILTERTYPE FilterMin[2];
    D3DTEXTUREFILTERTYPE FilterMag[2];
    D3DTEXTUREADDRESS AddressU;
    D3DTEXTUREADDRESS AddressV;
};

struct Direct3DDepthStencilState9
{
    BOOL DepthEnable;
    BOOL DepthWriteMask;
    D3DCMPFUNC DepthFunc;
    BOOL StencilEnable;
    UINT8 StencilReadMask;
    UINT8 StencilWriteMask;
    D3DSTENCILOP StencilFailOp;
    D3DSTENCILOP StencilDepthFailOp;
    D3DSTENCILOP StencilPassOp;
    D3DCMPFUNC StencilFunc;
};

struct Direct3DBlendState9
{
    BOOL BlendEnable;
    D3DBLEND SrcBlend;
    D3DBLEND DestBlend;
    D3DBLENDOP BlendOp;
    D3DBLEND SrcBlendAlpha;
    D3DBLEND DestBlendAlpha;
    D3DBLENDOP BlendOpAlpha;
    UINT8 RenderTargetWriteMask;
};

class GSDevice9 : public GSDevice<GSTexture9>
{
public:
	typedef GSTexture9 Texture;

private:
	// state cache

	IDirect3DVertexBuffer9* m_vb;
	UINT m_vb_count;
	const void* m_vb_vertices;
	UINT m_vb_stride;
	IDirect3DVertexDeclaration9* m_layout;
	D3DPRIMITIVETYPE m_topology;
	IDirect3DVertexShader9* m_vs;
	float* m_vs_cb;
	int m_vs_cb_len;
	IDirect3DTexture9* m_ps_srvs[2];
	IDirect3DPixelShader9* m_ps;
	float* m_ps_cb;
	int m_ps_cb_len;
	Direct3DSamplerState9* m_ps_ss;
	CRect m_scissor;
	Direct3DDepthStencilState9* m_dss;
	UINT m_sref;
	Direct3DBlendState9* m_bs;
	DWORD m_bf;
	IDirect3DSurface9* m_rtv;
	IDirect3DSurface9* m_dsv;

	//

	bool Create(int type, Texture& t, int w, int h, int format);
	void DoMerge(Texture* st, GSVector4* sr, GSVector4* dr, Texture& dt, bool slbg, bool mmod, GSVector4& c);
	void DoInterlace(Texture& st, Texture& dt, int shader, bool linear, float yoffset = 0);

	//

	DDCAPS m_ddcaps;
	D3DCAPS9 m_d3dcaps;
	CComPtr<IDirect3D9> m_d3d;
	CComPtr<IDirect3DDevice9> m_dev;
	CComPtr<IDirect3DSwapChain9> m_swapchain;
	Texture m_backbuffer;

public: // TODO
	D3DPRESENT_PARAMETERS m_pp;
	CComPtr<ID3DXFont> m_font;

	struct
	{
		CComPtr<IDirect3DVertexShader9> vs;
		CComPtr<IDirect3DVertexDeclaration9> il;
		CComPtr<IDirect3DPixelShader9> ps[5];
		Direct3DSamplerState9 ln;
		Direct3DSamplerState9 pt;
		Direct3DDepthStencilState9 dss;
		Direct3DBlendState9 bs;
	} m_convert;

	struct
	{
		CComPtr<IDirect3DPixelShader9> ps[2];
		Direct3DBlendState9 bs;
	} m_merge;

	struct
	{
		CComPtr<IDirect3DPixelShader9> ps[4];
	} m_interlace;

public:
	GSDevice9();
	virtual ~GSDevice9();

	bool Create(HWND hWnd, bool vsync);
	bool Reset(int w, int h, bool fs);
	bool IsLost();
	void Present(const CRect& r);
	void BeginScene();
	void EndScene();
	void Draw(LPCTSTR str);
	bool CopyOffscreen(Texture& src, const GSVector4& sr, Texture& dst, int w, int h, int format = 0);

	void ClearRenderTarget(Texture& t, const GSVector4& c);
	void ClearRenderTarget(Texture& t, DWORD c);
	void ClearDepth(Texture& t, float c);
	void ClearStencil(Texture& t, BYTE c);

	bool CreateRenderTarget(Texture& t, int w, int h, int format = 0);
	bool CreateDepthStencil(Texture& t, int w, int h, int format = 0);
	bool CreateTexture(Texture& t, int w, int h, int format = 0);
	bool CreateOffscreen(Texture& t, int w, int h, int format = 0);

	IDirect3DDevice9* operator->() {return m_dev;}
	operator IDirect3DDevice9*() {return m_dev;}

	// TODO: void IASetVertexBuffer(IDirect3DVertexBuffer9* vb, UINT count, const void* vertices, UINT stride);
	void IASetVertexBuffer(UINT count, const void* vertices, UINT stride);
	void IASetInputLayout(IDirect3DVertexDeclaration9* layout);
	void IASetPrimitiveTopology(D3DPRIMITIVETYPE topology);
	void VSSetShader(IDirect3DVertexShader9* vs, const float* vs_cb, int vs_cb_len);
	void PSSetShaderResources(IDirect3DTexture9* srv0, IDirect3DTexture9* srv1);
	void PSSetShader(IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len);
	void PSSetSamplerState(Direct3DSamplerState9* ss);
	void RSSet(int width, int height, const RECT* scissor = NULL);
	void OMSetDepthStencilState(Direct3DDepthStencilState9* dss, UINT sref);
	void OMSetBlendState(Direct3DBlendState9* bs, DWORD bf);
	void OMSetRenderTargets(IDirect3DSurface9* rtv, IDirect3DSurface9* dsv);
	void DrawPrimitive();

	template<class T> void IASetVertexBuffer(UINT count, T* vertices)
	{
		IASetVertexBuffer(count, vertices, sizeof(T));
	}

	void StretchRect(Texture& st, Texture& dt, const GSVector4& dr, bool linear = true);
	void StretchRect(Texture& st, const GSVector4& sr, Texture& dt, const GSVector4& dr, bool linear = true);
	void StretchRect(Texture& st, const GSVector4& sr, Texture& dt, const GSVector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, bool linear = true);
	void StretchRect(Texture& st, const GSVector4& sr, Texture& dt, const GSVector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, Direct3DBlendState9* bs, bool linear = true);

	HRESULT CompileShader(UINT id, LPCSTR entry, const D3DXMACRO* macro, IDirect3DVertexShader9** vs, const D3DVERTEXELEMENT9* layout, int count, IDirect3DVertexDeclaration9** il);
	HRESULT CompileShader(UINT id, LPCSTR entry, const D3DXMACRO* macro, IDirect3DPixelShader9** ps);

	virtual bool IsCurrentRGBA()
	{
		return false;
	}

	// TODO
	bool SaveToFileD24S8(IDirect3DSurface9* ds, LPCTSTR fn);
};
