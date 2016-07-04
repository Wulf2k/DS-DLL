#include "Project.h"
#include "DllMain.h"
#include "Test.h"
#include "Unloader.h"
#include "Console.h"
#include <Windows.h>
#include <windowsx.h>
#include <WinBase.h>
#include <stdio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <Minhook.h>
#include <process.h>
#pragma comment(lib, "libMinHook.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")


using namespace std;

DLLEXPORT void Initialize();
DLLEXPORT void Run();
DLLEXPORT void Cleanup();
DLLEXPORT void __cdecl  hotkeyThread(void*);


BOOL WINAPI OnConsoleSignal(DWORD dwCtrlType);
DWORD pGlobalChainD3Device = NULL;

typedef HRESULT(WINAPI* tBeginScene)(LPDIRECT3DDEVICE9 pDevice);
typedef HRESULT(WINAPI* tCreateTexture)(LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9 **ppTexture, HANDLE *pSharedHAndle);
typedef HRESULT(WINAPI* tCreateVertexBuffer)(LPDIRECT3DDEVICE9 pDevice, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle);
typedef HRESULT(WINAPI* tDrawIndexedPrimitive)(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);
typedef HRESULT(WINAPI* tDrawIndexedPrimitiveUP)(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, void *pIndexData, D3DFORMAT IndexDataFormat, void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
typedef HRESULT(WINAPI* tDrawPrimitive)(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE pPrimitiveType, UINT startVertex, UINT primitiveCount);
typedef HRESULT(WINAPI* tDrawPrimitiveUP)(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE pPrimitiveType, UINT primitiveCount, void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
typedef HRESULT(WINAPI* tGetLight)(LPDIRECT3DDEVICE9 pDevice, DWORD Index, D3DLIGHT9 *pLight);
typedef HRESULT(WINAPI* tGetStreamSource)(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9 **ppStreamData, UINT *pOffsetInBytes, UINT *pStride);
typedef HRESULT(WINAPI* tLightEnable)(LPDIRECT3DDEVICE9 pDevice, DWORD LightIndex, BOOL bEnable);
typedef HRESULT(WINAPI* tEndScene)(LPDIRECT3DDEVICE9 pDevice);
typedef HRESULT(WINAPI* tReset)(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
typedef HRESULT(WINAPI* tSetLight)(LPDIRECT3DDEVICE9 pDevice, DWORD Index, D3DLIGHT9 *pLight);
typedef HRESULT(WINAPI* tSetRenderState)(LPDIRECT3DDEVICE9 pDevice, D3DRENDERSTATETYPE pState, DWORD value);
typedef HRESULT(WINAPI* tSetStreamSource)(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes, UINT Stride);
typedef HRESULT(WINAPI* tSetTexture)(LPDIRECT3DDEVICE9 pDevice, DWORD Sampler, IDirect3DBaseTexture9 *pTexture);

tBeginScene oBeginScene = NULL;
tCreateTexture oCreateTexture = NULL;
tCreateVertexBuffer oCreateVertexBuffer = NULL;
tDrawIndexedPrimitive oDrawIndexedPrimitive = NULL;
tDrawIndexedPrimitiveUP oDrawIndexedPrimitiveUP = NULL;
tDrawPrimitive oDrawPrimitive = NULL;
tDrawPrimitiveUP oDrawPrimitiveUP = NULL;
tEndScene oEndScene = NULL;
tGetLight oGetLight = NULL;
tGetStreamSource oGetStreamSource = NULL;
tLightEnable oLightEnable = NULL;
tReset oReset = NULL;
tSetLight oSetLight = NULL;
tSetRenderState oSetRenderState = NULL;
tSetStreamSource oSetStreamSource = NULL;
tSetTexture oSetTexture = NULL;

HANDLE hHotkeyThread;


LPDIRECT3DDEVICE9 pD3dDevice;
D3DVIEWPORT9 d3dViewport;       // Resolution
LPD3DXFONT gFont;
LPD3DXFONT giFont;

// Pointer Math
#define ADDPTR(ptr, add) PVOID((PBYTE(ptr) + size_t(add)))
#define SUBPTR(ptr, add) PVOID((PBYTE(ptr) - size_t(add)))
#define DEREF(ptr, add, type) *static_cast<type*>(ADDPTR(ptr,add))

#define dx_endscene 0
#define dx_reset 1

#define ALIGN_TOP 1
#define ALIGN_LEFT 2
#define ALIGN_RIGHT 3
#define ALIGN_BOTTOM 4
#define ALIGN_CENTER 5

#define FONT_SIZE_1080 18
#define FONT_SIZE_720 18
#define FONT_SPACING_1080 19
#define FONT_SPACING_720 19

#define C_BLACK D3DCOLOR_RGBA(0,0,0,255)
#define C_WHITE D3DCOLOR_RGBA(255,255,255,255)
#define C_LIGHTGRAY D3DCOLOR_RGBA(235,235,235,255)
#define C_DARKGRAY D3DCOLOR_RGBA(100,100,100,255)
#define C_TRANSBLACK D3DCOLOR_RGBA(0, 0, 0, 100)
#define C_TRANSGRAY D3DCOLOR_RGBA(128, 128, 128, 128)



struct sDXFunctions
{
	DWORD BeginSceneAddress;
	DWORD CreateTextureAddress;
	DWORD CreateVertexBufferAddress;
	DWORD DrawIndexedPrimitiveAddress;
	DWORD DrawIndexedPrimitiveUPAddress;
	DWORD DrawPrimitiveAddress;
	DWORD DrawPrimitiveUPAddress;
	DWORD EndSceneAddress;
	DWORD GetLightAddress;
	DWORD GetStreamSourceAddress;
	DWORD LightEnableAddress;
	DWORD ResetAddress;
	DWORD SetLightAddress;
	DWORD SetRenderStateAddress;
	DWORD SetStreamSourceAddress;
	DWORD SetTextureAddress;
};
sDXFunctions DXFunctions;

enum DXVTable
{
	QueryInterface, // 0
	AddRef, // 1
	Release, // 2
	TestCooperativeLevel, // 3
	GetAvailableTextureMem, // 4
	EvictManagedResources, // 5
	GetDirect3D, // 6
	_GetDeviceCaps, // 7
	GetDisplayMode, // 8
	GetCreationParameters, // 9
	SetCursorProperties, // 10
	SetCursorPosition, // 11
	_ShowCursor, // 12
	CreateAdditionalSwapChain, // 13
	GetSwapChain, // 14
	GetNumberOfSwapChains, // 15
	Reset, // 16
	Present, // 17
	GetBackBuffer, // 18
	GetRasterStatus, // 19
	SetDialogBoxMode, // 20
	SetGammaRamp, // 21
	GetGammaRamp, // 22
	CreateTexture, // 23
	CreateVolumeTexture, // 24
	CreateCubeTexture, // 25
	CreateVertexBuffer, // 26
	CreateIndexBuffer, // 27
	CreateRenderTarget, // 28
	CreateDepthStencilSurface, // 29
	UpdateSurface, // 30
	UpdateTexture, // 31
	GetRenderTargetData, // 32
	GetFrontBufferData, // 33
	StretchRect, // 34
	ColorFill, // 35
	CreateOffscreenPlainSurface, // 36
	SetRenderTarget, // 37
	GetRenderTarget, // 38
	SetDepthStencilSurface, // 39
	GetDepthStencilSurface, // 40
	BeginScene, // 41
	EndScene, // 42
	Clear, // 43
	SetTransform, // 44
	GetTransform, // 45
	MultiplyTransform, // 46
	SetViewport, // 47
	GetViewport, // 48
	SetMaterial, // 49
	GetMaterial, // 50
	SetLight, // 51
	GetLight, // 52
	LightEnable, // 53
	GetLightEnable, // 54
	SetClipPlane, // 55
	GetClipPlane, // 56
	SetRenderState, // 57
	GetRenderState, // 58
	CreateStateBlock, // 59
	BeginStateBlock, // 60
	EndStateBlock, // 61
	SetClipStatus, // 62
	GetClipStatus, // 63
	GetTexture, // 64
	SetTexture, // 65
	GetTextureStageState, // 66
	SetTextureStageState, // 67
	GetSamplerState, // 68
	SetSamplerState, // 69
	ValidateDevice, // 70
	_SetPaletteEntries, // 71
	_GetPaletteEntries, // 72
	SetCurrentTexturePalette, // 73
	GetCurrentTexturePalette, // 74
	SetScissorRect, // 75
	GetScissorRect, // 76
	SetSoftwareVertexProcessing, // 77
	GetSoftwareVertexProcessing, // 78
	SetNPatchMode, // 79
	GetNPatchMode, // 80
	DrawPrimitive, // 81
	DrawIndexedPrimitive, // 82
	DrawPrimitiveUP, // 83
	DrawIndexedPrimitiveUP, // 84
	ProcessVertices, // 85
	CreateVertexDeclaration, // 86
	SetVertexDeclaration, // 87
	GetVertexDeclaration, // 88
	SetFVF, // 89
	GetFVF, // 90
	CreateVertexShader, // 91
	SetVertexShader, // 92
	GetVertexShader, // 93
	SetVertexShaderConstantF, // 94
	GetVertexShaderConstantF, // 95
	SetVertexShaderConstantI, // 96
	GetVertexShaderConstantI, // 97
	SetVertexShaderConstantB, // 98
	GetVertexShaderConstantB, // 99
	SetStreamSource, // 100
	GetStreamSource, // 101
	SetStreamSourceFreq, // 102
	GetStreamSourceFreq, // 103
	SetIndices, // 104
	GetIndices, // 105
	CreatePixelShader, // 106
	SetPixelShader, // 107
	GetPixelShader, // 108
	SetPixelShaderConstantF, // 109
	GetPixelShaderConstantF, // 110
	SetPixelShaderConstantI, // 111
	GetPixelShaderConstantI, // 112
	SetPixelShaderConstantB, // 113
	GetPixelShaderConstantB, // 114
	DrawRectPatch, // 115
	DrawTriPatch, // 116
	DeletePatch, // 117
	CreateQuery // 118
};
struct charPos
{
	unsigned int unk1;
	float facing;
	BYTE unk2[0x8];
	float xPos;
	float yPos;
	float zPos;
};
struct charLocData
{
	BYTE unk1[0x1c];
	charPos* pos;
};
struct creature
{
	BYTE unk1[0x28];
	charLocData* loc;
	BYTE unk2[0xC];
	char modelName[0x10];
	BYTE unk3[0x28];
	unsigned int PhantomType;
	unsigned int TeamType;
	BYTE unk4[0x25C];
	unsigned int currHP;
	unsigned int maxHP;
	BYTE unk5[0x8];
	unsigned int currStam;
	unsigned int maxStam;
};



struct LoadedCreatures
{
	PVOID pUnknown;             // 0x137DC70
	creature* firstCreature;    // +4
	creature* lastCreature;     // +8
};


HRESULT WINAPI hBeginScene(LPDIRECT3DDEVICE9 pDevice);
HRESULT WINAPI hCreateTexture(LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9 **ppTexture, HANDLE *pSharedHAndle);
HRESULT WINAPI hCreateVertexBuffer(LPDIRECT3DDEVICE9 pDevice, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle);
HRESULT WINAPI hDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);
HRESULT WINAPI hDrawIndexedPrimitiveUP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, void *pIndexData, D3DFORMAT IndexDataFormat, void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
HRESULT WINAPI hDrawPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE pPrimativeType, UINT startVertex, UINT primitiveCount);
HRESULT WINAPI hDrawPrimitiveUP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE pPrimitiveType, UINT primitiveCount, void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
HRESULT WINAPI hEndScene(LPDIRECT3DDEVICE9 pDevice);
HRESULT WINAPI hGetLight(LPDIRECT3DDEVICE9 pDevice, DWORD Index, D3DLIGHT9 *pLight);
HRESULT WINAPI hGetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9 **ppStreamData, UINT *pOffsetInBytes, UINT *pStride);
HRESULT WINAPI hLightEnable(LPDIRECT3DDEVICE9 pDevice, DWORD LightIndex, BOOL bEnable);
HRESULT WINAPI hSetLight(LPDIRECT3DDEVICE9 pDevice, DWORD Index, D3DLIGHT9 *pLight);
HRESULT WINAPI hSetRenderState(LPDIRECT3DDEVICE9 pDevice, D3DRENDERSTATETYPE pState, DWORD value);
HRESULT WINAPI hSetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes, UINT Stride);
HRESULT WINAPI hSetTexture(LPDIRECT3DDEVICE9 pDevice, DWORD Sampler, IDirect3DBaseTexture9 *pTexture);

