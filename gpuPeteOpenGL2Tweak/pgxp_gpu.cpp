#include "stdafx.h"
#include "pgxp_gpu.h"

#include "gpuPeteOpenGL2Tweak.h"
#include "GPUPlugin.h"
#include "SafeWrite.h"

#include <ppl.h>


const unsigned char primSizeTable[256] =
{
    // 00
    0,0,3,0,0,0,0,0,
    // 08
    0,0,0,0,0,0,0,0,
    // 10
    0,0,0,0,0,0,0,0,
    // 18
    0,0,0,0,0,0,0,0,
    // 20
    4,4,4,4,7,7,7,7,
    // 28
    5,5,5,5,9,9,9,9,
    // 30
    6,6,6,6,9,9,9,9,
    // 38
    8,8,8,8,12,12,12,12,
    // 40
    3,3,3,3,0,0,0,0,
    // 48
//    5,5,5,5,6,6,6,6,      //FLINE
    254,254,254,254,254,254,254,254,
    // 50
    4,4,4,4,0,0,0,0,
    // 58
//    7,7,7,7,9,9,9,9,    //    LINEG3    LINEG4
    255,255,255,255,255,255,255,255,
    // 60
    3,3,3,3,4,4,4,4,    //    TILE    SPRT
    // 68
    2,2,2,2,3,3,3,3,    //    TILE1
    // 70
    2,2,2,2,3,3,3,3,
    // 78
    2,2,2,2,3,3,3,3,
    // 80
    4,0,0,0,0,0,0,0,
    // 88
    0,0,0,0,0,0,0,0,
    // 90
    0,0,0,0,0,0,0,0,
    // 98
    0,0,0,0,0,0,0,0,
    // a0
    3,0,0,0,0,0,0,0,
    // a8
    0,0,0,0,0,0,0,0,
    // b0
    0,0,0,0,0,0,0,0,
    // b8
    0,0,0,0,0,0,0,0,
    // c0
    3,0,0,0,0,0,0,0,
    // c8
    0,0,0,0,0,0,0,0,
    // d0
    0,0,0,0,0,0,0,0,
    // d8
    0,0,0,0,0,0,0,0,
    // e0
    0,1,1,1,1,1,1,0,
    // e8
    0,0,0,0,0,0,0,0,
    // f0
    0,0,0,0,0,0,0,0,
    // f8
    0,0,0,0,0,0,0,0
};

static PGXP* s_PGXP;

