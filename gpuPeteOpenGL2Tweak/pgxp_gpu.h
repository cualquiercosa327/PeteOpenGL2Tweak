#pragma once

/***************************************************************************
*   Copyright (C) 2016 by iCatButler                                          *
*   Copyright (C) 2015 by tapcio                                          *
*   Copyright (C) 2011 by Blade_Arma                                      *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
***************************************************************************/

struct OGLVertex
{
	float x;
	float y;
	float z;
};

typedef struct
{
	float	x;
	float	y;
	float	z;
	union
	{
		unsigned int	flags;
		unsigned char	compFlags[4];
	};
	unsigned int	count;
	unsigned int	value;
	unsigned int	mFlags;
} PGXP_vertex;

#define NONE	 0
#define ALL		 0xFFFFFFFF
#define VALID	 1
#define VALID_0  (VALID << 0)
#define VALID_1  (VALID << 8)
#define VALID_2  (VALID << 16)
#define VALID_3  (VALID << 24)
#define VALID_01  (VALID_0 | VALID_1)
#define VALID_012  (VALID_0 | VALID_1 | VALID_2)
#define VALID_ALL  (VALID_0 | VALID_1 | VALID_2 | VALID_3)
#define INV_VALID_ALL  (ALL ^ VALID_ALL)

static const uint32_t addr_mask[8] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
0x7FFFFFFF, 0x1FFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

class PGXP
{
private:
	typedef BOOL(__cdecl* offset_fn)(void);
	typedef void(__cdecl* primPoly_fn)(unsigned char *baseAddr);
	typedef void(__cdecl* ortho_fn)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal);

	std::array<short*, 4> lx;
	std::array<short*, 4> ly;

	std::array<PGXP_vertex, 4> fxy;
	u32 numVertices;	// iCB: Used for glVertex3fv fix
	u32 vertexIdx;

	std::array<OGLVertex*, 4> vertex;
	s16* PSXDisplay_CumulOffset_x;
	s16* PSXDisplay_CumulOffset_y;

	PGXP_vertex* PGXP_Mem;
	u32* lUsedAddr;
	int* gpuDataP;
	u32 currentAddr = 0;

	/// CACHING
	const unsigned int mode_init	= 0;
	const unsigned int mode_write	= 1;
	const unsigned int mode_read	= 2;
	const unsigned int mode_fail	= 3;

	PGXP_vertex vertexCache[0x800 * 2][0x800 * 2];

	unsigned int baseID = 0;
	unsigned int lastID = 0;
	unsigned int cacheMode = 0;

	unsigned int	IsSessionID(unsigned int vertID);
	PGXP_vertex*	GetCachedVertex(short sx, short sy);
	// /CACHING

	void fix_offsets(s32 count);

	static offset_fn ooffset3;
	static BOOL __cdecl offset3(void);

	static offset_fn ooffset4;
	static BOOL __cdecl offset4(void);

	static primPoly_fn oprimPolyF3;
	static void __cdecl primPolyF3(unsigned char *baseAddr);

	static primPoly_fn oprimPolyFT3;
	static void __cdecl primPolyFT3(unsigned char *baseAddr);

	static primPoly_fn oprimPolyF4;
	static void __cdecl primPolyF4(unsigned char *baseAddr);

	static primPoly_fn oprimPolyFT4;
	static void __cdecl primPolyFT4(unsigned char *baseAddr);

	static primPoly_fn oprimPolyG3;
	static void __cdecl primPolyG3(unsigned char *baseAddr);

	static primPoly_fn oprimPolyGT3;
	static void __cdecl primPolyGT3(unsigned char *baseAddr);

	static primPoly_fn oprimPolyG4;
	static void __cdecl primPolyG4(unsigned char *baseAddr);

	static primPoly_fn oprimPolyGT4;
	static void __cdecl primPolyGT4(unsigned char *baseAddr);

	static void(APIENTRY* oglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	static void APIENTRY Hook_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);

	static void(APIENTRY* oglVertex3fv)(const GLfloat * v);
	static void APIENTRY Hook_glVertex3fv(const GLfloat * v);

	void GetVertices(u32* baseAddr);
	void ResetVertex();

public:
	PGXP();
	~PGXP();

	void SetMemoryPtr(unsigned int addr, unsigned char* pVRAM);
	void SetAddress();

	void CacheVertex(short sx, short sy, const PGXP_vertex* _pVertex);
};