void initDXFunctions();
HRESULT GenerateTexture(IDirect3DDevice9 *pD3Ddev, IDirect3DTexture9 **ppD3Dtex, DWORD colour32);

int fontSize = FONT_SIZE_1080;
int fontLineSpacing = FONT_SPACING_1080;
bool bRunning = false;
bool gVisible = true;

bool wireframe = false;

bool debugEXE = false;


bool Initialize_DirectX()
{
	printf("Initialize_DirectX Called.\n");

	Console::Create("My Console");


	DWORD dbgchk;
	dbgchk = 0x00400080;

	__asm {
		mov eax, dbgchk
		mov eax, [eax]
		mov dbgchk, eax
	}

	if (dbgchk == 0xCE9634B4)
	{
		debugEXE = true;
		pGlobalChainD3Device = 0x13A882C;
	}
	else
	{
		debugEXE = false;
		pGlobalChainD3Device = 0x13A466C;
	}



	pD3dDevice = 0;

	__asm {
		mov eax, pGlobalChainD3Device
		cmp eax, 0
		je ptrZero
		mov eax, [eax]
		cmp eax, 0
		je ptrZero
		mov eax, [eax]
		cmp eax, 0
		je ptrZero
		mov eax, [eax + 0x10]
		cmp eax, 0
		je ptrZero
		mov pD3dDevice, eax

		ptrZero :
	}

	if (pD3dDevice == NULL)
		return false;

	printf("pD3dDevice: %p\n", pD3dDevice);



	if (!dbgchk)
	{
		DWORD OldProtect = 0;
		VirtualProtect((LPVOID)0xbe73fe, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
		VirtualProtect((LPVOID)0xbe719f, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
		VirtualProtect((LPVOID)0xbe722b, 1, PAGE_EXECUTE_READWRITE, &OldProtect);


		__asm {
			mov eax, 0xBE73FE
			mov bx, 04
			mov[eax], bx

			mov eax, 0xBE719F
			mov[eax], bx

			mov eax, 0xBE722B
			mov[eax], bx
		}
	}



	initDXFunctions();



	pD3dDevice->GetViewport(&d3dViewport);
	printf("d3dViewport.width: %d\n", d3dViewport.Width);
	printf("d3dViewport.height: %d\n", d3dViewport.Height);
	//printf("endHook: %p\n", DXFunctions.EndSceneAddress);
	//printf("resetHook: %p\n", DXFunctions.ResetAddress);
	//printf("setRenderStateHook: %p\n", DXFunctions.SetRenderStateAddress);

	printf("DrawIndexedPrimitive: %p\n", DXFunctions.DrawIndexedPrimitiveAddress);
	printf("DrawIndexedPrimitiveUP: %p\n", DXFunctions.DrawIndexedPrimitiveUPAddress);
	printf("DrawPrimitive: %p\n", DXFunctions.DrawPrimitiveAddress);
	printf("DrawPrimitiveUP: %p\n", DXFunctions.DrawPrimitiveUPAddress);
	printf("Finished Initialize_DirectX\n");


	return true;
}
bool CreateDefaultFont()
{
	if (gFont == NULL)
	{
		printf("\tgFont == NULL\n");
		pD3dDevice->GetViewport(&d3dViewport);
		fontLineSpacing = (d3dViewport.Height >= 1080 ? FONT_SPACING_1080 : FONT_SPACING_720);
		fontSize = (d3dViewport.Height >= 1080 ? FONT_SIZE_1080 : FONT_SIZE_720);

		if (HRESULT fontCreateResult = D3DXCreateFont(pD3dDevice, fontSize, 0, FW_NORMAL, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas", &gFont) != S_OK)
		{
			printf("Problem creating Font: Result = %lu  -  %08X\n", fontCreateResult, fontCreateResult);
			return false;
		}
		printf("\tCreated gFont: %p\n", gFont);
		printf("\tFont Size: %d\n", fontSize);
		printf("\tFont Line Spacing: %d\n", fontLineSpacing);
	}

	if (giFont == NULL)
	{
		printf("\tgiFont == NULL\n");
		pD3dDevice->GetViewport(&d3dViewport);
		fontLineSpacing = (d3dViewport.Height >= 1080 ? FONT_SPACING_1080 : FONT_SPACING_720);
		fontSize = (d3dViewport.Height >= 1080 ? FONT_SIZE_1080 : FONT_SIZE_720);

		if (HRESULT fontCreateResult = D3DXCreateFont(pD3dDevice, (fontSize + 2), 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas", &giFont) != S_OK)
		{
			printf("Problem creating Font: Result = %lu  -  %08X\n", fontCreateResult, fontCreateResult);
			return false;
		}
		printf("\tCreated giFont: %p\n", giFont);
		printf("\tFont Size: %d\n", fontSize);
		printf("\tFont Line Spacing: %d\n", fontLineSpacing);
	}

	return ((gFont != NULL) && (giFont != NULL));
}

void DrawScreenText(LPD3DXFONT font, wstring text, int x, int y, D3DCOLOR color)
{
	if (font == NULL)
		return;

	RECT pos; pos.left = x; pos.top = y;
	font->DrawText(0, text.c_str(), -1, &pos, DT_NOCLIP, color);
}
void DrawGradientBox(IDirect3DDevice9* pDevice, float x, float y, float w, float h, D3DCOLOR startColor, D3DCOLOR endColor)
{
	struct D3DVERTEX
	{
		float x, y, z, rhw;
		D3DCOLOR color;

	}
	vertices[] =
	{
		{ x, y, 0, 1.0f, startColor },
		{ x + w, y, 0, 1.0f, endColor },
		{ x, y + h, 0, 1.0f, startColor },
		{ x + w, y + h, 0, 1.0f, endColor }
	};

	pDevice->SetTexture(0, nullptr);

	pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
	pDevice->SetRenderState(D3DRS_ALPHAREF, (DWORD)8);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3DRS_DESTBLENDALPHA);
	pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	pDevice->SetRenderState(D3DRS_FOGENABLE, D3DZB_FALSE);

	pDevice->SetFVF(0x004 | 0x040 | 0X100);
	pDevice->SetFVF(0x004 | 0x040);
	pDevice->SetTexture(0, nullptr);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(D3DVERTEX));
}

void drawStuff()
{



}


HRESULT WINAPI hBeginScene(LPDIRECT3DDEVICE9 pDevice)
{
	//printf("BeginScene called.\n");

	HRESULT tmp;
	tmp = oBeginScene(pDevice);
	return tmp;
}
HRESULT WINAPI hCreateTexture(LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9 **ppTexture, HANDLE *pSharedHAndle)
{
	//printf("CreateTexture called.  ppTexture:  %p\n", ppTexture);


	HRESULT tmp;
	tmp = oCreateTexture(pDevice, Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHAndle);
	return tmp;

}
HRESULT WINAPI hCreateVertexBuffer(LPDIRECT3DDEVICE9 pDevice, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle)
{
	//printf("CreateVertexBuffer called.  FVF: %p\n", &FVF);

	HRESULT tmp;
	tmp = oCreateVertexBuffer(pDevice, Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle);
	return tmp;
}
HRESULT WINAPI hDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
	//printf("DrawIndexedPrimitive Called. Type: %d, BaseVert: %d, MinIndex: %d, NumVert: %d, StartIndex: %d, PrimCount: %d\n", Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);




	if (wireframe)
		pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);


	HRESULT tmp;

	tmp = oDrawIndexedPrimitive(pDevice, Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);
	
	if (wireframe)
		pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	

	return tmp;
	
}
HRESULT WINAPI hDrawIndexedPrimitiveUP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, void *pIndexData, D3DFORMAT IndexDataFormat, void *pVertexStreamZeroData, UINT VertexStreamZeroStride) 
{
	//printf("DrawIndexedPrimitiveUP Called.\n");
	//pD3dDevice->SetTexture(0, texBlue);
	return oDrawIndexedPrimitiveUP(pDevice, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}
HRESULT WINAPI hDrawPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE primitiveType, UINT startVertex, UINT primitiveCount)
{
	//printf("DrawPrimitive Called.\n");
	//pD3dDevice->SetTexture(0, texBlue);

	HRESULT tmp;
	tmp = oDrawPrimitive(pDevice, primitiveType, startVertex, primitiveCount);
	return tmp;
}
HRESULT WINAPI hDrawPrimitiveUP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE primitiveType, UINT primitiveCount, void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	//printf("DrawPrimitiveUP Called.\n");
	//pD3dDevice->SetTexture(0, texBlue);
	return oDrawPrimitiveUP(pDevice, primitiveType, primitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}
HRESULT WINAPI hEndScene(LPDIRECT3DDEVICE9 pDevice)
{


	if (!gVisible) return oEndScene(pDevice);

	if (!CreateDefaultFont())
	{
		printf("Problem creating default font: returning to normal EndScene\n");
		return oEndScene(pDevice);
	}


	drawStuff();


	return oEndScene(pDevice);
}
HRESULT WINAPI hGetLight(LPDIRECT3DDEVICE9 pDevice, DWORD Index, D3DLIGHT9 *pLight)
{
	//printf("GetLight called.\n");

	HRESULT tmp;
	tmp = oGetLight(pDevice, Index, pLight);
	return tmp;
}
HRESULT WINAPI hGetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9 **ppStreamData, UINT *pOffsetInBytes, UINT *pStride)
{
	//printf("GetStreamSource called.\n");

	HRESULT tmp;
	tmp = oGetStreamSource(pDevice, StreamNumber, ppStreamData, pOffsetInBytes, pStride);
	return tmp;
}
HRESULT WINAPI hLightEnable(LPDIRECT3DDEVICE9 pDevice, DWORD LightIndex, BOOL bEnable)
{
	//printf("hLightEnable() called. \n");
	HRESULT tmp;
	tmp = oLightEnable(pDevice, LightIndex, bEnable);
	return tmp;
}
HRESULT WINAPI hReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	printf("hReset() called.\n");

	if (gFont)
		gFont->OnLostDevice(); // Avoid device loss errors


	HRESULT hReturn = oReset(pDevice, pPresentationParameters);
	if (hReturn == D3D_OK && gFont)
		gFont->OnResetDevice();

	gFont = NULL;

	CreateDefaultFont();


	return hReturn;
}
HRESULT WINAPI hSetLight(LPDIRECT3DDEVICE9 pDevice, DWORD Index, D3DLIGHT9 *pLight)
{
	//printf("hSetLight() called.  %p\n", &pLight);

	HRESULT tmp;
	tmp = oSetLight(pDevice, Index, pLight);
	return tmp;
}
HRESULT WINAPI hSetRenderState(LPDIRECT3DDEVICE9 pDevice, D3DRENDERSTATETYPE pState, DWORD value)
{
	//printf("hSetRenderState() called.\n");



	if (pState == D3DRS_ALPHABLENDENABLE) {
		//printf("pState: %d\n", pState);
		//value = 0;
	}

	if (pState == D3DRS_AMBIENT) {
		//printf("pState: %d\n", pState);
		//value = 0;
	}

	if (pState == D3DRS_COLORWRITEENABLE) {
		//printf("pState: %d\n", pState);
		//value = 8;
	}

	if (pState == D3DRS_CULLMODE) {
		//value = 3;
	}

	if (pState == D3DRS_DESTBLEND) {
		//printf("pState: %d\n", pState);
		//value = 5;
	}

	if (pState == D3DRS_FILLMODE) {
		//printf("pState: %d\n", pState);
	}

	if (pState == D3DRS_FOGENABLE) {
		//printf("pState: %d\n", pState);
	}

	if (pState == D3DRS_FOGSTART) {
		//printf("pState: %d\n", pState);
		//value = 0;
	}

	if (pState == D3DRS_LIGHTING) {
		//printf("pState: %d\n", pState);
		//value = 0;
	}

	if (pState == D3DRS_ZENABLE) {
		//printf("pState: %d\n", pState);
		//value = 0;
	}



	if (pState != D3DRS_FILLMODE && pState != D3DRS_CULLMODE && pState != D3DRS_ALPHABLENDENABLE && 
		pState != D3DRS_DEPTHBIAS && pState != D3DRS_SEPARATEALPHABLENDENABLE && pState != D3DRS_DESTBLEND && 
		pState != D3DRS_SRCBLEND && pState != D3DRS_BLENDOP && pState != D3DRS_BLENDFACTOR && 
		pState != D3DRS_ALPHAFUNC) {
		//printf("pState: %d, %d\n", pState, value);
	}



	HRESULT tmp;
	tmp = oSetRenderState(pDevice, pState, value);
	return tmp;
}
HRESULT WINAPI hSetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes, UINT Stride)
{
	//printf("SetStreamSource called.  Stride:  %d, StreamNumber:  %d, StreamData:  %p\n", Stride, StreamNumber, pStreamData);

	HRESULT tmp;
	tmp = oSetStreamSource(pDevice, StreamNumber, pStreamData, OffsetInBytes, Stride);
	return tmp;
}
HRESULT WINAPI hSetTexture(LPDIRECT3DDEVICE9 pDevice, DWORD Sampler, IDirect3DBaseTexture9 *pTexture)
{
	//printf("hSetTexture() called.  %d, %p\n", Sampler, &pTexture);
	HRESULT tmp;
	tmp = oSetTexture(pDevice, Sampler, pTexture);
	return tmp;
}