PGXP::PGXP()
{
	s_PGXP = this;

	lx[0] = (s16*)GPUPlugin::Get().GetPluginMem(0x00051A08);
	lx[1] = (s16*)GPUPlugin::Get().GetPluginMem(0x00051A0A);
	lx[2] = (s16*)GPUPlugin::Get().GetPluginMem(0x00051A0C);
	lx[3] = (s16*)GPUPlugin::Get().GetPluginMem(0x00051A0E);

	ly[0] = (s16*)GPUPlugin::Get().GetPluginMem(0x00051A10);
	ly[1] = (s16*)GPUPlugin::Get().GetPluginMem(0x00051A12);
	ly[2] = (s16*)GPUPlugin::Get().GetPluginMem(0x00051A14);
	ly[3] = (s16*)GPUPlugin::Get().GetPluginMem(0x00051A16);

	vertex[0] = (OGLVertex*)GPUPlugin::Get().GetPluginMem(0x00052220);
	vertex[1] = (OGLVertex*)GPUPlugin::Get().GetPluginMem(0x00052238);
	vertex[2] = (OGLVertex*)GPUPlugin::Get().GetPluginMem(0x00052250);
	vertex[3] = (OGLVertex*)GPUPlugin::Get().GetPluginMem(0x00052268);

	numVertices	= 0;
	vertexIdx	= 0;

	PSXDisplay_CumulOffset_x = (s16*)GPUPlugin::Get().GetPluginMem(0x00051FFC);
	PSXDisplay_CumulOffset_y = (s16*)GPUPlugin::Get().GetPluginMem(0x00051FFE);

	lineHackMode = 1;

	//dword_10052130 = dmaMem;
	PGXP_Mem	= NULL;
	lUsedAddr	= (u32*)GPUPlugin::Get().GetPluginMem(0x00052130);

	offset_fn offset3 = (offset_fn)GPUPlugin::Get().GetPluginMem(0x000041B0);
	CreateHook(offset3, PGXP::offset3, &ooffset3);
	EnableHook(offset3);

	offset_fn offset4 = (offset_fn)GPUPlugin::Get().GetPluginMem(0x000043F0);
	CreateHook(offset4, PGXP::offset4, &ooffset4);
	EnableHook(offset4);

	primPoly_fn primPolyF3 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001BFF0);
	CreateHook(primPolyF3, PGXP::primPolyF3, &oprimPolyF3);
	EnableHook(primPolyF3);

	primPoly_fn primPolyFT3 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001A620);
	CreateHook(primPolyFT3, PGXP::primPolyFT3, &oprimPolyFT3);
	EnableHook(primPolyFT3);

	primPoly_fn primPolyF4 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x00019DD0);
	CreateHook(primPolyF4, PGXP::primPolyF4, &oprimPolyF4);
	EnableHook(primPolyF4);

	primPoly_fn primPolyFT4 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001AFB0);
	CreateHook(primPolyFT4, PGXP::primPolyFT4, &oprimPolyFT4);
	EnableHook(primPolyFT4);

	primPoly_fn primPolyG3 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001B880);
	CreateHook(primPolyG3, PGXP::primPolyG3, &oprimPolyG3);
	EnableHook(primPolyG3);

	primPoly_fn primPolyGT3 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001B3B0);
	CreateHook(primPolyGT3, PGXP::primPolyGT3, &oprimPolyGT3);
	EnableHook(primPolyGT3);

	primPoly_fn primPolyG4 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x00019F60);
	CreateHook(primPolyG4, PGXP::primPolyG4, &oprimPolyG4);
	EnableHook(primPolyG4);

	primPoly_fn primPolyGT4 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001BA40);
	CreateHook(primPolyGT4, PGXP::primPolyGT4, &oprimPolyGT4);
	EnableHook(primPolyGT4);

	rectTexAlign_fn rectTexAlign = (rectTexAlign_fn)GPUPlugin::Get().GetPluginMem(0x0001A900);
	CreateHook(rectTexAlign, PGXP::rectTexAlign, &orectTexAlign);
	EnableHook(rectTexAlign);

	doLineCheck_fn doLineCheck = (doLineCheck_fn)GPUPlugin::Get().GetPluginMem(0x0001A1C0);
	CreateHook(doLineCheck, PGXP::doLineCheck, &odoLineCheck);
	EnableHook(doLineCheck);

	CreateHook(glOrtho, Hook_glOrtho, reinterpret_cast<void**>(&oglOrtho));
	EnableHook(glOrtho);

	CreateHook(glVertex3fv, Hook_glVertex3fv, reinterpret_cast<void**>(&oglVertex3fv));
	EnableHook(glVertex3fv);

	PLUGINLOG("PGXP Enabled");
}

PGXP::~PGXP()
{
	DisableHook(glOrtho);
	DisableHook(glVertex3fv);
}

void PGXP::SetMemoryPtr(unsigned int addr, unsigned char* pVRAM)
{
	PGXP_Mem = (PGXP_vertex*)(pVRAM);
	currentAddr = addr;
}

void PGXP::SetAddress(uint32_t *baseAddrL, int size)
{
	currentAddr = (*lUsedAddr + 4) >> 2;
	pDMABlock = baseAddrL;
	blockSize = size;
}

void PGXP::SetLineHackMode(u32 lineHack)
{
	lineHackMode = lineHack;
}

void PGXP::ResetVertex()
{
	for (unsigned int i = 0; i < 4; i++)	//iCB: remove stale vertex data
	{
		vertex[i]->x = vertex[i]->y = 0.f;
		vertex[i]->z = 0.95f;
		fxy[i].z = 1.f;
	}
}

