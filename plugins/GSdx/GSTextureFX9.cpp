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
#include "GSTextureFX9.h"
#include "resource.h"

GSTextureFX9::GSTextureFX9()
	: m_dev(NULL)
{
}

bool GSTextureFX9::Create(GSDevice9* dev)
{
	m_dev = dev;

	VSSelector sel;
	
	sel.bppz = 0;
	sel.tme = 0;
	sel.fst = 0;
	sel.logz = 0;

	VSConstantBuffer cb;

	SetupVS(sel, &cb); // creates layout

	return true;
}

bool GSTextureFX9::CreateMskFix(GSTexture9& t, DWORD size, DWORD msk, DWORD fix)
{
	DWORD hash = (size << 20) | (msk << 10) | fix;

	if(CRBMap<DWORD, GSTexture9>::CPair* pair = m_mskfix.Lookup(hash))
	{
		t = pair->m_value;
	}
	else
	{
		if(!m_dev->CreateTexture(t, size, 1, D3DFMT_R32F))
		{
			return false;
		}

		BYTE* bits;
		int pitch;
		
		if(t.Map(&bits, pitch))
		{
			for(DWORD i = 0; i < size; i++)
			{
				((float*)bits)[i] = (float)((i & msk) | fix) / size;
			}

			t.Unmap();
		}

		m_mskfix.SetAt(hash, t);
	}

	return true;
}

bool GSTextureFX9::SetupIA(const GSVertexHW9* vertices, UINT count, D3DPRIMITIVETYPE prim)
{
	m_dev->IASetVertexBuffer(count, vertices);
	m_dev->IASetInputLayout(m_il);
	m_dev->IASetPrimitiveTopology(prim);

	return true;
}

bool GSTextureFX9::SetupVS(VSSelector sel, const VSConstantBuffer* cb)
{
	CComPtr<IDirect3DVertexShader9> vs;

	if(CRBMap<DWORD, CComPtr<IDirect3DVertexShader9> >::CPair* pair = m_vs.Lookup(sel))
	{
		vs = pair->m_value;
	}
	else
	{
		CStringA str[4];

		str[0].Format("%d", sel.bppz);
		str[1].Format("%d", sel.tme);
		str[2].Format("%d", sel.fst);
		str[3].Format("%d", sel.logz);

		D3DXMACRO macro[] =
		{
			{"VS_BPPZ", str[0]},
			{"VS_TME", str[1]},
			{"VS_FST", str[2]},
			{"VS_LOGZ", str[3]},
			{NULL, NULL},
		};

		static const D3DVERTEXELEMENT9 layout[] =
		{
			{0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
			{0, 8, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
			{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1},
			{0, 16,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
			D3DDECL_END()
		};

		CComPtr<IDirect3DVertexDeclaration9> il;

		m_dev->CompileShader(IDR_TFX9_FX, "vs_main", macro, &vs, layout, countof(layout), &il);

		if(m_il == NULL)
		{
			m_il = il;
		}

		m_vs.SetAt(sel, vs);
	}

	m_dev->VSSetShader(vs, (const float*)cb, sizeof(*cb) / sizeof(GSVector4));

	return true;
}

bool GSTextureFX9::SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel, IDirect3DTexture9* tex, IDirect3DTexture9* pal, bool psrr)
{
	m_dev->PSSetShaderResources(tex, pal);

	if(tex && psrr)
	{
		if(sel.wms == 3)
		{
			D3DSURFACE_DESC desc;
			tex->GetLevelDesc(0, &desc);

			GSTexture9 t;
			CreateMskFix(t, desc.Width, cb->UMSK, cb->UFIX);			

			(*m_dev)->SetTexture(2, t);
		}

		if(sel.wmt == 3)
		{
			D3DSURFACE_DESC desc;
			tex->GetLevelDesc(0, &desc);

			GSTexture9 t;
			CreateMskFix(t, desc.Height, cb->VMSK, cb->VFIX);			

			(*m_dev)->SetTexture(3, t);
		}
	}

	UpdatePS(sel, cb, ssel, psrr);

	return true;
}

void GSTextureFX9::UpdatePS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel, bool psrr)
{
	HRESULT hr;

	if(!psrr)
	{
		if(sel.wms == 3) sel.wms = 0;
		if(sel.wmt == 3) sel.wmt = 0;
	}

	CComPtr<IDirect3DPixelShader9> ps;

	if(CRBMap<DWORD, CComPtr<IDirect3DPixelShader9> >::CPair* pair = m_ps.Lookup(sel))
	{
		ps = pair->m_value;
	}
	else
	{
		CStringA str[12];

		str[0].Format("%d", sel.fst);
		str[1].Format("%d", sel.wms);
		str[2].Format("%d", sel.wmt);
		str[3].Format("%d", sel.bpp);
		str[4].Format("%d", sel.aem);
		str[5].Format("%d", sel.tfx);
		str[6].Format("%d", sel.tcc);
		str[7].Format("%d", sel.ate);
		str[8].Format("%d", sel.atst);
		str[9].Format("%d", sel.fog);
		str[10].Format("%d", sel.clr1);
		str[11].Format("%d", sel.rt);

		D3DXMACRO macro[] =
		{
			{"FST", str[0]},
			{"WMS", str[1]},
			{"WMT", str[2]},
			{"BPP", str[3]},
			{"AEM", str[4]},
			{"TFX", str[5]},
			{"TCC", str[6]},
			{"ATE", str[7]},
			{"ATST", str[8]},
			{"FOG", str[9]},
			{"CLR1", str[10]},
			{"RT", str[11]},
			{NULL, NULL},
		};

		hr = m_dev->CompileShader(IDR_TFX9_FX, "ps_main", macro, &ps);

		m_ps.SetAt(sel, ps);
	}

	m_dev->PSSetShader(ps, (const float*)cb, sizeof(*cb) / sizeof(GSVector4));

	Direct3DSamplerState9* ss = NULL;

	if(sel.tfx != 4)
	{
		if(sel.bpp >= 3 || sel.wms >= 3 || sel.wmt >= 3) 
		{
			ssel.min = ssel.mag = 0;
		}

		if(CRBMap<DWORD, Direct3DSamplerState9*>::CPair* pair = m_ps_ss.Lookup(ssel))
		{
			ss = pair->m_value;
		}
		else
		{
			ss = new Direct3DSamplerState9();

			memset(ss, 0, sizeof(*ss));

			ss->FilterMin[0] = ssel.min ? D3DTEXF_LINEAR : D3DTEXF_POINT;
			ss->FilterMag[0] = ssel.mag ? D3DTEXF_LINEAR : D3DTEXF_POINT;
			ss->FilterMin[1] = D3DTEXF_POINT;
			ss->FilterMag[1] = D3DTEXF_POINT;

			ss->AddressU = ssel.tau ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
			ss->AddressV = ssel.tav ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;

			m_ps_ss.SetAt(ssel, ss);
		}
	}

	m_dev->PSSetSamplerState(ss);
}

void GSTextureFX9::SetupRS(int w, int h, const RECT& scissor)
{
	m_dev->RSSet(w, h, &scissor);
}

void GSTextureFX9::SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, BYTE bf, IDirect3DSurface9* rt, IDirect3DSurface9* ds)
{
	UpdateOM(dssel, bsel, bf);

	m_dev->OMSetRenderTargets(rt, ds);
}

