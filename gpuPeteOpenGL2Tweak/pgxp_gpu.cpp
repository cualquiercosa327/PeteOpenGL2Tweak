#include "stdafx.h"
#include "pgxp_gpu.h"

#include "gpuPeteOpenGL2Tweak.h"
#include "GPUPlugin.h"
#include "SafeWrite.h"

#include <ppl.h>

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

	//CreateHook(glOrtho, Hook_glOrtho, reinterpret_cast<void**>(&oglOrtho));
	//EnableHook(glOrtho);

	CreateHook(glVertex3fv, Hook_glVertex3fv, reinterpret_cast<void**>(&oglVertex3fv));
	EnableHook(glVertex3fv);

	PLUGINLOG("PGXP Enabled");
}

PGXP::~PGXP()
{
	DisableHook(glVertex3fv);
}

void PGXP::SetMemoryPtr(unsigned int addr, unsigned char* pVRAM)
{
	if (pVRAM)
		PGXP_Mem = (PGXP_vertex*)(pVRAM);
	currentAddr = addr;
}

void PGXP::SetAddress()
{
	currentAddr = (*lUsedAddr + 4) >> 2;
}

void PGXP::ResetVertex()
{
	PLUGINLOG("After: %f, %f, %f, %f", vertex[0]->z, vertex[0]->z, vertex[0]->z, vertex[0]->z);
	for (unsigned int i = 0; i < 4; i++)	//iCB: remove stale vertex data
	{
		vertex[i]->x = vertex[i]->y = 0.f;
		vertex[i]->z = 1.f;
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

	if (PGXP_Mem == NULL)
		return;

	// Offset to start of primitive
	primStart = &PGXP_Mem[currentAddr + 1];

	for (unsigned i = 0; i < count; ++i)
	{
		if (primStart[stride * i].valid)
		{
			fxy[i] = primStart[stride * i];
		}
		else
			fxy[i].valid = 0;
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
		if (!fxy[i].valid)
			invalidVert++;
	}

	for (int i = 0; i < count; ++i)
	{
		float w = fxy[i].z;

		if (invalidVert > 0)
			w = 1;

		if (fxy[i].valid /*&& std::fabs(fxy[i].x - *lx[i]) < 1.0f && std::fabs(fxy[i].y - *ly[i]) < 1.0f*/)
		{
			vertex[i]->x = (fxy[i].x + *PSXDisplay_CumulOffset_x);
			vertex[i]->y = (fxy[i].y + *PSXDisplay_CumulOffset_y);
		}

		fxy[i].z = w; // store w for later restoration in glVertex3fv
	}
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
	oprimPolyFT3(baseAddr);
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
	if ((right - left) != 0)
	{
		m[0] = 2 / (right - left);
		m[8] = -((right + left) / (right - left));
	}
	if ((top - bottom) != 0)
	{
		m[5] = 2 / (top - bottom);
		m[9] = -((top + bottom) / (top - bottom));
	}
	m[10] = -2 / (zFar - zNear);
	m[14] = -((zFar + zNear) / (zFar - zNear));
	m[11] = 1;

	glLoadMatrixf(m);
	//oglOrtho(left, right, bottom, top, zNear, zFar);
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
		temp[3] = s_PGXP->fxy[s_PGXP->vertexIdx].z;

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