void PGXP::GetVertices(u32* addr)
{
	const unsigned int primStrideTable[]	= { 1, 2, 1, 2, 2, 3, 2, 3, 0 };
	const unsigned int primCountTable[]		= { 3, 3, 4, 4, 3, 3, 4, 4, 0 };

	unsigned int	primCmd = ((*addr >> 24) & 0xff);		// primitive command
	unsigned int	primIdx = (primCmd - 0x20) >> 2;		// index to primitive lookup
	primIdx = primIdx < 8 ? primIdx : 8;
	unsigned int	stride = primStrideTable[primIdx];		// stride between vertices
	unsigned int	count = primCountTable[primIdx];		// number of vertices
	PGXP_vertex*	primStart = NULL;						// pointer to first vertex
	short*			pPrimData = ((short*)addr) + 2;			// primitive data for cache lookups

	// calculate offset to actual data
	int offset = 0;
	while ((pDMABlock[offset] != *addr) && (offset < blockSize))
	{
		unsigned char command = (unsigned char)((pDMABlock[offset] >> 24) & 0xff);
		unsigned int primSize = primSizeTable[command];

		if (primSize == 0)
		{
			offset++;
			continue;
		}
		else if (primSize > 128)
		{
			while (((pDMABlock[offset] & 0xF000F000) != 0x50005000) && (offset < blockSize))
				++offset;
		}
		else
			offset += primSize;
	}

	if (PGXP_Mem != NULL)
	{
		// Offset to start of primitive
		primStart = &PGXP_Mem[currentAddr + offset + 1];
	}

	for (unsigned i = 0; i < count; ++i)
	{
		if (primStart && ((primStart[stride * i].flags & VALID_01) == VALID_01) && (primStart[stride * i].value == *(u32*)(&pPrimData[stride * i * 2])))
		{
			fxy[i] = primStart[stride * i];
		}
		else
		{
			fxy[i].flags = NONE;

			// Look in cache for valid vertex
			PGXP_vertex* pCacheVert = GetCachedVertex(pPrimData[stride * i * 2], pPrimData[(stride * i * 2) + 1]);
			if (pCacheVert)
			{
				if (IsSessionID(pCacheVert->count) && (pCacheVert->mFlags == 1))
					fxy[i] = *pCacheVert;
			}
		}
	}
}

void PGXP::fix_offsets(s32 count)
{
	u32 invalidVert = 0;

	// Reset vertex count
	numVertices = count;
	vertexIdx = 0;

	// Find any invalid vertices
	for (unsigned i = 0; i < count; ++i)
	{
		if ((fxy[i].flags & VALID_012) != VALID_012)
			invalidVert++;
	}

	for (int i = 0; i < count; ++i)
	{
		float w = fxy[i].z;

		if (invalidVert > 0)
			w = 1;

		if (((fxy[i].flags & VALID_01) == VALID_01) /*&& std::fabs(fxy[i].x - *lx[i]) < 1.0f && std::fabs(fxy[i].y - *ly[i]) < 1.0f*/)
		{
			// clear upper 4 bits
			float x = fxy[i].x *(1 << 16);
			float y = fxy[i].y *(1 << 16);
			x = (float)(((int)x << 4) >> 4);
			y = (float)(((int)y << 4) >> 4);
			x /= (1 << 16);
			y /= (1 << 16);

			vertex[i]->x = (x + *PSXDisplay_CumulOffset_x);
			vertex[i]->y = (y + *PSXDisplay_CumulOffset_y);
		}

		fxy[i].z = w; // store w for later restoration in glVertex3fv
	}
}


/////////////////////////////////
//// Blade_Arma's Vertex Cache (CatBlade?)
/////////////////////////////////

unsigned int PGXP::IsSessionID(unsigned int vertID)
{
	// No wrapping
	if (lastID >= baseID)
		return (vertID >= baseID);

	// If vertID is >= baseID it is pre-wrap and in session
	if (vertID >= baseID)
		return 1;

	// vertID is < baseID, If it is <= lastID it is post-wrap and in session
	if (vertID <= lastID)
		return 1;

	return 0;
}