void initDXFunctions()
{
	DWORD* pVTable = (DWORD*)pD3dDevice;
	pVTable = (DWORD*)pVTable[0];
	DXFunctions.BeginSceneAddress = pVTable[DXVTable::BeginScene];
	DXFunctions.CreateTextureAddress = pVTable[DXVTable::CreateTexture];
	DXFunctions.CreateVertexBufferAddress = pVTable[DXVTable::CreateVertexBuffer];
	DXFunctions.DrawIndexedPrimitiveAddress = pVTable[DXVTable::DrawIndexedPrimitive];
	DXFunctions.DrawIndexedPrimitiveUPAddress = pVTable[DXVTable::DrawIndexedPrimitiveUP];
	DXFunctions.DrawPrimitiveAddress = pVTable[DXVTable::DrawPrimitive];
	DXFunctions.DrawPrimitiveUPAddress = pVTable[DXVTable::DrawPrimitiveUP];
	DXFunctions.EndSceneAddress = pVTable[DXVTable::EndScene];
	DXFunctions.GetLightAddress = pVTable[DXVTable::GetLight];
	DXFunctions.GetStreamSourceAddress = pVTable[DXVTable::GetStreamSource];
	DXFunctions.LightEnableAddress = pVTable[DXVTable::LightEnable];
	DXFunctions.ResetAddress = pVTable[DXVTable::Reset];
	DXFunctions.SetLightAddress = pVTable[DXVTable::SetLight];
	DXFunctions.SetRenderStateAddress = pVTable[DXVTable::SetRenderState];
	DXFunctions.SetStreamSourceAddress = pVTable[DXVTable::SetStreamSource];
	DXFunctions.SetTextureAddress = pVTable[DXVTable::SetTexture];

}

