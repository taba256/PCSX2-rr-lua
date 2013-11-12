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
#include "GSRendererHW9.h"
#include "GSCrc.h"
#include "resource.h"

GSRendererHW9::GSRendererHW9(BYTE* base, bool mt, void (*irq)(), int nloophack, const GSRendererSettings& rs)
	: GSRendererHW<Device, Vertex, TextureCache>(base, mt, irq, nloophack, rs, false)
{
	m_fba.enabled = !!AfxGetApp()->GetProfileInt(_T("Settings"), _T("fba"), TRUE);
	m_logz = !!AfxGetApp()->GetProfileInt(_T("Settings"), _T("logz"), FALSE);

	InitVertexKick<GSRendererHW9>();
}

bool GSRendererHW9::Create(LPCTSTR title)
{
	if(!__super::Create(title))
		return false;

	if(!m_tfx.Create(&m_dev))
		return false;

	//

	memset(&m_date.dss, 0, sizeof(m_date.dss));

	m_date.dss.StencilEnable = true;
	m_date.dss.StencilReadMask = 1;
	m_date.dss.StencilWriteMask = 1;
	m_date.dss.StencilFunc = D3DCMP_ALWAYS;
	m_date.dss.StencilPassOp = D3DSTENCILOP_REPLACE;

	memset(&m_date.bs, 0, sizeof(m_date.bs));

	//

	memset(&m_fba.dss, 0, sizeof(m_fba.dss));

	m_fba.dss.StencilEnable = true;
	m_fba.dss.StencilReadMask = 2;
	m_fba.dss.StencilWriteMask = 2;
	m_fba.dss.StencilFunc = D3DCMP_EQUAL;
	m_fba.dss.StencilPassOp = D3DSTENCILOP_ZERO;
	m_fba.dss.StencilFailOp = D3DSTENCILOP_ZERO;
	m_fba.dss.StencilDepthFailOp = D3DSTENCILOP_ZERO;

	memset(&m_fba.bs, 0, sizeof(m_fba.bs));

	m_fba.bs.RenderTargetWriteMask = D3DCOLORWRITEENABLE_ALPHA;

	//

	return true;
}

template<DWORD prim, DWORD tme, DWORD fst> 
void GSRendererHW9::VertexKick(bool skip)
{
	Vertex& dst = m_vl.AddTail();

	dst.p.x = (float)m_v.XYZ.X;
	dst.p.y = (float)m_v.XYZ.Y;
	dst.p.z = (float)m_v.XYZ.Z;

	dst.c0 = m_v.RGBAQ.ai32[0];
	dst.c1 = m_v.FOG.ai32[1];

	if(tme)
	{
		if(fst)
		{
			GSVector4::storel(&dst.t, m_v.GetUV());
		}
		else
		{
			dst.t.x = m_v.ST.S;
			dst.t.y = m_v.ST.T;
			dst.p.w = m_v.RGBAQ.Q;
		}
	}

	DWORD count = 0;
	
	if(Vertex* v = DrawingKick<prim>(skip, count))
	{
		GSVector4 scissor = m_context->scissor.dx9;

		GSVector4 pmin, pmax;

		switch(prim)
		{
		case GS_POINTLIST:
			pmin = v[0].p;
			pmax = v[0].p;
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
		case GS_SPRITE:
			pmin = v[0].p.minv(v[1].p);
			pmax = v[0].p.maxv(v[1].p);
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			pmin = v[0].p.minv(v[1].p).minv(v[2].p);
			pmax = v[0].p.maxv(v[1].p).maxv(v[2].p);
			break;
		}

		GSVector4 test = (pmax < scissor) | (pmin > scissor.zwxy());

		if(test.mask() & 3)
		{
			return;
		}

		switch(prim)
		{
		case GS_POINTLIST:
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
			if(PRIM->IIP == 0) {v[0].c0 = v[1].c0;}
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			if(PRIM->IIP == 0) {v[0].c0 = v[1].c0 = v[2].c0;}
			break;
		case GS_SPRITE:
			if(PRIM->IIP == 0) {v[0].c0 = v[1].c0;}
			v[0].p.z = v[1].p.z;
			v[0].p.w = v[1].p.w;
			v[0].c1 = v[1].c1;
			v[2] = v[1];
			v[3] = v[1];
			v[1].p.y = v[0].p.y;
			v[1].t.y = v[0].t.y;
			v[2].p.x = v[0].p.x;
			v[2].t.x = v[0].t.x;
			v[4] = v[1];
			v[5] = v[2];
			count += 4;
			break;
		}

		m_count += count;
	}
}