void PGXP::CacheVertex(short sx, short sy, const PGXP_vertex* _pVertex)
{
	const PGXP_vertex*	pNewVertex = _pVertex;
	PGXP_vertex*		pOldVertex = NULL;

	if (!pNewVertex)
	{
		cacheMode = mode_fail;
		return;
	}

	//if (bGteAccuracy)
	{
		if (cacheMode != mode_write)
		{
			// Initialise cache on first use
			if (cacheMode == mode_init)
				memset(vertexCache, 0x00, sizeof(vertexCache));

			// First vertex of write session (frame?)
			cacheMode = mode_write;
			baseID = pNewVertex->count;
		}

		lastID = pNewVertex->count;

		if (sx >= -0x800 && sx <= 0x7ff &&
			sy >= -0x800 && sy <= 0x7ff)
		{
			pOldVertex = &vertexCache[sy + 0x800][sx + 0x800];

			// To avoid ambiguity there can only be one valid entry per-session
			if (IsSessionID(pOldVertex->count) && (pOldVertex->value == pNewVertex->value))
			{
				// check to ensure this isn't identical
				if ((fabsf(pOldVertex->x - pNewVertex->x) > 0.1f) ||
					(fabsf(pOldVertex->y - pNewVertex->y) > 0.1f) ||
					(fabsf(pOldVertex->z - pNewVertex->z) > 0.1f))
				{
					pOldVertex->mFlags = 5;
					return;
				}
			}

			// Write vertex into cache
			*pOldVertex = *pNewVertex;
			pOldVertex->mFlags = 1;
		}
	}
}

PGXP_vertex* PGXP::GetCachedVertex(short sx, short sy)
{
	//if (bGteAccuracy)
	{
		if (cacheMode != mode_read)
		{
			if (cacheMode == mode_fail)
				return NULL;

			// Initialise cache on first use
			if (cacheMode == mode_init)
				memset(vertexCache, 0x00, sizeof(vertexCache));

			// First vertex of read session (frame?)
			cacheMode = mode_read;
		}

		if (sx >= -0x800 && sx <= 0x7ff &&
			sy >= -0x800 && sy <= 0x7ff)
		{
			// Return pointer to cache entry
			return &vertexCache[sy + 0x800][sx + 0x800];
		}
	}

	return NULL;
}



// 0 = disabled
// 1 = enabled (default mode) 
// 2 = enabled (aggressive mode)

// Hack to deal with PS1 games rendering axis aligned lines using 1 pixel wide triangles with UVs that describe a line
// Suitable for games like Soul Blade, Doom and Hexen
bool PGXP::Hack_FindLine(flatTriPrim* srcPrim, flatTriPrim* outPrim, OGLVertex* outVertices)
{
	int pxWidth = 1;	// width of a single pixel
	unsigned short cornerIdx, shortIdx, longIdx;

	sourceVert* srcVert = &srcPrim->verts[0];

	memcpy(outPrim, srcPrim, sizeof(flatTriPrim));

	// reject 3D elements
	if ((fxy[0].z != fxy[1].z) ||
		(fxy[0].z != fxy[2].z))
		return false;

	// find short side of triangle / end of line with 2 vertices (guess which vertex is the right angle)
	if ((srcVert[0].u == srcVert[1].u) && (srcVert[0].v == srcVert[1].v))
		cornerIdx = 0;
	else if ((srcVert[1].u == srcVert[2].u) && (srcVert[1].v == srcVert[2].v))
		cornerIdx = 1;
	else if ((srcVert[2].u == srcVert[0].u) && (srcVert[2].v == srcVert[0].v))
		cornerIdx = 2;
	else
		return false;

	// assign other indices to remaining vertices
	shortIdx = (cornerIdx + 1) % 3;
	longIdx = (shortIdx + 1) % 3;

	// determine line orientation and check width
	if ((srcVert[cornerIdx].x == srcVert[shortIdx].x) && (abs(srcVert[cornerIdx].y - srcVert[shortIdx].y) == pxWidth))
	{
		// line is horizontal
		// determine which is truly the corner by checking against the long side, while making sure it is axis aligned
		if (srcVert[shortIdx].y == srcVert[longIdx].y)
		{
			unsigned short tempIdx = shortIdx;
			shortIdx = cornerIdx;
			cornerIdx = tempIdx;
		}
		else if (srcVert[cornerIdx].y != srcVert[longIdx].y)
			return false;

		// flip corner index to other side of quad
		outVertices[cornerIdx] = *vertex[longIdx];
		outVertices[cornerIdx].y = vertex[shortIdx]->y;

		outPrim->verts[cornerIdx].x = srcVert[longIdx].x;
		outPrim->verts[cornerIdx].u = srcVert[longIdx].u;
		outPrim->verts[cornerIdx].v = srcVert[longIdx].v;
		outPrim->verts[cornerIdx].y = srcVert[shortIdx].y;
	}
	else if ((srcVert[cornerIdx].y == srcVert[shortIdx].y) && (abs(srcVert[cornerIdx].x - srcVert[shortIdx].x) == pxWidth))
	{
		// line is vertical
		// determine which is truly the corner by checking against the long side, while making sure it is axis aligned
		if (srcVert[shortIdx].x == srcVert[longIdx].x)
		{
			unsigned short tempIdx = shortIdx;
			shortIdx = cornerIdx;
			cornerIdx = tempIdx;
		}
		else if (srcVert[cornerIdx].x != srcVert[longIdx].x)
			return false;

		// flip corner index to other side of quad
		outVertices[cornerIdx] = *vertex[longIdx];
		outVertices[cornerIdx].x = vertex[shortIdx]->x;

		outPrim->verts[cornerIdx].y = srcVert[longIdx].y;
		outPrim->verts[cornerIdx].u = srcVert[longIdx].u;
		outPrim->verts[cornerIdx].v = srcVert[longIdx].v;
		outPrim->verts[cornerIdx].x = srcVert[shortIdx].x;
	}
	else
		return false;

	// write out second tri
	outVertices[shortIdx] = *vertex[shortIdx];
	outVertices[longIdx] = *vertex[longIdx];

	
	outPrim->verts[longIdx].x = srcVert[longIdx].x;
	outPrim->verts[longIdx].y = srcVert[longIdx].y;
	outPrim->verts[longIdx].u = srcVert[longIdx].u;
	outPrim->verts[longIdx].v = srcVert[longIdx].v;

	outPrim->verts[shortIdx].x = srcVert[shortIdx].x;
	outPrim->verts[shortIdx].y = srcVert[shortIdx].y;
	outPrim->verts[shortIdx].u = srcVert[shortIdx].u;
	outPrim->verts[shortIdx].v = srcVert[shortIdx].v;

	return true;
}


