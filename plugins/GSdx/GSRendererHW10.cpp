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
#include "GSRendererHW10.h"
#include "GSCrc.h"
#include "resource.h"

GSRendererHW10::GSRendererHW10(BYTE* base, bool mt, void (*irq)(), int nloophack, const GSRendererSettings& rs)
	: GSRendererHW<Device, Vertex, TextureCache>(base, mt, irq, nloophack, rs, true)
{
	InitVertexKick<GSRendererHW10>();
}

bool GSRendererHW10::Create(LPCTSTR title)
{
	if(!__super::Create(title))
		return false;

	if(!m_tfx.Create(&m_dev))
		return false;

	//

	D3D10_DEPTH_STENCIL_DESC dsd;

	memset(&dsd, 0, sizeof(dsd));

	dsd.DepthEnable = false;
	dsd.StencilEnable = true;
	dsd.StencilReadMask = 1;
	dsd.StencilWriteMask = 1;
	dsd.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
	dsd.FrontFace.StencilPassOp = D3D10_STENCIL_OP_REPLACE;
	dsd.FrontFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
	dsd.BackFace.StencilPassOp = D3D10_STENCIL_OP_REPLACE;
	dsd.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;

	m_dev->CreateDepthStencilState(&dsd, &m_date.dss);

	D3D10_BLEND_DESC bd;

	memset(&bd, 0, sizeof(bd));

	m_dev->CreateBlendState(&bd, &m_date.bs);

	//

	return true;
}

template<DWORD prim, DWORD tme, DWORD fst> 
void GSRendererHW10::VertexKick(bool skip)
{
	Vertex& dst = m_vl.AddTail();

	dst.m128i[0] = m_v.m128i[0];
	dst.m128i[1] = m_v.m128i[1];

	if(tme && fst)
	{
		GSVector4::storel(&dst.ST, m_v.GetUV());
	}

	DWORD count = 0;
	
	if(Vertex* v = DrawingKick<prim>(skip, count))
	{
		GSVector4i scissor = m_context->scissor.dx10;

		#if _M_SSE >= 0x401

		GSVector4i pmin, pmax, v0, v1, v2;

		switch(prim)
		{
		case GS_POINTLIST:
			v0 = GSVector4i::load((int)v[0].p.xy).upl16();
			pmin = v0;
			pmax = v0;
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
		case GS_SPRITE:
			v0 = GSVector4i::load((int)v[0].p.xy);
			v1 = GSVector4i::load((int)v[1].p.xy);
			pmin = v0.min_u16(v1).upl16();
			pmax = v0.max_u16(v1).upl16();
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			v0 = GSVector4i::load((int)v[0].p.xy);
			v1 = GSVector4i::load((int)v[1].p.xy);
			v2 = GSVector4i::load((int)v[2].p.xy);
			pmin = v0.min_u16(v1).min_u16(v2).upl16();
			pmax = v0.max_u16(v1).max_u16(v2).upl16();
			break;
		}

		GSVector4i test = (pmax < scissor) | (pmin > scissor.zwxy());

		if(test.mask() & 0xff)
		{
			return;
		}

		#else

		switch(prim)
		{
		case GS_POINTLIST:
			if(v[0].p.x < scissor.x 
			|| v[0].p.x > scissor.z
			|| v[0].p.y < scissor.y 
			|| v[0].p.y > scissor.w)
			{
				return;
			}
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
		case GS_SPRITE:
			if(v[0].p.x < scissor.x && v[1].p.x < scissor.x
			|| v[0].p.x > scissor.z && v[1].p.x > scissor.z
			|| v[0].p.y < scissor.y && v[1].p.y < scissor.y
			|| v[0].p.y > scissor.w && v[1].p.y > scissor.w)
			{
				return;
			}
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			if(v[0].p.x < scissor.x && v[1].p.x < scissor.x && v[2].p.x < scissor.x
			|| v[0].p.x > scissor.z && v[1].p.x > scissor.z && v[2].p.x > scissor.z
			|| v[0].p.y < scissor.y && v[1].p.y < scissor.y && v[2].p.y < scissor.y
			|| v[0].p.y > scissor.w && v[1].p.y > scissor.w && v[2].p.y > scissor.w)
			{
				return;
			}
			break;
		}

		#endif

		m_count += count;
	}
}

