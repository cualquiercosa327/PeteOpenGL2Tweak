#include "stdafx.h"
#include "GTEAccHack.h"

#include "gpuPeteOpenGL2Tweak.h"
#include "GPUPlugin.h"
#include "SafeWrite.h"

#include <ppl.h>

static GTEAccHack* s_GTEAccHack;

GTEAccHack::GTEAccHack()
{
	s_GTEAccHack = this;

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

	PSXDisplay_CumulOffset_x = (s16*)GPUPlugin::Get().GetPluginMem(0x00051FFC);
	PSXDisplay_CumulOffset_y = (s16*)GPUPlugin::Get().GetPluginMem(0x00051FFE);

	//dword_10052130 = dmaMem;
	lUsedAddr = (u32*)GPUPlugin::Get().GetPluginMem(0x00052130);

	offset_fn offset3 = (offset_fn)GPUPlugin::Get().GetPluginMem(0x000041B0);
	CreateHook(offset3, GTEAccHack::offset3, &ooffset3);
	EnableHook(offset3);

	offset_fn offset4 = (offset_fn)GPUPlugin::Get().GetPluginMem(0x000043F0);
	CreateHook(offset4, GTEAccHack::offset4, &ooffset4);
	EnableHook(offset4);

	primPoly_fn primPolyF3 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001BFF0);
	CreateHook(primPolyF3, GTEAccHack::primPolyF3, &oprimPolyF3);
	EnableHook(primPolyF3);

	primPoly_fn primPolyFT3 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001A620);
	CreateHook(primPolyFT3, GTEAccHack::primPolyFT3, &oprimPolyFT3);
	EnableHook(primPolyFT3);

	primPoly_fn primPolyF4 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x00019DD0);
	CreateHook(primPolyF4, GTEAccHack::primPolyF4, &oprimPolyF4);
	EnableHook(primPolyF4);

	primPoly_fn primPolyFT4 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001AFB0);
	CreateHook(primPolyFT4, GTEAccHack::primPolyFT4, &oprimPolyFT4);
	EnableHook(primPolyFT4);

	primPoly_fn primPolyG3 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001B880);
	CreateHook(primPolyG3, GTEAccHack::primPolyG3, &oprimPolyG3);
	EnableHook(primPolyG3);

	primPoly_fn primPolyGT3 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001B3B0);
	CreateHook(primPolyGT3, GTEAccHack::primPolyGT3, &oprimPolyGT3);
	EnableHook(primPolyGT3);

	primPoly_fn primPolyG4 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x00019F60);
	CreateHook(primPolyG4, GTEAccHack::primPolyG4, &oprimPolyG4);
	EnableHook(primPolyG4);

	primPoly_fn primPolyGT4 = (primPoly_fn)GPUPlugin::Get().GetPluginMem(0x0001BA40);
	CreateHook(primPolyGT4, GTEAccHack::primPolyGT4, &oprimPolyGT4);
	EnableHook(primPolyGT4);

	PLUGINLOG("GTE Accuracy Hack Enabled");
}

GTEAccHack::~GTEAccHack()
{
}

void GTEAccHack::GteFifoInvalidate(u32 cmd)
{
	switch (cmd)
	{
	case 12:
		precise_fifo[0].valid = false;
		break;
	case 13:
		precise_fifo[1].valid = false;
		break;
	case 14:
		precise_fifo[2].valid = false;
		break;
	case 15:
		precise_fifo[3].valid = false;

		precise_fifo[0] = precise_fifo[1];
		precise_fifo[1] = precise_fifo[2];
		precise_fifo[2] = precise_fifo[3];
		break;
	}
}

void GTEAccHack::GteFifoAdd(s64 llx, s64 lly, u16 z)
{
	precise_fifo[3].x = float(llx) / float(1 << 16);
	precise_fifo[3].y = float(lly) / float(1 << 16);
	precise_fifo[3].z = z;
	precise_fifo[3].valid = true;

	precise_fifo[0] = precise_fifo[1];
	precise_fifo[1] = precise_fifo[2];
	precise_fifo[2] = precise_fifo[3];
}