DLLEXPORT void __cdecl Start(void*)
{
	Unloader::Initialize(hDll);

	Console::Create("DS-DLL");

	if (!SetConsoleCtrlHandler(OnConsoleSignal, TRUE)) {
		printf("\nERROR: Could not set control handler\n");
		return;
	}

	printf("Initializing\n");
	Initialize();
	Run();
	Cleanup();

	SetConsoleCtrlHandler(OnConsoleSignal, FALSE);
	Console::Free();
	Unloader::UnloadSelf(true);		// Unloading on a new thread fixes an unload issue
}
DWORD ModuleCheckingThread()
{
	if (MH_Initialize() != MH_OK)
		return -1;

	*(PDWORD)&oBeginScene = (DWORD)DXFunctions.BeginSceneAddress;
	*(PDWORD)&oCreateTexture = (DWORD)DXFunctions.CreateTextureAddress;
	*(PDWORD)&oCreateVertexBuffer = (DWORD)DXFunctions.CreateVertexBufferAddress;
	*(PDWORD)&oDrawIndexedPrimitive = (DWORD)DXFunctions.DrawIndexedPrimitiveAddress;
	*(PDWORD)&oDrawIndexedPrimitiveUP = (DWORD)DXFunctions.DrawIndexedPrimitiveUPAddress;
	*(PDWORD)&oDrawPrimitive = (DWORD)DXFunctions.DrawPrimitiveAddress;
	*(PDWORD)&oDrawPrimitiveUP = (DWORD)DXFunctions.DrawPrimitiveUPAddress;
	*(PDWORD)&oEndScene = (DWORD)DXFunctions.EndSceneAddress;
	*(PDWORD)&oGetLight = (DWORD)DXFunctions.GetLightAddress;
	*(PDWORD)&oGetStreamSource = (DWORD)DXFunctions.GetStreamSourceAddress;
	*(PDWORD)&oLightEnable = (DWORD)DXFunctions.LightEnableAddress;
	*(PDWORD)&oReset = (DWORD)DXFunctions.ResetAddress;
	*(PDWORD)&oSetLight = (DWORD)DXFunctions.SetLightAddress;
	*(PDWORD)&oSetRenderState = (DWORD)DXFunctions.SetRenderStateAddress;
	*(PDWORD)&oSetStreamSource = (DWORD)DXFunctions.SetStreamSourceAddress;
	*(PDWORD)&oSetTexture = (DWORD)DXFunctions.SetTextureAddress;



	if (MH_CreateHook((void*)DXFunctions.BeginSceneAddress, &hBeginScene, reinterpret_cast<void**>(&oBeginScene)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.BeginSceneAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.CreateTextureAddress, &hCreateTexture, reinterpret_cast<void**>(&oCreateTexture)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.CreateTextureAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.CreateVertexBufferAddress, &hCreateVertexBuffer, reinterpret_cast<void**>(&oCreateVertexBuffer)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.CreateVertexBufferAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.DrawIndexedPrimitiveAddress, &hDrawIndexedPrimitive, reinterpret_cast<void**>(&oDrawIndexedPrimitive)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.DrawIndexedPrimitiveAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.DrawIndexedPrimitiveUPAddress, &hDrawIndexedPrimitiveUP, reinterpret_cast<void**>(&oDrawIndexedPrimitiveUP)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.DrawIndexedPrimitiveUPAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.DrawPrimitiveAddress, &hDrawPrimitive, reinterpret_cast<void**>(&oDrawPrimitive)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.DrawPrimitiveAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.DrawPrimitiveUPAddress, &hDrawPrimitiveUP, reinterpret_cast<void**>(&oDrawPrimitiveUP)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.DrawPrimitiveUPAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.EndSceneAddress, &hEndScene, reinterpret_cast<void**>(&oEndScene)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.EndSceneAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.GetLightAddress, &hGetLight, reinterpret_cast<void**>(&oGetLight)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.GetLightAddress) != MH_OK)
		return -1;
	

	if (MH_CreateHook((void*)DXFunctions.GetStreamSourceAddress, &hGetStreamSource, reinterpret_cast<void**>(&oGetStreamSource)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.GetStreamSourceAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.LightEnableAddress, &hLightEnable, reinterpret_cast<void**>(&oLightEnable)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.LightEnableAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.ResetAddress, &hReset, reinterpret_cast<void**>(&oReset)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.ResetAddress) != MH_OK)
		return -1;

	
	if (MH_CreateHook((void*)DXFunctions.SetLightAddress, &hSetLight, reinterpret_cast<void**>(&oSetLight)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.SetLightAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.SetRenderStateAddress, &hSetRenderState, reinterpret_cast<void**>(&oSetRenderState)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.SetRenderStateAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.SetStreamSourceAddress, &hSetStreamSource, reinterpret_cast<void**>(&oSetStreamSource)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.SetStreamSourceAddress) != MH_OK)
		return -1;


	if (MH_CreateHook((void*)DXFunctions.SetTextureAddress, &hSetTexture, reinterpret_cast<void**>(&oSetTexture)))
		return -1;
	if (MH_EnableHook((void*)DXFunctions.SetTextureAddress) != MH_OK)
		return -1;


	return 0;
}
HRESULT GenerateTexture(IDirect3DDevice9 *pD3Ddev, IDirect3DTexture9 **ppD3Dtex, DWORD colour32)
{
	if (FAILED(pD3Ddev->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, ppD3Dtex, NULL)))
		return E_FAIL;

	WORD colour16 = ((WORD)((colour32 >> 28) & 0xF) << 12)
		| (WORD)(((colour32 >> 20) & 0xF) << 8)
		| (WORD)(((colour32 >> 12) & 0xF) << 4)
		| (WORD)(((colour32 >> 4) & 0xF) << 0);

	D3DLOCKED_RECT d3dlr;
	(*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
	WORD *pDst16 = (WORD*)d3dlr.pBits;

	for (int xy = 0; xy < 8 * 8; xy++)
		*pDst16++ = colour16;

	(*ppD3Dtex)->UnlockRect(0);

	return S_OK;
}