void GSTextureFX9::UpdateOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, BYTE bf)
{
	Direct3DDepthStencilState9* dss = NULL;

	if(CRBMap<DWORD, Direct3DDepthStencilState9*>::CPair* pair = m_om_dss.Lookup(dssel))
	{
		dss = pair->m_value;
	}
	else
	{
		dss = new Direct3DDepthStencilState9();

		memset(dss, 0, sizeof(*dss));

		if(dssel.date || dssel.fba)
		{
			dss->StencilEnable = true;
			dss->StencilReadMask = 1;
			dss->StencilWriteMask = 2;
			dss->StencilFunc = dssel.date ? D3DCMP_EQUAL : D3DCMP_ALWAYS;
			dss->StencilPassOp = dssel.fba ? D3DSTENCILOP_REPLACE : D3DSTENCILOP_KEEP;
			dss->StencilFailOp = dssel.fba ? D3DSTENCILOP_ZERO : D3DSTENCILOP_KEEP;
			dss->StencilDepthFailOp = dssel.fba ? D3DSTENCILOP_ZERO : D3DSTENCILOP_KEEP;
		}

		if(!(dssel.zte && dssel.ztst == 1 && !dssel.zwe))
		{
			static const D3DCMPFUNC ztst[] = 
			{
				D3DCMP_NEVER, 
				D3DCMP_ALWAYS, 
				D3DCMP_GREATEREQUAL, 
				D3DCMP_GREATER
			};

			dss->DepthEnable = dssel.zte;
			dss->DepthWriteMask = dssel.zwe;
			dss->DepthFunc = ztst[dssel.ztst];
		}

		m_om_dss.SetAt(dssel, dss);
	}

	m_dev->OMSetDepthStencilState(dss, 3);

	Direct3DBlendState9* bs = NULL;
	
	if(CRBMap<DWORD, Direct3DBlendState9*>::CPair* pair = m_om_bs.Lookup(bsel))
	{
		bs = pair->m_value;
	}
	else
	{
		bs = new Direct3DBlendState9();

		memset(bs, 0, sizeof(*bs));

		bs->BlendEnable = bsel.abe;

		if(bsel.abe)
		{
			// (A:Cs/Cd/0 - B:Cs/Cd/0) * C:As/Ad/FIX + D:Cs/Cd/0

			static const struct {int bogus; D3DBLENDOP op; D3DBLEND src, dst;} map[3*3*3*3] =
			{
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0000: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0001: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0002: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0010: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0011: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0012: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0020: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0021: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0022: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{1, D3DBLENDOP_SUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},			// * 0100: (Cs - Cd)*As + Cs ==> Cs*(As + 1) - Cd*As
				{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA},			// 0101: (Cs - Cd)*As + Cd ==> Cs*As + Cd*(1 - As)
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},			// 0102: (Cs - Cd)*As + 0 ==> Cs*As - Cd*As
				{1, D3DBLENDOP_SUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},		// * 0110: (Cs - Cd)*Ad + Cs ==> Cs*(Ad + 1) - Cd*Ad
				{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_INVDESTALPHA},			// 0111: (Cs - Cd)*Ad + Cd ==> Cs*Ad + Cd*(1 - Ad)
				{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},			// 0112: (Cs - Cd)*Ad + 0 ==> Cs*Ad - Cd*Ad
				{1, D3DBLENDOP_SUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},	// * 0120: (Cs - Cd)*F + Cs ==> Cs*(F + 1) - Cd*F
				{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_INVBLENDFACTOR},		// 0121: (Cs - Cd)*F + Cd ==> Cs*F + Cd*(1 - F)
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},	// 0122: (Cs - Cd)*F + 0 ==> Cs*F - Cd*F
				{1, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},					// * 0200: (Cs - 0)*As + Cs ==> Cs*(As + 1)
				{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ONE},					// 0201: (Cs - 0)*As + Cd ==> Cs*As + Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},					// 0202: (Cs - 0)*As + 0 ==> Cs*As
				{1, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},					// * 0210: (Cs - 0)*Ad + Cs ==> Cs*(As + 1)
				{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_ONE},					// 0211: (Cs - 0)*Ad + Cd ==> Cs*Ad + Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_ZERO},					// 0212: (Cs - 0)*Ad + 0 ==> Cs*Ad
				{1, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},				// * 0220: (Cs - 0)*F + Cs ==> Cs*(F + 1)
				{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ONE},				// 0221: (Cs - 0)*F + Cd ==> Cs*F + Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},				// 0222: (Cs - 0)*F + 0 ==> Cs*F
				{0, D3DBLENDOP_ADD, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA},			// 1000: (Cd - Cs)*As + Cs ==> Cd*As + Cs*(1 - As)
				{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},		// * 1001: (Cd - Cs)*As + Cd ==> Cd*(As + 1) - Cs*As
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},		// 1002: (Cd - Cs)*As + 0 ==> Cd*As - Cs*As
				{0, D3DBLENDOP_ADD, D3DBLEND_INVDESTALPHA, D3DBLEND_DESTALPHA},			// 1010: (Cd - Cs)*Ad + Cs ==> Cd*Ad + Cs*(1 - Ad)
				{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},	// * 1011: (Cd - Cs)*Ad + Cd ==> Cd*(Ad + 1) - Cs*Ad
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},	// 1012: (Cd - Cs)*Ad + 0 ==> Cd*Ad - Cs*Ad
				{0, D3DBLENDOP_ADD, D3DBLEND_INVBLENDFACTOR, D3DBLEND_BLENDFACTOR},		// 1020: (Cd - Cs)*F + Cs ==> Cd*F + Cs*(1 - F)
				{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},// * 1021: (Cd - Cs)*F + Cd ==> Cd*(F + 1) - Cs*F
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},// 1022: (Cd - Cs)*F + 0 ==> Cd*F - Cs*F
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1100: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1101: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1102: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1110: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1111: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1112: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1120: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1121: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1122: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_SRCALPHA},					// 1200: (Cd - 0)*As + Cs ==> Cs + Cd*As
				{2, D3DBLENDOP_ADD, D3DBLEND_DESTCOLOR, D3DBLEND_SRCALPHA},				// ** 1201: (Cd - 0)*As + Cd ==> Cd*(1 + As)  // ffxii main menu background glow effect
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_SRCALPHA},					// 1202: (Cd - 0)*As + 0 ==> Cd*As
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_DESTALPHA},					// 1210: (Cd - 0)*Ad + Cs ==> Cs + Cd*Ad
				{2, D3DBLENDOP_ADD, D3DBLEND_DESTCOLOR, D3DBLEND_DESTALPHA},			// ** 1211: (Cd - 0)*Ad + Cd ==> Cd*(1 + Ad)
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_DESTALPHA},					// 1212: (Cd - 0)*Ad + 0 ==> Cd*Ad
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},				// 1220: (Cd - 0)*F + Cs ==> Cs + Cd*F
				{2, D3DBLENDOP_ADD, D3DBLEND_DESTCOLOR, D3DBLEND_BLENDFACTOR},			// ** 1221: (Cd - 0)*F + Cd ==> Cd*(1 + F)
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_BLENDFACTOR},				// 1222: (Cd - 0)*F + 0 ==> Cd*F
				{0, D3DBLENDOP_ADD, D3DBLEND_INVSRCALPHA, D3DBLEND_ZERO},				// 2000: (0 - Cs)*As + Cs ==> Cs*(1 - As)
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_ONE},			// 2001: (0 - Cs)*As + Cd ==> Cd - Cs*As
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},			// 2002: (0 - Cs)*As + 0 ==> 0 - Cs*As
				{0, D3DBLENDOP_ADD, D3DBLEND_INVDESTALPHA, D3DBLEND_ZERO},				// 2010: (0 - Cs)*Ad + Cs ==> Cs*(1 - Ad)
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_ONE},			// 2011: (0 - Cs)*Ad + Cd ==> Cd - Cs*Ad
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_ZERO},			// 2012: (0 - Cs)*Ad + 0 ==> 0 - Cs*Ad
				{0, D3DBLENDOP_ADD, D3DBLEND_INVBLENDFACTOR, D3DBLEND_ZERO},			// 2020: (0 - Cs)*F + Cs ==> Cs*(1 - F)
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_ONE},		// 2021: (0 - Cs)*F + Cd ==> Cd - Cs*F
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},		// 2022: (0 - Cs)*F + 0 ==> 0 - Cs*F
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_SRCALPHA},				// 2100: (0 - Cd)*As + Cs ==> Cs - Cd*As
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVSRCALPHA},				// 2101: (0 - Cd)*As + Cd ==> Cd*(1 - As)
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ZERO, D3DBLEND_SRCALPHA},				// 2102: (0 - Cd)*As + 0 ==> 0 - Cd*As
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_DESTALPHA},				// 2110: (0 - Cd)*Ad + Cs ==> Cs - Cd*Ad
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVDESTALPHA},				// 2111: (0 - Cd)*Ad + Cd ==> Cd*(1 - Ad)
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_DESTALPHA},				// 2112: (0 - Cd)*Ad + 0 ==> 0 - Cd*Ad
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},			// 2120: (0 - Cd)*F + Cs ==> Cs - Cd*F
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVBLENDFACTOR},			// 2121: (0 - Cd)*F + Cd ==> Cd*(1 - F)
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},			// 2122: (0 - Cd)*F + 0 ==> 0 - Cd*F
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2200: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2201: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2202: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2210: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2211: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2212: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2220: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2221: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2222: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
			};

			// bogus: 0100, 0110, 0120, 0200, 0210, 0220, 1001, 1011, 1021

			// tricky: 1201, 1211, 1221
			//
			// Source.rgb = float3(1, 1, 1);
			// 1201 Cd*(1 + As) => Source * Dest color + Dest * Source alpha
			// 1211 Cd*(1 + Ad) => Source * Dest color + Dest * Dest alpha
			// 1221 Cd*(1 + F) => Source * Dest color + Dest * Factor

			int i = ((bsel.a * 3 + bsel.b) * 3 + bsel.c) * 3 + bsel.d;

			bs->BlendOp = map[i].op;
			bs->SrcBlend = map[i].src;
			bs->DestBlend = map[i].dst;
			bs->BlendOpAlpha = D3DBLENDOP_ADD;
			bs->SrcBlendAlpha = D3DBLEND_ONE;
			bs->DestBlendAlpha = D3DBLEND_ZERO;

			if(map[i].bogus == 1)
			{
				ASSERT(0);

				(bsel.a == 0 ? bs->SrcBlend : bs->DestBlend) = D3DBLEND_ONE;
			}
		}

		if(bsel.wr) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_RED;
		if(bsel.wg) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_GREEN;
		if(bsel.wb) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_BLUE;
		if(bsel.wa) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_ALPHA;

		m_om_bs.SetAt(bsel, bs);
	}

	m_dev->OMSetBlendState(bs, 0x010101 * bf);
}
