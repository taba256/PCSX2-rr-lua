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

#include "GSState.h"
#include "GSRasterizer.h"
#include "GSScanlineEnvironment.h"
#include "GSSetupPrimCodeGenerator.h"
#include "GSDrawScanlineCodeGenerator.h"
#include "GSAlignedClass.h"

class GSDrawScanline : public GSAlignedClass<16>, public IDrawScanline
{
	GSScanlineEnvironment m_env;

	//

	class GSSetupPrimMap : public GSCodeGeneratorFunctionMap<GSSetupPrimCodeGenerator, UINT64, SetupPrimStaticPtr>
	{
		GSScanlineEnvironment& m_env;

	public:
		GSSetupPrimMap(GSScanlineEnvironment& env);
		GSSetupPrimCodeGenerator* Create(UINT64 key, void* ptr, size_t maxsize);
	} m_sp;

	SetupPrimStaticPtr m_spf;

	void SetupPrim(const GSVertexSW* vertices, const GSVertexSW& dscan);

	//

	class GSDrawScanlineMap : public GSCodeGeneratorFunctionMap<GSDrawScanlineCodeGenerator, UINT64, DrawScanlineStaticPtr>
	{
		GSScanlineEnvironment& m_env;

	public:
		GSDrawScanlineMap(GSScanlineEnvironment& env);
		GSDrawScanlineCodeGenerator* Create(UINT64 key, void* ptr, size_t maxsize);
	} m_ds;

	//

	void DrawSolidRect(const GSVector4i& r, const GSVertexSW& v);

	template<class T, bool masked> 
	void DrawSolidRectT(const GSVector4i* row, int* col, const GSVector4i& r, DWORD c, DWORD m);

	template<class T, bool masked> 
	__forceinline void FillRect(const GSVector4i* row, int* col, const GSVector4i& r, DWORD c, DWORD m);

	template<class T, bool masked> 
	__forceinline void FillBlock(const GSVector4i* row, int* col, const GSVector4i& r, const GSVector4i& c, const GSVector4i& m);

protected:
	GSState* m_state;
	int m_id;

public:
	GSDrawScanline(GSState* state, int id);
	virtual ~GSDrawScanline();

	// IDrawScanline

	void BeginDraw(const GSRasterizerData* data, Functions* f);
	void EndDraw(const GSRasterizerStats& stats);
	void PrintStats() {m_ds.PrintStats();}
};
