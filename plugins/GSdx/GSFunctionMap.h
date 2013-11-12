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

#include "GS.h"

struct GSRasterizerStats
{
	__int64 ticks;
	int prims, pixels;

	GSRasterizerStats() 
	{
		Reset();
	}

	void Reset() 
	{
		ticks = 0;
		pixels = prims = 0;
	}
};

template<class KEY, class VALUE> class GSFunctionMap
{
protected:
	struct ActivePtr
	{
		UINT64 frame, frames;
		__int64 ticks, pixels;
		VALUE f;
	};

	CRBMap<KEY, VALUE> m_map;
	CRBMap<KEY, ActivePtr*> m_map_active;
	ActivePtr* m_active;

	virtual VALUE GetDefaultFunction(KEY key) = 0;

public:
	GSFunctionMap() 
		: m_active(NULL) 
	{
	}

	virtual ~GSFunctionMap()
	{
		POSITION pos = m_map_active.GetHeadPosition();

		while(pos)
		{
			delete m_map_active.GetNextValue(pos);
		}

		m_map_active.RemoveAll();
	}

	void SetAt(KEY key, VALUE f)
	{
		m_map.SetAt(key, f);
	}

	VALUE Lookup(KEY key)
	{
		m_active = NULL;

		if(!m_map_active.Lookup(key, m_active))
		{
			CRBMap<KEY, VALUE>::CPair* pair = m_map.Lookup(key);

			ActivePtr* p = new ActivePtr();

			memset(p, 0, sizeof(*p));

			p->frame = (UINT64)-1;

			p->f = pair ? pair->m_value : GetDefaultFunction(key);

			m_map_active.SetAt(key, p);

			m_active = p;
		}

		return m_active->f;
	}

	void UpdateStats(const GSRasterizerStats& stats, UINT64 frame)
	{
		if(m_active)
		{
			if(m_active->frame != frame)
			{
				m_active->frame = frame;
				m_active->frames++;
			}

			m_active->pixels += stats.pixels;
			m_active->ticks += stats.ticks;
		}
	}

	virtual void PrintStats()
	{
		__int64 ttpf = 0;

		POSITION pos = m_map_active.GetHeadPosition();

		while(pos)
		{
			ActivePtr* p = m_map_active.GetNextValue(pos);
			
			if(p->frames)
			{
				ttpf += p->ticks / p->frames;
			}
		}

		pos = m_map_active.GetHeadPosition();

		while(pos)
		{
			KEY key;
			ActivePtr* p;

			m_map_active.GetNextAssoc(pos, key, p);

			if(p->frames > 0)
			{
				__int64 tpp = p->pixels > 0 ? p->ticks / p->pixels : 0;
				__int64 tpf = p->frames > 0 ? p->ticks / p->frames : 0;
				__int64 ppf = p->frames > 0 ? p->pixels / p->frames : 0;

				printf("[%012I64x]%c %6.2f%% | %5.2f%% | f %4I64d | p %10I64d | tpp %4I64d | tpf %9I64d | ppf %7I64d\n", 
					(UINT64)key, !m_map.Lookup(key) ? '*' : ' ',
					(float)(tpf * 10000 / 50000000) / 100, 
					(float)(tpf * 10000 / ttpf) / 100, 
					p->frames, p->pixels, 
					tpp, tpf, ppf);
			}
		}
	}
};

#include "GSCodeBuffer.h"

template<class CG, class KEY, class VALUE>
class GSCodeGeneratorFunctionMap : public GSFunctionMap<KEY, VALUE>
{
	CRBMap<UINT64, CG*> m_cgmap;
	GSCodeBuffer m_cb;

	enum {MAX_SIZE = 4096};

protected:
	virtual CG* Create(KEY key, void* ptr, size_t maxsize = MAX_SIZE) = 0;

public:
	GSCodeGeneratorFunctionMap()
	{
	}

	virtual ~GSCodeGeneratorFunctionMap()
	{
		POSITION pos = m_cgmap.GetHeadPosition();

		while(pos)
		{
			delete m_cgmap.GetNextValue(pos);
		}
	}

	VALUE GetDefaultFunction(KEY key)
	{
		CG* cg = NULL;

		if(!m_cgmap.Lookup(key, cg))
		{
			void* ptr = m_cb.GetBuffer(MAX_SIZE);

			cg = Create(key, ptr, MAX_SIZE);

			ASSERT(cg);

			m_cb.ReleaseBuffer(cg->getSize());

			m_cgmap.SetAt(key, cg);
		}

		return (VALUE)cg->getCode();
	}
};