void GSRendererHW10::Draw(int prim, Texture& rt, Texture& ds, GSTextureCache<Device>::GSTexture* tex)
{
	GSDrawingEnvironment& env = m_env;
	GSDrawingContext* context = m_context;
/*
	if(s_dump)
	{
		TRACE(_T("\n"));

		TRACE(_T("PRIM = %d, ZMSK = %d, ZTE = %d, ZTST = %d, ATE = %d, ATST = %d, AFAIL = %d, AREF = %02x\n"), 
			PRIM->PRIM, context->ZBUF.ZMSK, 
			context->TEST.ZTE, context->TEST.ZTST,
			context->TEST.ATE, context->TEST.ATST, context->TEST.AFAIL, context->TEST.AREF);

		for(int i = 0; i < m_count; i++)
		{
			TRACE(_T("[%d] %3.0f %3.0f %3.0f %3.0f\n"), i, (float)m_vertices[i].p.x / 16, (float)m_vertices[i].p.y / 16, (float)m_vertices[i].p.z, (float)m_vertices[i].a);
		}
	}
*/
	D3D10_PRIMITIVE_TOPOLOGY topology;
	int prims = 0;

	switch(prim)
	{
	case GS_POINTLIST:
		topology = D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
		prims = m_count;
		break;
	case GS_LINELIST: 
	case GS_LINESTRIP:
	case GS_SPRITE:
		topology = D3D10_PRIMITIVE_TOPOLOGY_LINELIST;
		prims = m_count / 2;
		break;
	case GS_TRIANGLELIST: 
	case GS_TRIANGLESTRIP: 
	case GS_TRIANGLEFAN: 
		topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
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

	// om

	GSTextureFX10::OMDepthStencilSelector om_dssel;

	om_dssel.zte = context->TEST.ZTE;
	om_dssel.ztst = context->TEST.ZTST;
	om_dssel.zwe = !context->ZBUF.ZMSK;
	om_dssel.date = context->FRAME.PSM != PSM_PSMCT24 ? context->TEST.DATE : 0;

	GSTextureFX10::OMBlendSelector om_bsel;

	om_bsel.abe = PRIM->ABE || (prim == 1 || prim == 2) && PRIM->AA1;
	om_bsel.a = context->ALPHA.A;
	om_bsel.b = context->ALPHA.B;
	om_bsel.c = context->ALPHA.C;
	om_bsel.d = context->ALPHA.D;
	om_bsel.wr = (context->FRAME.FBMSK & 0x000000ff) != 0x000000ff;
	om_bsel.wg = (context->FRAME.FBMSK & 0x0000ff00) != 0x0000ff00;
	om_bsel.wb = (context->FRAME.FBMSK & 0x00ff0000) != 0x00ff0000;
	om_bsel.wa = (context->FRAME.FBMSK & 0xff000000) != 0xff000000;

	float bf = (float)(int)context->ALPHA.FIX / 0x80;

	// vs

	GSTextureFX10::VSSelector vs_sel;

	vs_sel.bpp = 0;
	vs_sel.bppz = 0;
	vs_sel.tme = PRIM->TME;
	vs_sel.fst = PRIM->FST;
	vs_sel.prim = prim;

	if(tex)
	{
		vs_sel.bpp = tex->m_bpp2;
	}

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

	GSTextureFX10::VSConstantBuffer vs_cb;

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

	// gs

	GSTextureFX10::GSSelector gs_sel;

	gs_sel.iip = PRIM->IIP;
	gs_sel.prim = GSUtil::GetPrimClass(prim);

	// ps

	GSTextureFX10::PSSelector ps_sel;

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
	ps_sel.fba = context->FBA.FBA;
	ps_sel.aout = context->FRAME.PSM == PSM_PSMCT16 || context->FRAME.PSM == PSM_PSMCT16S || (context->FRAME.FBMSK & 0xff000000) == 0x7f000000 ? 1 : 0;

	GSTextureFX10::PSSamplerSelector ps_ssel;

	ps_ssel.min = m_filter == 2 ? (context->TEX1.MMIN & 1) : m_filter;
	ps_ssel.mag = m_filter == 2 ? (context->TEX1.MMAG & 1) : m_filter;
	ps_ssel.tau = 0;
	ps_ssel.tav = 0;

	GSTextureFX10::PSConstantBuffer ps_cb;

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
	m_tfx.SetupGS(gs_sel);
	m_tfx.SetupPS(ps_sel, &ps_cb, ps_ssel, 
		tex ? (ID3D10ShaderResourceView*)tex->m_texture : NULL, 
		tex ? (ID3D10ShaderResourceView*)tex->m_palette : NULL);
	m_tfx.SetupRS(w, h, scissor);

	// draw

	if(context->TEST.DoFirstPass())
	{
		m_tfx.Draw();
	}

	if(context->TEST.DoSecondPass())
	{
		ASSERT(!env.PABE.PABE);

		static const DWORD iatst[] = {1, 0, 5, 6, 7, 2, 3, 4};

		ps_sel.atst = iatst[ps_sel.atst];

		m_tfx.UpdatePS(ps_sel, &ps_cb, ps_ssel);

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

			m_tfx.Draw();
		}
	}

	m_dev.EndScene();
}

