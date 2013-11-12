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

#include "GSScanlineEnvironment.h"
#include "xbyak/xbyak.h"
#include "xbyak/xbyak_util.h"

using namespace Xbyak;

class GSSetupPrimCodeGenerator : public CodeGenerator
{
	void operator = (const GSSetupPrimCodeGenerator&);

	static const GSVector4 m_shift[5];

	util::Cpu m_cpu;

	GSScanlineEnvironment& m_env;

	struct {DWORD z:1, f:1, t:1, c:1;} m_en;

	void Generate();

	void Depth();
	void Texture();
	void Color();

public:
	GSSetupPrimCodeGenerator(GSScanlineEnvironment& env, void* ptr, size_t maxsize);
};