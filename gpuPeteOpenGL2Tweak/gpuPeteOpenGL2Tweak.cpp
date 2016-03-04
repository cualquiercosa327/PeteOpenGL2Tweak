
#include "stdafx.h"
#include "gpuPeteOpenGL2Tweak.h"
#include "GPUPlugin.h"
#include "GPUPatches.h"
#include "GTEAccHack.h"
#include "TextureScaler.h"

Context Context::instance;

Context::Context()
{
	MH_Initialize();
	config.reset(new Config);
	gpuPatches.reset(new GPUPatches);
}

Context::~Context()
{
	MH_Uninitialize();
}

s32 Context::OnGPUinit()
{
	CreateHook(GPUPlugin::Get().GPUopen, hookGPUopen, reinterpret_cast<void**>(&oGPUopen));
	EnableHook(GPUPlugin::Get().GPUopen);

	CreateHook(GPUPlugin::Get().GPUwriteDataMem, hookGPUwriteDataMem, reinterpret_cast<void**>(&oGPUwriteDataMem));
	EnableHook(GPUPlugin::Get().GPUwriteDataMem);

	if (config->GetxBRZScale() > 1)
	{
		if (!textureScaler)
			textureScaler.reset(new TextureScaler);
	}

	if (config->GetMulX() > 0 && config->GetMulY() > 0)
		gpuPatches->ResHack(config->GetMulX(), config->GetMulY());

	return GPUPlugin::Get().GPUinit();
}

s32(CALLBACK* Context::oGPUopen)(HWND hwndGPU);
s32 CALLBACK Context::hookGPUopen(HWND hwndGPU)
{
	s32 ret = oGPUopen(hwndGPU);

	static std::once_flag glewInitFlag;
	std::call_once(glewInitFlag, glewInit);

	if (context.GetConfig()->GetFixFullscreenAspect())
		context.gpuPatches->FixFullscreenAspect();

	if (context.GetConfig()->GetFixMemoryDetection())
		context.gpuPatches->FixMemoryDetection();

	HWND insertAfter = HWND_TOP;
	UINT flags = SWP_NOSIZE | SWP_NOMOVE | SWP_NOCOPYBITS | SWP_NOSENDCHANGING;

	if (context.GetConfig()->GetWindowOnTop())
		insertAfter = HWND_TOPMOST;

	if (context.GetConfig()->GetWindowX() > -1 && context.GetConfig()->GetWindowY() > -1)
		flags &= ~SWP_NOMOVE;

	SetWindowPos(hwndGPU, insertAfter, context.GetConfig()->GetWindowX(), context.GetConfig()->GetWindowY(), 0, 0, flags);
	context.gpuPatches->ApplyWindowProc(hwndGPU);

	if (context.GetConfig()->GetVSyncInterval())
		context.gpuPatches->EnableVsync(context.GetConfig()->GetVSyncInterval());

	if (context.GetConfig()->GetHideCursor())
		while (ShowCursor(FALSE) > -1);

	return ret;
}

s32 Context::OnGPUclose()
{
	if (config->GetVSyncInterval())
		gpuPatches->EnableVsync(0);

	if (config->GetHideCursor()) while (ShowCursor(TRUE) < 0);
	return GPUPlugin::Get().GPUclose();
}

void Context::OnGPUsetframelimit(u32 option)
{
	if (config->GetDisableSetFrameLimit())
		return;

	return GPUPlugin::Get().GPUsetframelimit(option);
}

s32 Context::OnGPUdmaChain(u32 * baseAddrL, u32 addr)
{
	if (config->GetGTEAccuracy())
	{
		if (!gteAccHack)
			gteAccHack.reset(new GTEAccHack);

		gteAccHack->OnDmaChain(baseAddrL, addr);
	}
	return GPUPlugin::Get().GPUdmaChain(baseAddrL, addr);
}


void (CALLBACK* Context::oGPUwriteDataMem)(u32* pMem, s32 iSize);
void CALLBACK Context::hookGPUwriteDataMem(u32* pMem, s32 iSize)
{
	if (context.gteAccHack)
		context.gteAccHack->OnWriteDataMem(pMem, iSize);

	return oGPUwriteDataMem(pMem, iSize);
}

void Context::OnGteFifoAdd(s64 llx, s64 lly, u16 z)
{
	if (config->GetGTEAccuracy())
	{
		if (!gteAccHack)
			gteAccHack.reset(new GTEAccHack);

		return gteAccHack->GteFifoAdd(llx, lly, z);
	}
}

void Context::OnGteFifoInvalidate(u32 cmd)
{
	if (gteAccHack)
		return gteAccHack->GteFifoInvalidate(cmd);

}

void Context::OnGteTransferToRam(u32 address, u32 cmd)
{
	if (gteAccHack)
		return gteAccHack->GteTransferToRam(address, cmd);
}

void Context::OnGTEwriteDataMem(u32* pMem, s32 size, u32 address)
{
	if (gteAccHack)
		return gteAccHack->GTEwriteDataMem(pMem, size, address);
}