void GTEAccHack::GteTransferToRam(u32 address, u32 cmd)
{
	uint32_t paddr = address & addr_mask[address >> 29];

	if (paddr >= 0x00800000)
		return;

	// Store to RAM
	paddr = (paddr & 0x1FFFFF) >> 2;

	gte_precision* p = nullptr;

	switch (cmd) {
	case 12:
		p = &precise_fifo[0];
		break;

	case 13:
		p = &precise_fifo[1];
		break;

	case 14:
		p = &precise_fifo[2];
		break;

	case 15:
		p = &precise_fifo[3];
		break;
	}

	if (p)
	{
		if (p->valid)
		{
			PrecisionRAM[paddr] = *p;
			p->valid = false;
			//PLUGINLOG("GteTransferToRam %X %X %u %f %f", address, paddr, cmd, PrecisionRAM[paddr].x, PrecisionRAM[paddr].y);
		}
	}

}

void GTEAccHack::GTEwriteDataMem(u32* pMem, s32 size, u32 address)
{
	//unsigned char command;
	//uint32_t gdata = 0;

	//for (int i = 0; i < size; ++i)
	{
		//gdata = *pMem;
		//command = (unsigned char)((gdata >> 24) & 0xff);

		//PLUGINLOG("GTEwriteDataMem %p %d %X %X", pMem, size, address, command);
	}

	uint32_t paddr = address & addr_mask[address >> 29];
	if (paddr >= 0x00800000)
		return;
	currentAddress = (paddr & 0x1FFFFF) >> 2;
}

void GTEAccHack::OnDmaChain(u32 * baseAddrL, u32 addr)
{
	//currentAddress = *lUsedAddr;
}

void GTEAccHack::OnWriteDataMem(u32* pMem, s32 iSize)
{
	currentAddress = (*lUsedAddr + 4) >> 2;
	u8 primCmd = ((*pMem >> 24) & 0xff);
	//PLUGINLOG("OnWriteDataMem %X %X", primCmd, currentAddress);
}

void GTEAccHack::primPoly(u32* baseAddr)
{
	u8 primCmd = ((*baseAddr >> 24) & 0xff);
	uint32_t *gpuData = ((uint32_t *)baseAddr);
	short *sgpuData = ((short *)baseAddr);

	switch (primCmd)
	{
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
		fxy[0] = PrecisionRAM[currentAddress + 1];	//short[2];
		fxy[1] = PrecisionRAM[currentAddress + 2];	//short[4];
		fxy[2] = PrecisionRAM[currentAddress + 3];	//short[6];
		break;

	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
		fxy[0] = PrecisionRAM[currentAddress + 1];	//short[2];
		fxy[1] = PrecisionRAM[currentAddress + 3];	//short[6];
		fxy[2] = PrecisionRAM[currentAddress + 5];	//short[10];
		break;

	case 0x28:
	case 0x29:
	case 0x2A:
	case 0x2B:
		fxy[0] = PrecisionRAM[currentAddress + 1];	//short[2];
		fxy[1] = PrecisionRAM[currentAddress + 2];	//short[4];
		fxy[2] = PrecisionRAM[currentAddress + 3];	//short[6];
		fxy[3] = PrecisionRAM[currentAddress + 4];	//short[8];
		break;

	case 0x2C:
	case 0x2D:
	case 0x2E:
	case 0x2F:
		fxy[0] = PrecisionRAM[currentAddress + 1];	//short[2];
		fxy[1] = PrecisionRAM[currentAddress + 3];	//short[6];
		fxy[2] = PrecisionRAM[currentAddress + 5];	//short[10];
		fxy[3] = PrecisionRAM[currentAddress + 7];	//short[14];
		break;

	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
		fxy[0] = PrecisionRAM[currentAddress + 1];	//short[2];
		fxy[1] = PrecisionRAM[currentAddress + 3];	//short[6];
		fxy[2] = PrecisionRAM[currentAddress + 5];	//short[10];
		break;

	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
		fxy[0] = PrecisionRAM[currentAddress + 1];	//short[2];
		fxy[1] = PrecisionRAM[currentAddress + 4];	//short[8];
		fxy[2] = PrecisionRAM[currentAddress + 7];	//short[14];
		break;

	case 0x38:
	case 0x39:
	case 0x3A:
	case 0x3B:
		fxy[0] = PrecisionRAM[currentAddress + 1];	//short[2];
		fxy[1] = PrecisionRAM[currentAddress + 3];	//short[6];
		fxy[2] = PrecisionRAM[currentAddress + 5];	//short[10];
		fxy[2] = PrecisionRAM[currentAddress + 7];	//short[14];
		break;

	case 0x3C:
	case 0x3D:
	case 0x3E:
	case 0x3F:
		fxy[0] = PrecisionRAM[currentAddress + 1];	//short[2];
		fxy[1] = PrecisionRAM[currentAddress + 4];	//short[8];
		fxy[2] = PrecisionRAM[currentAddress + 7];	//short[14];
		fxy[3] = PrecisionRAM[currentAddress + 10];	//short[20];
		break;

	default:
		return;
	}

	//PLUGINLOG("NORM %X %d %d  %d %d  %d %d  %d %d", primCmd, *lx[0], *ly[0], *lx[1], *ly[1], *lx[2], *ly[2], *lx[3], *ly[3]);
	//PLUGINLOG("ACC  %X %f %f  %f %f  %f %f  %f %f", primCmd, fxy[0].x, fxy[0].y, fxy[1].x, fxy[1].y, fxy[2].x, fxy[2].y, fxy[3].x, fxy[3].y);

}