// Hack to deal with PS1 games rendering axis aligned lines using 1 pixel wide triangles and force UVs to describe a line
// Required for games like Dark Forces and Duke Nukem
bool PGXP::Hack_ForceLine(flatTriPrim* srcPrim, flatTriPrim* outPrim, OGLVertex* outVertices)
{
	int pxWidth = 1;	// width of a single pixel
	unsigned short cornerIdx, shortIdx, longIdx;

	sourceVert* srcVert = &srcPrim->verts[0];

	memcpy(outPrim, srcPrim, sizeof(flatTriPrim));

	// reject 3D elements
	if ((fxy[0].z != fxy[1].z) ||
		(fxy[0].z != fxy[2].z))
		return false;

	// find vertical AB
	unsigned short A, B, C;
	if (srcVert[0].x == srcVert[1].x)
		A = 0;
	else if (srcVert[1].x == srcVert[2].x)
		A = 1;
	else if (srcVert[2].x == srcVert[0].x)
		A = 2;
	else
		return false;

	// assign other indices to remaining vertices
	B = (A + 1) % 3;
	C = (B + 1) % 3;

	// find horizontal AC or BC
	if (srcVert[A].y == srcVert[C].y)
		cornerIdx = A;
	else if (srcVert[B].y == srcVert[C].y)
		cornerIdx = B;
	else
		return false;

	// determine lengths of sides
	if (abs(srcVert[A].y - srcVert[B].y) == pxWidth)
	{
		// is Horizontal
		shortIdx = (cornerIdx == A) ? B : A;
		longIdx = C;

		// flip corner index to other side of quad
		outVertices[cornerIdx] = *vertex[longIdx];
		outVertices[cornerIdx].y = vertex[shortIdx]->y;

		outPrim->verts[cornerIdx].x = srcVert[longIdx].x;
		outPrim->verts[cornerIdx].u = srcVert[longIdx].u;
		outPrim->verts[cornerIdx].v = srcVert[longIdx].v;
		outPrim->verts[cornerIdx].y = srcVert[shortIdx].y;
	}
	else if (abs(srcVert[A].x - srcVert[C].x) == pxWidth)
	{
		// is Vertical
		shortIdx = C;
		longIdx = (cornerIdx == A) ? B : A;

		// flip corner index to other side of quad
		outVertices[cornerIdx] = *vertex[longIdx];
		outVertices[cornerIdx].x = vertex[shortIdx]->x;

		outPrim->verts[cornerIdx].y = srcVert[longIdx].y;
		outPrim->verts[cornerIdx].u = srcVert[longIdx].u;
		outPrim->verts[cornerIdx].v = srcVert[longIdx].v;
		outPrim->verts[cornerIdx].x = srcVert[shortIdx].x;
	}
	else
		return false;

	// force UVs into a line along the upper or left most edge of the triangle
	// Otherwise the wrong UVs will be sampled on second triangle and by hardware renderers
	srcVert[shortIdx].u = srcVert[cornerIdx].u;
	srcVert[shortIdx].v = srcVert[cornerIdx].v;


	// write out second tri
	outVertices[shortIdx] = *vertex[shortIdx];
	outVertices[longIdx] = *vertex[longIdx];

	outPrim->verts[longIdx].x = srcVert[longIdx].x;
	outPrim->verts[longIdx].y = srcVert[longIdx].y;
	outPrim->verts[longIdx].u = srcVert[longIdx].u;
	outPrim->verts[longIdx].v = srcVert[longIdx].v;

	outPrim->verts[shortIdx].x = srcVert[shortIdx].x;
	outPrim->verts[shortIdx].y = srcVert[shortIdx].y;
	outPrim->verts[shortIdx].u = srcVert[shortIdx].u;
	outPrim->verts[shortIdx].v = srcVert[shortIdx].v;

	return true;
}