void GSRendererHW9::Draw(int prim, Texture& rt, Texture& ds, GSTextureCache<Device>::GSTexture* tex)
{
	GSDrawingEnvironment& env = m_env;
	GSDrawingContext* context = m_context;

	D3DPRIMITIVETYPE topology;
	int prims = 0;

	switch(prim)
	{
	case GS_POINTLIST:
		topology = D3DPT_POINTLIST;
		prims = m_count;
		break;
	case GS_LINELIST: 
	case GS_LINESTRIP:
		topology = D3DPT_LINELIST;
		prims = m_count / 2;
		break;
	case GS_TRIANGLELIST: 
	case GS_TRIANGLESTRIP: 
	case GS_TRIANGLEFAN: 
	case GS_SPRITE:
		topology = D3DPT_TRIANGLELIST;
		prims = m_count / 3;
		break;
	default:
		__assume(0);
	}

	m_perfmon.Put(GSPerfMon::Prim, prims);
	m_perfmon.Put(GSPerfMon::Draw, 1);

	// date

	SetupDATE(rt, ds);

	//

	m_dev.BeginScene();

	m_dev->SetRenderState(D3DRS_SHADEMODE, PRIM->IIP ? D3DSHADE_GOURAUD : D3DSHADE_FLAT); // TODO

	// om

	GSTextureFX9::OMDepthStencilSelector om_dssel;

	om_dssel.zte = context->TEST.ZTE;
	om_dssel.ztst = context->TEST.ZTST;
	om_dssel.zwe = !context->ZBUF.ZMSK;
	om_dssel.date = context->FRAME.PSM != PSM_PSMCT24 ? context->TEST.DATE : 0;
	om_dssel.fba = m_fba.enabled ? context->FBA.FBA : 0;

	GSTextureFX9::OMBlendSelector om_bsel;

	om_bsel.abe = PRIM->ABE || (prim == 1 || prim == 2) && PRIM->AA1;
	om_bsel.a = context->ALPHA.A;
	om_bsel.b = context->ALPHA.B;
	om_bsel.c = context->ALPHA.C;
	om_bsel.d = context->ALPHA.D;
	om_bsel.wr = (context->FRAME.FBMSK & 0x000000ff) != 0x000000ff;
	om_bsel.wg = (context->FRAME.FBMSK & 0x0000ff00) != 0x0000ff00;
	om_bsel.wb = (context->FRAME.FBMSK & 0x00ff0000) != 0x00ff0000;
	om_bsel.wa = (context->FRAME.FBMSK & 0xff000000) != 0xff000000;

	BYTE bf = context->ALPHA.FIX >= 0x80 ? 0xff : (BYTE)(context->ALPHA.FIX * 2);

	// vs

	GSTextureFX9::VSSelector vs_sel;

	vs_sel.bppz = 0;
	vs_sel.tme = PRIM->TME;
	vs_sel.fst = PRIM->FST;
	vs_sel.logz = m_logz ? 1 : 0;

	if(om_dssel.zte && om_dssel.ztst > 0 && om_dssel.zwe)
	{
		if(context->ZBUF.PSM == PSM_PSMZ24)
		{
			if(WrapZ(0xffffff))
			{
				vs_sel.bppz = 1;
				om_dssel.ztst = 1;
			}
		}
		else if(context->ZBUF.PSM == PSM_PSMZ16 || context->ZBUF.PSM == PSM_PSMZ16S)
		{
			if(WrapZ(0xffff))
			{
				vs_sel.bppz = 2;
				om_dssel.ztst = 1;
			}
		}
	}

	GSTextureFX9::VSConstantBuffer vs_cb;

	float sx = 2.0f * rt.m_scale.x / (rt.GetWidth() * 16);
	float sy = 2.0f * rt.m_scale.y / (rt.GetHeight() * 16);
	float ox = (float)(int)context->XYOFFSET.OFX;
	float oy = (float)(int)context->XYOFFSET.OFY;

	vs_cb.VertexScale = GSVector4(sx, -sy, 1.0f / UINT_MAX, 0.0f);
	vs_cb.VertexOffset = GSVector4(ox * sx + 1, -(oy * sy + 1), 0.0f, -1.0f);
	vs_cb.TextureScale = GSVector2(1.0f, 1.0f);

	if(PRIM->TME && PRIM->FST)
	{
		vs_cb.TextureScale.x = 1.0f / (16 << context->TEX0.TW);
		vs_cb.TextureScale.y = 1.0f / (16 << context->TEX0.TH);
	}

	// ps

	GSTextureFX9::PSSelector ps_sel;

	ps_sel.fst = PRIM->FST;
	ps_sel.wms = context->CLAMP.WMS;
	ps_sel.wmt = context->CLAMP.WMT;
	ps_sel.bpp = 0;
	ps_sel.aem = env.TEXA.AEM;
	ps_sel.tfx = context->TEX0.TFX;
	ps_sel.tcc = context->TEX0.TCC;
	ps_sel.ate = context->TEST.ATE;
	ps_sel.atst = context->TEST.ATST;
	ps_sel.fog = PRIM->FGE;
	ps_sel.clr1 = om_bsel.abe && om_bsel.a == 1 && om_bsel.b == 2 && om_bsel.d == 1;
	ps_sel.rt = tex && tex->m_rendered;

	GSTextureFX9::PSSamplerSelector ps_ssel;

	ps_ssel.min = m_filter == 2 ? (context->TEX1.MMIN & 1) : m_filter;
	ps_ssel.mag = m_filter == 2 ? (context->TEX1.MMAG & 1) : m_filter;
	ps_ssel.tau = 0;
	ps_ssel.tav = 0;

	GSTextureFX9::PSConstantBuffer ps_cb;

	ps_cb.FogColor = GSVector4(env.FOGCOL.FCR, env.FOGCOL.FCG, env.FOGCOL.FCB, 0) / 255.0f;
	ps_cb.TA0 = (float)(int)env.TEXA.TA0 / 255;
	ps_cb.TA1 = (float)(int)env.TEXA.TA1 / 255;
	ps_cb.AREF = (float)(int)context->TEST.AREF / 255;

	if(context->TEST.ATST == 2 || context->TEST.ATST == 5)
	{
		ps_cb.AREF -= 0.9f/256;
	}
	else if(context->TEST.ATST == 3 || context->TEST.ATST == 6)
	{
		ps_cb.AREF += 0.9f/256;
	}

	if(tex)
	{
		ps_sel.bpp = tex->m_bpp2;

		switch(context->CLAMP.WMS)
		{
		case 0: 
			ps_ssel.tau = 1; 
			break;
		case 1: 
			ps_ssel.tau = 0; 
			break;
		case 2: 
			ps_cb.MINU = ((float)(int)context->CLAMP.MINU + 0.5f) / (1 << context->TEX0.TW);
			ps_cb.MAXU = ((float)(int)context->CLAMP.MAXU) / (1 << context->TEX0.TW);
			ps_ssel.tau = 0; 
			break;
		case 3: 
			ps_cb.UMSK = context->CLAMP.MINU;
			ps_cb.UFIX = context->CLAMP.MAXU;
			ps_ssel.tau = 1; 
			break;
		default: 
			__assume(0);
		}

		switch(context->CLAMP.WMT)
		{
		case 0: 
			ps_ssel.tav = 1; 
			break;
		case 1: 
			ps_ssel.tav = 0; 
			break;
		case 2: 
			ps_cb.MINV = ((float)(int)context->CLAMP.MINV + 0.5f) / (1 << context->TEX0.TH);
			ps_cb.MAXV = ((float)(int)context->CLAMP.MAXV) / (1 << context->TEX0.TH);
			ps_ssel.tav = 0; 
			break;
		case 3: 
			ps_cb.VMSK = context->CLAMP.MINV;
			ps_cb.VFIX = context->CLAMP.MAXV;
			ps_ssel.tav = 1; 
			break;
		default: 
			__assume(0);
		}

		float w = (float)tex->m_texture.GetWidth();
		float h = (float)tex->m_texture.GetHeight();

		ps_cb.WH = GSVector2(w, h);
		ps_cb.rWrH = GSVector2(1.0f / w, 1.0f / h);
	}
	else
	{
		ps_sel.tfx = 4;
	}

	// rs

	int w = rt.GetWidth();
	int h = rt.GetHeight();

	CRect scissor = (CRect)GSVector4i(GSVector4(rt.m_scale).xyxy() * context->scissor.in) & CRect(0, 0, w, h);

	//

	m_tfx.SetupOM(om_dssel, om_bsel, bf, rt, ds);
	m_tfx.SetupIA(m_vertices, m_count, topology);
	m_tfx.SetupVS(vs_sel, &vs_cb);
	m_tfx.SetupPS(ps_sel, &ps_cb, ps_ssel, 
		tex ? (IDirect3DTexture9*)tex->m_texture : NULL, 
		tex ? (IDirect3DTexture9*)tex->m_palette : NULL, 
		m_psrr);
	m_tfx.SetupRS(w, h, scissor);

	// draw

	if(context->TEST.DoFirstPass())
	{
		m_dev.DrawPrimitive();
	}

	if(context->TEST.DoSecondPass())
	{
		ASSERT(!env.PABE.PABE);

		static const DWORD iatst[] = {1, 0, 5, 6, 7, 2, 3, 4};

		ps_sel.atst = iatst[ps_sel.atst];

		m_tfx.UpdatePS(ps_sel, &ps_cb, ps_ssel, m_psrr);

		bool z = om_dssel.zwe;
		bool r = om_bsel.wr;
		bool g = om_bsel.wg;
		bool b = om_bsel.wb;
		bool a = om_bsel.wa;

		switch(context->TEST.AFAIL)
		{
		case 0: z = r = g = b = a = false; break; // none
		case 1: z = false; break; // rgba
		case 2: r = g = b = a = false; break; // z
		case 3: z = a = false; break; // rgb
		default: __assume(0);
		}

		if(z || r || g || b || a)
		{
			om_dssel.zwe = z;
			om_bsel.wr = r;
			om_bsel.wg = g;
			om_bsel.wb = b;
			om_bsel.wa = a;

			m_tfx.UpdateOM(om_dssel, om_bsel, bf);

			m_dev.DrawPrimitive();
		}
	}

	m_dev.EndScene();

	if(om_dssel.fba) UpdateFBA(rt);
}