void GTEAccHack::fix_offsets(s32 count)
{
	for (int i = 0; i < count; ++i)
	{
		if (fxy[i].valid /*&& std::fabs(fxy[i].x - *lx[i]) < 1.0f && std::fabs(fxy[i].y - *ly[i]) < 1.0f*/)
		{
			vertex[i]->x = fxy[i].x + *PSXDisplay_CumulOffset_x;
			vertex[i]->y = fxy[i].y + *PSXDisplay_CumulOffset_y;
		}
	}
}

GTEAccHack::offset_fn GTEAccHack::ooffset3;
BOOL __cdecl GTEAccHack::offset3(void)
{
	BOOL ret = ooffset3();
	s_GTEAccHack->fix_offsets(3);
	return ret;
}

GTEAccHack::offset_fn GTEAccHack::ooffset4;
BOOL __cdecl GTEAccHack::offset4(void)
{
	BOOL ret = ooffset4();
	s_GTEAccHack->fix_offsets(4);
	return ret;
}

GTEAccHack::primPoly_fn GTEAccHack::oprimPolyF3;
void __cdecl GTEAccHack::primPolyF3(unsigned char *baseAddr)
{
	s_GTEAccHack->primPoly((u32*)baseAddr);
	oprimPolyF3(baseAddr);
}

GTEAccHack::primPoly_fn GTEAccHack::oprimPolyFT3;
void __cdecl GTEAccHack::primPolyFT3(unsigned char *baseAddr)
{
	s_GTEAccHack->primPoly((u32*)baseAddr);
	oprimPolyFT3(baseAddr);
}

GTEAccHack::primPoly_fn GTEAccHack::oprimPolyF4;
void __cdecl GTEAccHack::primPolyF4(unsigned char *baseAddr)
{
	s_GTEAccHack->primPoly((u32*)baseAddr);
	oprimPolyF4(baseAddr);
}

GTEAccHack::primPoly_fn GTEAccHack::oprimPolyFT4;
void __cdecl GTEAccHack::primPolyFT4(unsigned char *baseAddr)
{
	s_GTEAccHack->primPoly((u32*)baseAddr);
	oprimPolyFT4(baseAddr);
}

GTEAccHack::primPoly_fn GTEAccHack::oprimPolyG3;
void __cdecl GTEAccHack::primPolyG3(unsigned char *baseAddr)
{
	s_GTEAccHack->primPoly((u32*)baseAddr);
	oprimPolyG3(baseAddr);
}

GTEAccHack::primPoly_fn GTEAccHack::oprimPolyGT3;
void __cdecl GTEAccHack::primPolyGT3(unsigned char *baseAddr)
{
	s_GTEAccHack->primPoly((u32*)baseAddr);
	oprimPolyGT3(baseAddr);
}

GTEAccHack::primPoly_fn GTEAccHack::oprimPolyG4;
void __cdecl GTEAccHack::primPolyG4(unsigned char *baseAddr)
{
	s_GTEAccHack->primPoly((u32*)baseAddr);
	oprimPolyG4(baseAddr);
}

GTEAccHack::primPoly_fn GTEAccHack::oprimPolyGT4;
void __cdecl GTEAccHack::primPolyGT4(unsigned char *baseAddr)
{
	s_GTEAccHack->primPoly((u32*)baseAddr);
	oprimPolyGT4(baseAddr);
}