PGXP::offset_fn PGXP::ooffset3;
BOOL __cdecl PGXP::offset3(void)
{
	BOOL ret = ooffset3();
	s_PGXP->fix_offsets(3);
	return ret;
}

PGXP::offset_fn PGXP::ooffset4;
BOOL __cdecl PGXP::offset4(void)
{
	BOOL ret = ooffset4();
	s_PGXP->fix_offsets(4);
	return ret;
}

PGXP::primPoly_fn PGXP::oprimPolyF3;
void __cdecl PGXP::primPolyF3(unsigned char *baseAddr)
{
	s_PGXP->GetVertices((u32*)baseAddr);
	oprimPolyF3(baseAddr);
	s_PGXP->ResetVertex();
}

PGXP::primPoly_fn PGXP::oprimPolyFT3;
void __cdecl PGXP::primPolyFT3(unsigned char *baseAddr)
{
	s_PGXP->GetVertices((u32*)baseAddr);

	flatTriPrim sourcePrimData;
	flatTriPrim newPrimData;
	OGLVertex newVertices[3];
	bool isLine = false;

	memcpy(&sourcePrimData, baseAddr, sizeof(flatTriPrim));

	switch (s_PGXP->lineHackMode)
	{
	case 0:
		// Disabled
		break;
	case 1:
		// Default mode
		isLine = s_PGXP->Hack_FindLine(&sourcePrimData, &newPrimData, newVertices);
		break;
	case 2:
		// Aggressive mode
		isLine = s_PGXP->Hack_ForceLine(&sourcePrimData, &newPrimData, newVertices);
		break;
	default:
		// Disabled
		break;
	}

	oprimPolyFT3((u8*)&sourcePrimData);

	if(isLine)
	{
		//copy new vertices
		for (u32 i = 0; i < 3; ++i)
			*s_PGXP->vertex[i] = newVertices[i];

		// Draw second triangle
		oprimPolyFT3((u8*)&newPrimData);
	}

	s_PGXP->ResetVertex();
}

PGXP::primPoly_fn PGXP::oprimPolyF4;
void __cdecl PGXP::primPolyF4(unsigned char *baseAddr)
{
	s_PGXP->GetVertices((u32*)baseAddr);
	oprimPolyF4(baseAddr);
	s_PGXP->ResetVertex();
}

PGXP::primPoly_fn PGXP::oprimPolyFT4;
void __cdecl PGXP::primPolyFT4(unsigned char *baseAddr)
{
	s_PGXP->GetVertices((u32*)baseAddr);
	oprimPolyFT4(baseAddr);
	s_PGXP->ResetVertex();
}