bool GSRendererHW9::WrapZ(float maxz)
{
	// should only run once if z values are in the z buffer range

	for(int i = 0, j = m_count; i < j; i++)
	{
		if(m_vertices[i].p.z <= maxz)
		{
			return false;
		}
	}

	return true;
}

void GSRendererHW9::SetupDATE(Texture& rt, Texture& ds)
{
	if(!m_context->TEST.DATE) return; // || (::GetAsyncKeyState(VK_CONTROL) & 0x8000)

	// sfex3 (after the capcom logo), vf4 (first menu fading in), ffxii shadows, rumble roses shadows

	GSVector4 mm;

	// TODO

	mm = GSVector4(-1, -1, 1, 1);
/*
	MinMaxXY(mm);

	int w = rt.GetWidth();
	int h = rt.GetHeight();

	float sx = 2.0f * rt.m_scale.x / (w * 16);
	float sy = 2.0f * rt.m_scale.y / (h * 16);	
	float ox = (float)(int)m_context->XYOFFSET.OFX;
	float oy = (float)(int)m_context->XYOFFSET.OFY;

	mm.x = (mm.x - ox) * sx - 1;
	mm.y = (mm.y - oy) * sy - 1;
	mm.z = (mm.z - ox) * sx - 1;
	mm.w = (mm.w - oy) * sy - 1;

	if(mm.x < -1) mm.x = -1;
	if(mm.y < -1) mm.y = -1;
	if(mm.z > +1) mm.z = +1;
	if(mm.w > +1) mm.w = +1;
*/
	GSVector4 uv = (mm + 1.0f) / 2.0f;

	//

	m_dev.BeginScene();

	// om

	GSTexture9 tmp;

	m_dev.CreateRenderTarget(tmp, rt.GetWidth(), rt.GetHeight());

	m_dev.OMSetRenderTargets(tmp, ds);
	m_dev.OMSetDepthStencilState(&m_date.dss, 1);
	m_dev.OMSetBlendState(&m_date.bs, 0);

	m_dev->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 0, 0);

	// ia

	GSVertexPT1 vertices[] =
	{
		{GSVector4(mm.x, -mm.y, 0.5f, 1.0f), GSVector2(uv.x, uv.y)},
		{GSVector4(mm.z, -mm.y, 0.5f, 1.0f), GSVector2(uv.z, uv.y)},
		{GSVector4(mm.x, -mm.w, 0.5f, 1.0f), GSVector2(uv.x, uv.w)},
		{GSVector4(mm.z, -mm.w, 0.5f, 1.0f), GSVector2(uv.z, uv.w)},
	};

	m_dev.IASetVertexBuffer(4, vertices);
	m_dev.IASetInputLayout(m_dev.m_convert.il);
	m_dev.IASetPrimitiveTopology(D3DPT_TRIANGLESTRIP);

	// vs

	m_dev.VSSetShader(m_dev.m_convert.vs, NULL, 0);

	// ps

	m_dev.PSSetShaderResources(rt, NULL);
	m_dev.PSSetShader(m_dev.m_convert.ps[m_context->TEST.DATM ? 2 : 3], NULL, 0);
	m_dev.PSSetSamplerState(&m_dev.m_convert.pt);

	// rs

	m_dev.RSSet(tmp.GetWidth(), tmp.GetHeight());

	//

	m_dev.DrawPrimitive();

	//

	m_dev.EndScene();

	m_dev.Recycle(tmp);
}

void GSRendererHW9::UpdateFBA(Texture& rt)
{
	m_dev.BeginScene();

	// om

	m_dev.OMSetDepthStencilState(&m_fba.dss, 2);
	m_dev.OMSetBlendState(&m_fba.bs, 0);

	// vs

	m_dev.VSSetShader(NULL, NULL, 0);

	// ps

	m_dev.PSSetShader(m_dev.m_convert.ps[4], NULL, 0);

	//

	int w = rt.GetWidth();
	int h = rt.GetHeight();

	GSVertexP vertices[] =
	{
		{GSVector4(0, 0, 0, 0)},
		{GSVector4(w, 0, 0, 0)},
		{GSVector4(0, h, 0, 0)},
		{GSVector4(w, h, 0, 0)},
	};

	m_dev->SetFVF(D3DFVF_XYZRHW);
	
	m_dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(vertices[0]));

	// 

	m_dev.EndScene();
}