void Initialize()
{
	HANDLE hDarkSouls = GetModuleHandleA("DarkSouls.exe");

	if (!hDarkSouls)
	{	
		printf("ERROR: Getting handle to darksouls.exe\n");
		return;
	}

	printf("Handle: %p\n", hDarkSouls);

	Initialize_DirectX();
	CreateDefaultFont();
	ModuleCheckingThread();

	printf("Last Error: %d\n", GetLastError());

	_beginthread(&hotkeyThread, 0, 0);

	
}
void Cleanup()
{
	MH_DisableHook(MH_ALL_HOOKS);


	if (MH_RemoveHook((void*)DXFunctions.BeginSceneAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for BeginSceneAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.CreateTextureAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for CreateTextureAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.CreateVertexBufferAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for CreateVertexBufferAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.DrawIndexedPrimitiveUPAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for DrawIndexedPrimitiveUPAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.DrawPrimitiveAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for DrawPrimitiveAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.DrawPrimitiveUPAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for DrawPrimitiveUPAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.EndSceneAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for EndSceneAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.GetLightAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for GetLightAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.GetStreamSourceAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for GetStreamSourceAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.LightEnableAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for LightEnableAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.ResetAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for ResetAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.SetLightAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for SetLightAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.SetRenderStateAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for SetRenderStateAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.SetStreamSourceAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for SetTextureAddress\n");
		return;
	}
	if (MH_RemoveHook((void*)DXFunctions.SetTextureAddress) != MH_OK)
	{
		printf("Error: Could not remove the hook for SetTextureAddress\n");
		return;
	}
}
void Run()
{
	bRunning = true;




	while (bRunning)
	{		

		

		Sleep(33);
	}
}

BOOL WINAPI OnConsoleSignal(DWORD dwCtrlType) {

	if (dwCtrlType == CTRL_C_EVENT)
	{
		printf("Ctrl-C handled, exiting...\n"); // do cleanup
		bRunning = false;
		return TRUE;
	}

	return FALSE;
}


DLLEXPORT void __cdecl hotkeyThread(void*)
{
	printf("hotkeyThread() called\n");

	bool hk_Enter_Pressed = false;
	
	bool hk_Num1_Pressed = false;
	bool hk_Num2_Pressed = false;
	bool hk_Num3_Pressed = false;

	bool hk_Numpad2_Pressed = false;
	bool hk_Numpad4_Pressed = false;
	bool hk_Numpad6_Pressed = false;
	bool hk_Numpad8_Pressed = false;

	bool hk_NumpadPlus_Pressed = false;
	


	short hk_Enter;

	short hk_Num1;
	short hk_Num2;
	short hk_Num3;

	short hk_Numpad2;
	short hk_Numpad4;
	short hk_Numpad6;
	short hk_Numpad8;

	short hk_NumpadPlus;


	while (bRunning)
	{
		HWND hforegroundWnd = GetForegroundWindow();
		HWND hDarkSouls = FindWindow(NULL, L"DARK SOULS");

		if ((hforegroundWnd == hDarkSouls) || (hDarkSouls == NULL))  // if the foreground window is "Dark Souls II" or fore whatever reason the window can't be found
		{
			
			hk_Enter = GetKeyState(0x0D);
			
			hk_Num1 = GetKeyState(0x31);
			hk_Num2 = GetKeyState(0x32);
			hk_Num3 = GetKeyState(0x33);

			hk_Numpad2 = GetKeyState(0x62);
			hk_Numpad4 = GetKeyState(0x64);
			hk_Numpad6 = GetKeyState(0x66);
			hk_Numpad8 = GetKeyState(0x68);

			hk_NumpadPlus = GetKeyState(0x6B);
			


			if (hk_Enter & 0x8000)
			{
				if (hk_Enter_Pressed == false)
				{
					hk_Enter_Pressed = true;

				}
			}
			else
			{
				hk_Enter_Pressed = false;
			}


			if (hk_Num1 & 0x8000) 
			{
				hk_Num1_Pressed = true;
				bRunning = false;
			}
			


			if (hk_Num2 & 0x8000) 
			{
				if (hk_Num2_Pressed == false) 
				{
					hk_Num2_Pressed = true;
					wireframe = !wireframe;

				}
					
			}
			else
			{
				hk_Num2_Pressed = false;
			}




			if (hk_Num3 & 0x8000) 
			{
				if (hk_Num3_Pressed == false)
				{
					hk_Num3_Pressed = true;

				}
			}
			else
			{
				hk_Num3_Pressed = false;
			}
				


			if (hk_Numpad2 & 0x8000) 
			{
				if (hk_Numpad2_Pressed == false)
				{
					hk_Numpad2_Pressed = true;
				}
			}
			else
			{
				hk_Numpad2_Pressed = false;
			}



			if (hk_Numpad4 & 0x8000) 
			{
				if (hk_Numpad4_Pressed == false) 
				{
					hk_Numpad4_Pressed = true;
				}
			}
			else
			{
				hk_Numpad4_Pressed = false;
			}


			if (hk_Numpad6 & 0x8000) 
			{
				if (hk_Numpad6_Pressed == false) 
				{
					hk_Numpad6_Pressed = true;
				}
			}
			else
			{
				hk_Numpad6_Pressed = false;
			}


			if (hk_Numpad8 & 0x8000) 
			{
				if (hk_Numpad8_Pressed == false) 
				{
					hk_Numpad8_Pressed = true;
				}
			}
			else
			{
				hk_Numpad8_Pressed = false;
			}


			if (hk_NumpadPlus & 0x8000)
			{
				if (hk_NumpadPlus_Pressed == false)
				{
					hk_NumpadPlus_Pressed = true;

				}
			}
			else
			{
				hk_NumpadPlus_Pressed = false;
			}


		}
		Sleep(30);
	}
}