PGXP::primPoly_fn PGXP::oprimPolyG3;
void __cdecl PGXP::primPolyG3(unsigned char *baseAddr)
{
	s_PGXP->GetVertices((u32*)baseAddr);
	oprimPolyG3(baseAddr);
	s_PGXP->ResetVertex();
}

PGXP::primPoly_fn PGXP::oprimPolyGT3;
void __cdecl PGXP::primPolyGT3(unsigned char *baseAddr)
{
	s_PGXP->GetVertices((u32*)baseAddr);
	oprimPolyGT3(baseAddr);
	s_PGXP->ResetVertex();
}

PGXP::primPoly_fn PGXP::oprimPolyG4;
void __cdecl PGXP::primPolyG4(unsigned char *baseAddr)
{
	s_PGXP->GetVertices((u32*)baseAddr);
	oprimPolyG4(baseAddr);
	s_PGXP->ResetVertex();
}

PGXP::primPoly_fn PGXP::oprimPolyGT4;
void __cdecl PGXP::primPolyGT4(unsigned char *baseAddr)
{
	s_PGXP->GetVertices((u32*)baseAddr);
	oprimPolyGT4(baseAddr);
	s_PGXP->ResetVertex();
}

PGXP::rectTexAlign_fn PGXP::orectTexAlign;
void __cdecl PGXP::rectTexAlign(void)
{
	if((s_PGXP->fxy[0].z != s_PGXP->fxy[1].z) || (s_PGXP->fxy[1].z != s_PGXP->fxy[2].z) || (s_PGXP->fxy[2].z != s_PGXP->fxy[3].z))
		return;

	orectTexAlign();
}

PGXP::doLineCheck_fn PGXP::odoLineCheck;
BOOL __cdecl PGXP::doLineCheck(u32* gpuData)
{
	return FALSE;
}

void (APIENTRY* PGXP::oglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void APIENTRY PGXP::Hook_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	GLfloat m[16];
	for (unsigned int i = 0; i < 16; ++i)
		m[i] = 0.f;

	// iCB: Implementation of glOrtho for reference and testing
	//if ((right-left) != 0)
	//{
	//	m[0] = 2 / (right - left);
	//	m[12] = -((right + left) / (right - left));
	//}
	//if ((top-bottom) != 0)
	//{
	//	m[5] = 2 / (top - bottom);
	//	m[13] = -((top + bottom) / (top - bottom));
	//}
	//m[10] = -2 / (zFar - zNear);
	//m[14] = -((zFar + zNear) / (zFar - zNear));
	//m[15] = 1;

	// iCB: Substitute z value for w
	//if ((right - left) != 0)
	//{
	//	m[0] = 2 / (right - left);
	//	m[8] = -((right + left) / (right - left));
	//}
	//if ((top - bottom) != 0)
	//{
	//	m[5] = 2 / (top - bottom);
	//	m[9] = -((top + bottom) / (top - bottom));
	//}
	//m[10] = -2 / (zFar - zNear);
	//m[14] = -((zFar + zNear) / (zFar - zNear));
	//m[11] = 1;

//	glLoadMatrixf(m);

	// iCB: Hack, extend near and far planes to accomodate z mask values after multiplication by w
	oglOrtho(left, right, bottom, top, zNear*100, zFar*100);
}

void (APIENTRY* PGXP::oglVertex3fv)(const GLfloat * v);
void APIENTRY PGXP::Hook_glVertex3fv(const GLfloat * v)
{
	// If there are PGXP vertices expected
	if (s_PGXP->vertexIdx < s_PGXP->numVertices)
	{
		// copy vertex and add w component
		GLfloat temp[4];
		memcpy(temp, v, sizeof(GLfloat) * 3);

		// Get vertex index
		size_t vertIdx = ((size_t)v - (size_t)s_PGXP->vertex[0]) / ((size_t)s_PGXP->vertex[1] - (size_t)s_PGXP->vertex[0]);

		if (vertIdx >= 4)
			temp[3] = 1;
		else
			temp[3] = s_PGXP->fxy[vertIdx].z;

		// pre-multiply each element by w (to negate perspective divide)
		for(u32 i=0; i < 3; i++)
			temp[i] *= temp[3];

		// pass complete vertex to OpenGL
		glVertex4fv(temp);
		s_PGXP->vertexIdx++;
	}
	else
	{
		oglVertex3fv(v);
	}
}