bool GSRendererHW10::WrapZ(DWORD maxz)
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

void GSRendererHW10::SetupDATE(Texture& rt, Texture& ds)
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

	GSTexture10 tmp;

	m_dev.CreateRenderTarget(tmp, rt.GetWidth(), rt.GetHeight());

	m_dev.OMSetRenderTargets(tmp, ds);
	m_dev.OMSetDepthStencilState(m_date.dss, 1);
	m_dev.OMSetBlendState(m_date.bs, 0);

	m_dev->ClearDepthStencilView(ds, D3D10_CLEAR_STENCIL, 0, 0);

	// ia

	GSVertexPT1 vertices[] =
	{
		{GSVector4(mm.x, -mm.y, 0.5f, 1.0f), GSVector2(uv.x, uv.y)},
		{GSVector4(mm.z, -mm.y, 0.5f, 1.0f), GSVector2(uv.z, uv.y)},
		{GSVector4(mm.x, -mm.w, 0.5f, 1.0f), GSVector2(uv.x, uv.w)},
		{GSVector4(mm.z, -mm.w, 0.5f, 1.0f), GSVector2(uv.z, uv.w)},
	};

	D3D10_BOX box = {0, 0, 0, sizeof(vertices), 1, 1};
	m_dev->UpdateSubresource(m_dev.m_convert.vb, 0, &box, vertices, 0, 0);

	m_dev.IASetVertexBuffer(m_dev.m_convert.vb, sizeof(vertices[0]));
	m_dev.IASetInputLayout(m_dev.m_convert.il);
	m_dev.IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// vs

	m_dev.VSSetShader(m_dev.m_convert.vs, NULL);

	// gs

	m_dev.GSSetShader(NULL);

	// ps

	m_dev.PSSetShaderResources(rt, NULL);
	m_dev.PSSetShader(m_dev.m_convert.ps[m_context->TEST.DATM ? 2 : 3], NULL);
	m_dev.PSSetSamplerState(m_dev.m_convert.pt, NULL);

	// rs

	m_dev.RSSet(tmp.GetWidth(), tmp.GetHeight());

	// set

	m_dev.DrawPrimitive(countof(vertices));

	//

	m_dev.EndScene();

	m_dev.Recycle(tmp);
}
