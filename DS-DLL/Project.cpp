#include "Project.h"
#include "DllMain.h"
#include "Test.h"
#include "Unloader.h"
#include "Console.h"

#include <atlstr.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <fstream>
#include <iomanip>
#include <Minhook.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <vector>
#include <wchar.h>
#include <WinBase.h>
#include <Windows.h>
#include <windowsx.h>


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
LPDIRECT3DDEVICE9 pD3dDevice;
D3DVIEWPORT9 d3dViewport;
LPD3DXFONT gFont;
LPD3DXFONT giFont;

HANDLE hHotkeyThread;
HRESULT GenerateTexture(IDirect3DDevice9 *pD3Ddev, IDirect3DTexture9 **ppD3Dtex, DWORD colour32);
bool InsertHook(void *pTarget, void *pDetour, void *pOriginal);
bool CreateDefaultFont();
void drawStuff();

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
#define C_VIOLET D3DCOLOR_RGBA(0xBF, 0x5F, 0xFF, 0xFF)
#define C_INDIGO D3DCOLOR_RGBA(0x4B, 0, 0x82, 0xFF)
#define C_RED D3DCOLOR_RGBA(0xFF, 0, 0, 0xFF)
#define C_PURPLE D3DCOLOR_RGBA(0x82, 0x0B, 0xBB, 0xFF)


int fontSize = FONT_SIZE_1080;
int fontLineSpacing = FONT_SPACING_1080;
bool bRunning = false;
bool gVisible = true;

bool wireframe = false;
bool debugEXE = false;
bool showstats = false;
wstring lastAtk[5] = { L"x",L"x",L"x",L"x",L"x" };
wstring lastDef[5] = { L"x",L"x",L"x",L"x",L"x" };
float lastDmg[5] = { 0, 0, 0, 0, 0 };
UINT *lastenemy = 0;
byte ver = 0x3e;   //0x2e

UINT *chardata1 = NULL;

ofstream fout("c:\\temp\\DLLout.txt");

//In-Game Function Hooks
typedef void(__stdcall *tDS_LandHit)(UINT *atkChar, float *dmg, UINT unk);

void __stdcall hDS_LandHit(UINT *atkChar, float *dmg, UINT unk);

tDS_LandHit oDS_LandHit = NULL;

/*

type definitions:
https://github.com/rlabrecque/Steamworks.NET-Example/tree/master/Assets/Plugins/Steamworks.NET/types/SteamTypes

Do these right later
*/



//Steam Matchmaking Hooks
typedef UINT SteamAPICall_t;
typedef UINT64 CSteamID;
typedef void(__thiscall *tAddRequestLobbyListCompatibleMembersFilter)(void*, UINT64 steamIDLobby);
typedef void(__thiscall *tAddRequestLobbyListDistanceFilter)(void*, UINT eLobbyDistanceFilter);
typedef void(__thiscall *tAddRequestLobbyListFilterSlotsAvailable)(void*, int nSlotsAvailable);
typedef void(__thiscall *tAddRequestLobbyListNearValueFilter)(void*, char *pchKeyToMatch, int nValueToBeCloseTo);
typedef void(__thiscall *tAddRequestLobbyListNumericalFilter)(void*, char *pchKeyToMatch, int nValueToMatch, UINT eComparisonType);
typedef void(__thiscall *tAddRequestLobbyListResultCountFilter)(void*, int cMaxResults);
typedef void(__thiscall *tAddRequestLobbyListStringFilter)(void*, char *pchKeyToMatch, char *pchValueToMatch, UINT eComparisonType);
typedef SteamAPICall_t(__thiscall *tCreateLobby)(void*, UINT eLobbyType, int cMaxMembers);
typedef UINT64(__thiscall *tGetLobbyByIndex)(void*, UINT *unknown, int iLobby);
typedef int(__thiscall *tGetLobbyChatEntry)(void*, int iChatID, UINT64 *pSteamIDUser, void *pvData, int cubData, UINT *peChatEntryType);
typedef char*(__thiscall *tGetLobbyData)(void*, UINT64 steamIDLobby, char *pchKey);
typedef bool(__thiscall *tGetLobbyDataByIndex)(void*, UINT64 steamIDLobby, int iLobbyData, char *pchKey, int cchKeyBufferSize, char *pchValue, int cchValueBufferSize);
typedef char*(__thiscall *tGetLobbyMemberData)(void*, UINT64 steamIDLobby, UINT64 steamIDUser, char *pchKey);
typedef UINT64(__thiscall *tGetLobbyMemberByIndex)(void*, UINT steamIDLobby, int iMember);
typedef int(__thiscall *tGetNumLobbyMembers)(void*, UINT64 steamIDLobby);
typedef void(__thiscall *tInviteUserToLobby)(void*, UINT64 steamIDLobby, UINT64 steamIDInvitee);
typedef void(__thiscall *tJoinLobby)(void*, UINT64 steamIDLobby);
typedef void(__thiscall *tLeaveLobby)(void*, UINT64 steamIDLobby);
typedef void(__thiscall *tRequestLobbyList)(void*);
typedef void(__thiscall *tSendLobbyChatMsg)(void*, UINT64 steamIDLobby, void *pvMsgBody, int cubMsgBody);
typedef bool(__thiscall *tSetLobbyData)(void*, UINT64 steamIDLoby, char *pchKey, char *pchValue);
typedef void(__thiscall *tSetLobbyMemberData)(void*, UINT64 steamIDLobby, char *pchKey, char *pchValue);

void __fastcall hAddRequestLobbyListCompatibleMembersFilter(void *This, void *notUsed, UINT64 steamIDLobby);
void __fastcall hAddRequestLobbyListDistanceFilter(void *This, void *notUsed, UINT eLobbyDistanceFilter);
void __fastcall hAddRequestLobbyListFilterSlotsAvailable(void *This, void *notUsed, int nSlotsAvailable);
void __fastcall hAddRequestLobbyListNearValueFilter(void *This, void *notUsed, char *pchKeyToMatch, int nValueToBeCloseTo);
void __fastcall hAddRequestLobbyListNumericalFilter(void *This, void *notUsed, char *pchKeyToMatch, int nValueToMatch, UINT eComparisonType);
void __fastcall hAddRequestLobbyListResultCountFilter(void* This, void *notUsed, int cMaxResults);
void __fastcall hAddRequestLobbyListStringFilter(void *This, void *notUsed, char *pchKeyToMatch, char *pchValueToMatch, UINT eComparisonType);
SteamAPICall_t __fastcall hCreateLobby(void *This, void *notUsed, UINT eLobbyType, int cMaxMembers);
UINT64 __fastcall hGetLobbyByIndex(void *This, void *notUsed, UINT *unknown, int iLobby);
int __fastcall hGetLobbyChatEntry(void *This, void *notUsed, int iChatID, UINT64 *pSteamIDUser, void *pvData, int cubData, UINT *peChatEntryType);
char* __fastcall hGetLobbyData(void *This, void *notUsed, UINT64 steamIDLobby, char *pchKey);
bool __fastcall hGetLobbyDataByIndex(void *This, void *notUsed, UINT64 steamIDLobby, int iLobbyData, char *pchKey, int cchKeyBufferSize, char *pchValue, int cchValueBufferSize);
UINT64 __fastcall hGetLobbyMemberByIndex(void* This, void *notUsed, UINT steamIDLobby, int iMember);
char* __fastcall hGetLobbyMemberData(void *This, void *notUsed, UINT64 steamIDLobby, UINT64 steamIDUser, char *pchKey);
int __fastcall hGetNumLobbyMembers(void *This, void *notUsed, UINT64 steamIDLobby);
void __fastcall hInviteUserToLobby(void *This, void *notUsed, UINT64 steamIDLobby, UINT64 steamIDInvitee);
void __fastcall hJoinLobby(void *This, void *notUsed, UINT64 steamIDLobby);
void __fastcall hLeaveLobby(void *This, void *notUsed, UINT64 steamIDLobby);
void __fastcall hRequestLobbyList(void *This, void *notUsed);
void __fastcall hSendLobbyChatMsg(void *This, void *notUsed, UINT64 steamIDLobby, void *pvMsgBody, int cubMsgBody);
bool __fastcall hSetLobbyData(void *This, void *notUsed, UINT64 steamIDLobby, char *pchKey, char *pchValue);
void __fastcall hSetLobbyMemberData(void *This, void *notUsed, UINT64 steamIDLobby, char *pchKey, char *pchValue);

tAddRequestLobbyListCompatibleMembersFilter oAddRequestLobbyListCompatibleMembersFilter = NULL;
tAddRequestLobbyListDistanceFilter oAddRequestLobbyListDistanceFilter = NULL;
tAddRequestLobbyListFilterSlotsAvailable oAddRequestLobbyListFilterSlotsAvailable = NULL;
tAddRequestLobbyListNearValueFilter oAddRequestLobbyListNearValueFilter = NULL;
tAddRequestLobbyListNumericalFilter oAddRequestLobbyListNumericalFilter = NULL;
tAddRequestLobbyListResultCountFilter oAddRequestLobbyListResultCountFilter = NULL;
tAddRequestLobbyListStringFilter oAddRequestLobbyListStringFilter = NULL;
tCreateLobby oCreateLobby = NULL;
tGetLobbyByIndex oGetLobbyByIndex = NULL;
tGetLobbyChatEntry oGetLobbyChatEntry = NULL;
tGetLobbyData oGetLobbyData = NULL;
tGetLobbyDataByIndex oGetLobbyDataByIndex = NULL;
tGetLobbyMemberByIndex oGetLobbyMemberByIndex = NULL;
tGetLobbyMemberData oGetLobbyMemberData = NULL;
tGetNumLobbyMembers oGetNumLobbyMembers = NULL;
tInviteUserToLobby oInviteUserToLobby = NULL;
tJoinLobby oJoinLobby = NULL;
tLeaveLobby oLeaveLobby = NULL;
tRequestLobbyList oRequestLobbyList = NULL;
tSendLobbyChatMsg oSendLobbyChatMsg = NULL;
tSetLobbyData oSetLobbyData = NULL;
tSetLobbyMemberData oSetLobbyMemberData = NULL;


//Steam Networking Hooks
typedef void (__thiscall *tReadP2PPacket)(void*, void *pubDest, UINT cubDest, UINT *pcubMsgSize, UINT64 *SteamID, UINT nChannel);
typedef void (__thiscall *tSendP2PPacket)(void*, UINT64 SteamID, void *pubData, UINT cubData, UINT eP2PSendType, UINT nChannel);

void __fastcall hReadP2PPacket(void *This, void *unused, void *pubDest, UINT cubDest, UINT *pcubMsgSize, UINT64 *SteamID, UINT nChannel);
void __fastcall hSendP2PPacket(void *This, void *unused, UINT64 SteamID, void *pubData, UINT cubData, UINT eP2PSendType, UINT nChannel);

tReadP2PPacket oReadP2PPacket = NULL;
tSendP2PPacket oSendP2PPacket = NULL;


//D3D9 Hooks
typedef HRESULT(WINAPI* tBeginScene)(LPDIRECT3DDEVICE9 pDevice);
typedef HRESULT(WINAPI* tColorFill)(LPDIRECT3DDEVICE9 pDevice, IDirect3DSurface9 *pSurface, RECT *pRect, D3DCOLOR color);
typedef HRESULT(WINAPI* tCreateIndexBuffer)(LPDIRECT3DDEVICE9 pDevice, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9 **ppIndexBuffer, HANDLE *pSharedHandle);
typedef HRESULT(WINAPI* tCreateRenderTarget)(LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle);
typedef HRESULT(WINAPI* tCreateTexture)(LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9 **ppTexture, HANDLE *pSharedHAndle);
typedef HRESULT(WINAPI* tCreateVertexBuffer)(LPDIRECT3DDEVICE9 pDevice, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle);
typedef HRESULT(WINAPI* tCreateVertexShader)(LPDIRECT3DDEVICE9 pDevice, DWORD *pFunction, IDirect3DVertexShader9 **ppShader);
typedef HRESULT(WINAPI* tDrawIndexedPrimitive)(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);
typedef HRESULT(WINAPI* tDrawIndexedPrimitiveUP)(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, void *pIndexData, D3DFORMAT IndexDataFormat, void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
typedef HRESULT(WINAPI* tDrawPrimitive)(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE pPrimitiveType, UINT startVertex, UINT primitiveCount);
typedef HRESULT(WINAPI* tDrawPrimitiveUP)(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE pPrimitiveType, UINT primitiveCount, void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
typedef HRESULT(WINAPI* tEndScene)(LPDIRECT3DDEVICE9 pDevice);
typedef HRESULT(WINAPI* tGetLight)(LPDIRECT3DDEVICE9 pDevice, DWORD Index, D3DLIGHT9 *pLight);
typedef HRESULT(WINAPI* tGetStreamSource)(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9 **ppStreamData, UINT *pOffsetInBytes, UINT *pStride);
typedef HRESULT(WINAPI* tLightEnable)(LPDIRECT3DDEVICE9 pDevice, DWORD LightIndex, BOOL bEnable);
typedef HRESULT(WINAPI* tProcessVertices)(LPDIRECT3DDEVICE9 pDevice, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9 *pDestBuffer, IDirect3DVertexDeclaration9 *pVertexDecl, DWORD Flags);
typedef HRESULT(WINAPI* tReset)(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
typedef HRESULT(WINAPI* tSetLight)(LPDIRECT3DDEVICE9 pDevice, DWORD Index, D3DLIGHT9 *pLight);
typedef HRESULT(WINAPI* tSetMaterial)(LPDIRECT3DDEVICE9 pDevice, D3DMATERIAL9 *pMaterial);
typedef HRESULT(WINAPI* tSetRenderState)(LPDIRECT3DDEVICE9 pDevice, D3DRENDERSTATETYPE pState, DWORD value);
typedef HRESULT(WINAPI* tSetStreamSource)(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes, UINT Stride);
typedef HRESULT(WINAPI* tSetTexture)(LPDIRECT3DDEVICE9 pDevice, DWORD Sampler, IDirect3DBaseTexture9 *pTexture);
typedef HRESULT(WINAPI* tSetViewport)(LPDIRECT3DDEVICE9 pDevice, D3DVIEWPORT9 *pViewport);

HRESULT WINAPI hBeginScene(LPDIRECT3DDEVICE9 pDevice);
HRESULT WINAPI hColorFill(LPDIRECT3DDEVICE9 pDevice, IDirect3DSurface9 *pSurface, RECT *pRect, D3DCOLOR color);
HRESULT WINAPI hCreateIndexBuffer(LPDIRECT3DDEVICE9 pDevice, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9 **ppIndexBuffer, HANDLE *pSharedHandle);
HRESULT WINAPI hCreateRenderTarget(LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle);
HRESULT WINAPI hCreateTexture(LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9 **ppTexture, HANDLE *pSharedHAndle);
HRESULT WINAPI hCreateVertexBuffer(LPDIRECT3DDEVICE9 pDevice, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle);
HRESULT WINAPI hCreateVertexShader(LPDIRECT3DDEVICE9 pDevice, DWORD *pFunction, IDirect3DVertexShader9 **ppShader);
HRESULT WINAPI hDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount);
HRESULT WINAPI hDrawIndexedPrimitiveUP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, void *pIndexData, D3DFORMAT IndexDataFormat, void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
HRESULT WINAPI hDrawPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE pPrimativeType, UINT startVertex, UINT primitiveCount);
HRESULT WINAPI hDrawPrimitiveUP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE pPrimitiveType, UINT primitiveCount, void *pVertexStreamZeroData, UINT VertexStreamZeroStride);
HRESULT WINAPI hEndScene(LPDIRECT3DDEVICE9 pDevice);
HRESULT WINAPI hGetLight(LPDIRECT3DDEVICE9 pDevice, DWORD Index, D3DLIGHT9 *pLight);
HRESULT WINAPI hGetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9 **ppStreamData, UINT *pOffsetInBytes, UINT *pStride);
HRESULT WINAPI hLightEnable(LPDIRECT3DDEVICE9 pDevice, DWORD LightIndex, BOOL bEnable);
HRESULT WINAPI hProcessVertices(LPDIRECT3DDEVICE9 pDevice, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9 *pDestBuffer, IDirect3DVertexDeclaration9 *pVertexDecl, DWORD Flags);
HRESULT WINAPI hReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters);
HRESULT WINAPI hSetLight(LPDIRECT3DDEVICE9 pDevice, DWORD Index, D3DLIGHT9 *pLight);
HRESULT WINAPI hSetMaterial(LPDIRECT3DDEVICE9 pDevice, D3DMATERIAL9 *pMaterial);
HRESULT WINAPI hSetRenderState(LPDIRECT3DDEVICE9 pDevice, D3DRENDERSTATETYPE pState, DWORD value);
HRESULT WINAPI hSetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes, UINT Stride);
HRESULT WINAPI hSetTexture(LPDIRECT3DDEVICE9 pDevice, DWORD Sampler, IDirect3DBaseTexture9 *pTexture);
HRESULT WINAPI hSetViewport(LPDIRECT3DDEVICE9 pDevice, D3DVIEWPORT9 *pViewport);

tBeginScene oBeginScene = NULL;
tColorFill oColorFill = NULL;
tCreateIndexBuffer oCreateIndexBuffer = NULL;
tCreateRenderTarget oCreateRenderTarget = NULL;
tCreateTexture oCreateTexture = NULL;
tCreateVertexBuffer oCreateVertexBuffer = NULL;
tCreateVertexShader oCreateVertexShader = NULL;
tDrawIndexedPrimitive oDrawIndexedPrimitive = NULL;
tDrawIndexedPrimitiveUP oDrawIndexedPrimitiveUP = NULL;
tDrawPrimitive oDrawPrimitive = NULL;
tDrawPrimitiveUP oDrawPrimitiveUP = NULL;
tEndScene oEndScene = NULL;
tGetLight oGetLight = NULL;
tGetStreamSource oGetStreamSource = NULL;
tLightEnable oLightEnable = NULL;
tProcessVertices oProcessVertices = NULL;
tReset oReset = NULL;
tSetLight oSetLight = NULL;
tSetMaterial oSetMaterial = NULL;
tSetRenderState oSetRenderState = NULL;
tSetStreamSource oSetStreamSource = NULL;
tSetTexture oSetTexture = NULL;
tSetViewport oSetViewport = NULL;

struct sDSGameFunctions
{
	DWORD DS_LandHitAddress;
};
sDSGameFunctions DSGameFunctions;


struct sSteamMatchmakingFunctions
{
	DWORD AddRequestLobbyListCompatibleMembersFilterAddress;
	DWORD AddRequestLobbyListDistanceFilterAddress;
	DWORD AddRequestLobbyListFilterSlotsAvailableAddress;
	DWORD AddRequestLobbyListNearValueFilterAddress;
	DWORD AddRequestLobbyListNumericalFilterAddress;
	DWORD AddRequestLobbyListResultCountFilterAddress;
	DWORD AddRequestLobbyListStringFilterAddress;
	DWORD CreateLobbyAddress;
	DWORD GetLobbyByIndexAddress;
	DWORD GetLobbyChatEntryAddress;
	DWORD GetLobbyDataAddress;
	DWORD GetLobbyDataByIndexAddress;
	DWORD GetLobbyMemberByIndexAddress;
	DWORD GetLobbyMemberDataAddress;
	DWORD GetNumLobbyMembersAddress;
	DWORD InviteUserToLobbyAddress;
	DWORD JoinLobbyAddress;
	DWORD LeaveLobbyAddress;
	DWORD RequestLobbyListAddress;
	DWORD SendLobbyChatMsgAddress;
	DWORD SetLobbyDataAddress;
	DWORD SetLobbyMemberDataAddress;
};
sSteamMatchmakingFunctions SteamMatchMakingFunctions;
enum SteamMatchmakingVTable
{
	GetFavoriteGameCount, // 0
	GetFavoriteGame, // 1
	AddFavoriteGame, // 2
	RemoveFavoriteGame, // 3
	RequestLobbyList, // 4
	AddRequestLobbyListStringFilter, // 5
	AddRequestLobbyListNumericalFilter, // 6
	AddRequestLobbyListNearValueFilter, // 7
	AddRequestLobbyListFilterSlotsAvailable, // 8
	AddRequestLobbyListDistanceFilter, // 9
	AddRequestLobbyListResultCountFilter, // 10
	AddRequestLobbyListCompatibleMembersFilter, // 11
	GetLobbyByIndex, // 12
	CreateLobby, // 13
	JoinLobby, // 14
	LeaveLobby, // 15
	InviteUserToLobby, // 16
	GetNumLobbyMembers, // 17
	GetLobbyMemberByIndex, // 18
	GetLobbyData, // 19
	SetLobbyData, // 20
	GetLobbyMemberData, // 21
	SetLobbyMemberData, // 22
	GetLobbyDataCount, // 23
	GetLobbyDataByIndex, // 24
	DeleteLobbyData, // 25
	SendLobbyChatMsg, // 26
	GetLobbyChatEntry, // 27
	RequestLobbyData, // 28
	SetLobbyGameServer, // 29
	GetLobbyGameServer, // 30
	SetLobbyMemberLimit, // 31
	GetLobbyMemberLimit, // 32
	SetLobbyVoiceEnabled, // 33
	RequestFriendsLobbies, // 34
	SetLobbyType, // 35
	SetLobbyJoinable, // 36
	GetLobbyOwner, // 37
	SetLobbyOwner, // 38
	SetLinkedLobby, // 39
	BeginGMSQuery, // 40
	PollGMSQuery, // 41
	GetGMSQueryResults, // 42
	ReleaseGMSQuery, // 43
	SendGameServerPingSample, // 44
	EnsureFavoriteGameAccountsUpdated // 45
};

struct sSteamNetworkingFunctions
{
	DWORD SendP2PPacketAddress;
	DWORD IsP2PPacketAvailableAddress;
	DWORD ReadP2PPacketAddress;
};
sSteamNetworkingFunctions SteamNetworkingFunctions;
enum SteamNetworkingVTable
{
	SendP2PPacket, // 0
	IsP2PPacketAvailable, // 1
	ReadP2PPacket, // 2
	AcceptP2PSessionWithUser, // 3
	CloseP2PSessionWithUser, // 4
	CloseP2PChannelWithUser, // 5
	GetP2PSessionState, // 6
	AllowP2PPacketRelay, // 7
	CreateListenSocket, // 8
	CreateP2PConnectionSocket, // 9 
	CreateConnectionSocket, // 10
	DestroySocket, // 11
	DestroyListenSocket, // 12
	SendDataOnSocket, // 13
	IsDataAvailableOnSocket, // 14
	RetrieveDataFromSocket, // 15
	IsDataAvailable, // 16
	RetrieveData, // 17
	GetSocketInfo, // 18
	GetListenSocketInfo, // 19
	GetSocketConnectionType, // 20
	GetMaxPacketSize // 21
};

struct sDXFunctions
{
	DWORD BeginSceneAddress;
	DWORD ColorFillAddress;
	DWORD CreateIndexBufferAddress;
	DWORD CreateRenderTargetAddress;
	DWORD CreateTextureAddress;
	DWORD CreateVertexBufferAddress;
	DWORD CreateVertexShaderAddress;
	DWORD DrawIndexedPrimitiveAddress;
	DWORD DrawIndexedPrimitiveUPAddress;
	DWORD DrawPrimitiveAddress;
	DWORD DrawPrimitiveUPAddress;
	DWORD EndSceneAddress;
	DWORD GetLightAddress;
	DWORD GetStreamSourceAddress;
	DWORD LightEnableAddress;
	DWORD ProcessVerticesAddress;
	DWORD ResetAddress;
	DWORD SetLightAddress;
	DWORD SetMaterialAddress;
	DWORD SetRenderStateAddress;
	DWORD SetStreamSourceAddress;
	DWORD SetTextureAddress;
	DWORD SetViewportAddress;
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

struct PacketData
{
	byte bytes[512];
};
struct charPos
{
	unsigned int unk1;
	float facing;				//0x0004
	BYTE unk2[0x8];
	float xPos;					//0x0010
	float yPos;					//0x0014
	float zPos;					//0x0018
};
struct charLocData
{
	BYTE unk1[0x1C];
	charPos *pos;				//0x001C
};
struct creatureData2
{
	
};
struct creatureData1
{
	BYTE unk1[0x28];
	charLocData *loc;			//0x0028
	BYTE unk2[0xC];
	wchar_t modelName[0x8];		//0x0038
	BYTE unk3[0x20];
	int NPCID;					//0x0068
	BYTE unk4[0x4];
	int PhantomType;			//0x0070
	int TeamType;				//0x0074
	BYTE unk5[0x148];
	float currPoise;			//0x01C0
	float maxPoise;				//0x01C4
	BYTE unk5a[0x4];
	float poiseRecoveryTimer;	//0x01CC
	BYTE unk6[0x104];
	int currHP;					//0x02D4
	int maxHP;					//0x02D8
	BYTE unk7[0x8];
	int currStam;				//0x02E4
	int maxStam;				//0x02E8
	BYTE unk8[0x14];
	int currentPoisonResist;	//0x0300
	int currentToxicResist;		//0x0304
	int currentBleedResist;		//0x0308
	int currentCurseResist;		//0x030C
	int maxPoisonResist;		//0x0310
	int maxToxicResist;			//0x0314
	int maxBleedResist;			//0x0318
	int maxCurseResist;			//0x031C
	BYTE unk9[0xF4];
	creatureData2 *cdata2;		//0x0414
	BYTE unk10[0x4];
	int AIID;					//0x041C
};



struct LoadedCreatures
{
	PVOID pUnknown;             // 0x137DC70
	creatureData1 *firstCreature;    // +4
	creatureData1 *lastCreature;     // +8
};




//In-Game Function Hooks
void __stdcall hDS_LandHit(UINT *atkChar, float *dmg, UINT unk)
{
	UINT *defChar = NULL;

	__asm {
		mov defChar, eax
		push eax
	}


	if (!(defChar == lastenemy) && !(defChar == chardata1) && defChar)
		lastenemy = defChar;
	
	
	creatureData1 *atk = (creatureData1*)atkChar;
	creatureData1 *def = (creatureData1*)defChar;


	for (int i = 0; i < 4; i++)
	{
		lastAtk[i] = lastAtk[i + 1];
		lastDef[i] = lastDef[i + 1];
		lastDmg[i] = lastDmg[i + 1];
	}
	

	if (atkChar && defChar)
	{
		lastAtk[4] = *(wstring*)&atk->modelName;
		lastDef[4] = *(wstring*)&def->modelName;
		lastDmg[4] = (float)(*dmg);
	}


	__asm {
		pop eax
	}
	oDS_LandHit(atkChar, dmg, unk);
}


//Steam Matchmaking Hooks
void __fastcall hAddRequestLobbyListCompatibleMembersFilter(void *This, void *notUsed, UINT64 steamIDLobby)
{
	printf("hAddRequestLobbyListCompatibleMembersFilter called - steamIDLobby:  %llx\n", steamIDLobby);

	oAddRequestLobbyListCompatibleMembersFilter(This, steamIDLobby);
}
void __fastcall hAddRequestLobbyListDistanceFilter(void *This, void *notUsed, UINT eLobbyDistanceFilter)
{
	printf("hAddRequestLobbyListDistanceFilter called - eLobbyDistanceFilter:  %d\n", eLobbyDistanceFilter);

	oAddRequestLobbyListDistanceFilter(This, eLobbyDistanceFilter);
}
void __fastcall hAddRequestLobbyListFilterSlotsAvailable(void *This, void *notUsed, int nSlotsAvailable)
{
	printf("hAddRequestLobbyListFilterSlotsAvailable called - nSlotsAvailable: %d\n", nSlotsAvailable);

	oAddRequestLobbyListFilterSlotsAvailable(This, nSlotsAvailable);
}
void __fastcall hAddRequestLobbyListNearValueFilter(void *This, void *notUsed, char *pchKeyToMatch, int nValueToBeCloseTo)
{
	printf("hAddRequestLobbyListNearValueFilter called - Key, Value:  '%s' '%s'.\n", pchKeyToMatch, nValueToBeCloseTo);

	oAddRequestLobbyListNearValueFilter(This, pchKeyToMatch, nValueToBeCloseTo);
}
void __fastcall hAddRequestLobbyListNumericalFilter(void* This, void *notUsed, char *pchKeyToMatch, int nValueToMatch, UINT eComparisonType)
{
	printf("hAddRequestLobbyListNumericalFilter called - Key, Value:  '%s' '%s'.\n", pchKeyToMatch,nValueToMatch);

	oAddRequestLobbyListNumericalFilter(This, pchKeyToMatch, nValueToMatch, eComparisonType);
}
void __fastcall hAddRequestLobbyListResultCountFilter(void* This, void *notUsed, int cMaxResults)
{
	printf("hAddRequestLobbyListResultCountFilter called - cMaxResults:  %d\n", cMaxResults);

	oAddRequestLobbyListResultCountFilter(This, cMaxResults);
}
void __fastcall hAddRequestLobbyListStringFilter(void* This, void *notUsed, char *pchKeyToMatch, char *pchValueToMatch, UINT eComparisonType)
{
	printf("hAddRequestLobbyListStringFilter called - Key, Value:  '%s' '%s'.\n", pchKeyToMatch, pchValueToMatch);

	oAddRequestLobbyListStringFilter(This, pchKeyToMatch, pchValueToMatch, eComparisonType);
}
SteamAPICall_t __fastcall hCreateLobby(void *This, void *notUsed, UINT eLobbyType, int cMaxMembers)
{
	SteamAPICall_t tmp;
	tmp = oCreateLobby(This, eLobbyType, cMaxMembers);

	printf("hCreateLobby called.  LobbyType: %d, cMaxMembers, SteamAPICall_t: %d\n", eLobbyType, cMaxMembers, tmp);

	return tmp;
}
UINT64 __fastcall hGetLobbyByIndex(void* This, void* notUsed, UINT* unknown, int iLobby)
{
	//Not sure this one's returning properly
	//printf("hGetLobbyByIndex called.\n");

	UINT64 tmp = 0;
	tmp = oGetLobbyByIndex(This, unknown, iLobby);

	printf("hGetLobbyByIndex called - steamIDLobby: %llx\n", tmp);

	return tmp;
}
int __fastcall hGetLobbyChatEntry(void *This, void *notUsed, int iChatID, UINT64 *pSteamIDUser, void *pvData, int cubData, UINT *peChatEntryType)
{
	printf("hGetLobbyChatEntry called.\n");

	int tmp = 0;
	tmp = oGetLobbyChatEntry(This, iChatID, pSteamIDUser, pvData, cubData, peChatEntryType);
	return tmp;
}
char* __fastcall hGetLobbyData(void *This, void *notUsed, UINT64 steamIDLobby, char *pchKey)
{
	char *tmp;
	tmp = oGetLobbyData(This, steamIDLobby, pchKey);

	printf("hGetLobbyData called - steamIDLobby, Key, Value:  %llx, %s, %s\n", steamIDLobby, pchKey, tmp);
	return tmp;
}
bool __fastcall hGetLobbyDataByIndex(void *This, void *notUsed, UINT64 steamIDLobby, int iLobbyData, char *pchKey, int cchKeyBufferSize, char *pchValue, int cchValueBufferSize)
{
	printf("hGetLobbyDataByIndex called - Key: %s, Value: %s.\n", pchKey, pchValue);

	bool tmp = false;
	tmp = oGetLobbyDataByIndex(This, steamIDLobby, iLobbyData, pchKey, cchKeyBufferSize, pchValue, cchValueBufferSize);
	return tmp;
}
UINT64 __fastcall hGetLobbyMemberByIndex(void *This, void *notUsed, UINT steamIDLobby, int iMember)
{
	UINT64 tmp = 0;
	tmp = oGetLobbyMemberByIndex(This, steamIDLobby, iMember);

	printf("hGetLobbyMemberByIndex called - iMember:  %llx.\n", tmp);

	return tmp;
}
char* __fastcall hGetLobbyMemberData(void *This, void *notUsed, UINT64 steamIDLobby, UINT64 steamIDUser, char *pchKey)
{
	
	char *tmp;
	tmp = oGetLobbyMemberData(This, steamIDLobby, steamIDUser, pchKey);

	printf("hGetLobbyMemberData called - steamIDLobby, steamIDUser, pchKey:  %llx, %llx, %p\n", steamIDLobby, steamIDUser, pchKey);

	
	return tmp;
}
int __fastcall hGetNumLobbyMembers(void *This, void *notUsed, UINT64 steamIDLobby)
{
	int tmp = 0;
	tmp = oGetNumLobbyMembers(This, steamIDLobby);

	printf("hGetNumLobbyMembers called - steamIDLobby, Members:  %llx, %d\n", steamIDLobby, tmp);

	return tmp;
}
void __fastcall hInviteUserToLobby(void *This, void *notUsed, UINT64 steamIDLobby, UINT64 steamIDInvitee)
{
	printf("hInviteUserToLobby called - steamIDLobby, steamIDInvitee:  %llx, %llx\n", steamIDLobby, steamIDInvitee);

	oInviteUserToLobby(This, steamIDLobby, steamIDInvitee);
}
void __fastcall hJoinLobby(void *This, void *notUsed, UINT64 steamIDLobby)
{
	printf("hJoinLobby called.  steamIDLobby:  %llx\n", steamIDLobby);

	oJoinLobby(This, steamIDLobby);
}
void __fastcall hLeaveLobby(void* This, void *notUsed, UINT64 steamIDLobby)
{
	printf("hLeaveLobby called.  steamIDLobby:  %llx\n", steamIDLobby);

	oLeaveLobby(This, steamIDLobby);
}
void __fastcall hRequestLobbyList(void *This, void *notUsed)
{
	printf("hRequestLobbyList called.\n");
		
	oRequestLobbyList(This);
}
void __fastcall hSendLobbyChatMsg(void *This, void *notUsed, UINT64 steamIDLobby, void *pvMsgBody, int cubMsgBody)
{
	printf("hSendLobbyChatMsg called - Lobby: %llx, Msg: %s\n", steamIDLobby, pvMsgBody);

	oSendLobbyChatMsg(This, steamIDLobby, pvMsgBody, cubMsgBody);
}
bool __fastcall hSetLobbyData(void *This, void *notUsed, UINT64 steamIDLobby, char *pchKey, char *pchValue)
{
	bool tmp;
	tmp = oSetLobbyData(This, steamIDLobby, pchKey, pchValue);

	printf("hSetLobbyData called - steamIDLobby, pchKey, pchValue, return:  %llx, %s, %s, %d\n", steamIDLobby, pchKey, pchValue, tmp);

	return tmp;
}
void __fastcall hSetLobbyMemberData(void *This, void *notUsed, UINT64 steamIDLobby, char *pchKey, char *pchValue)
{
	printf("hSetLobbyMemberData called - steamIDLobby, pchKey, pchValue:  %llx, %s, %s\n", steamIDLobby, pchKey, pchValue);

	oSetLobbyMemberData(This, steamIDLobby, pchKey, pchValue);
}



//Steam Networking Hooks
void __fastcall hReadP2PPacket(void *This, void *unused, void *pubDest, UINT cubDest, UINT *pcubMsgSize, UINT64 *SteamID, UINT nChannel)
{
	//printf("hReadP2PPacket called.\n");


	oReadP2PPacket(This, pubDest, cubDest, pcubMsgSize, SteamID, nChannel);

	PacketData* packetData = (PacketData*)pubDest;

	SYSTEMTIME st;
	GetLocalTime(&st);
	fout << dec << setfill('0') << std::setw(2) << st.wHour << ':'
		<< std::setw(2) << st.wMinute << ':'
		<< std::setw(2) << st.wSecond << '.'
		<< std::setw(3) << st.wMilliseconds << ',';

	fout << "In ," << nChannel << ",x,";
	fout << hex << setfill('0') << setw(16) << *SteamID << ",";
	fout << dec << setfill('0') << std::setw(4) << cubDest << ",";
	for (int i = 0; i < cubDest; ++i)
		fout << hex << setw(2) << (int)packetData->bytes[i] << " ";
	fout << endl;

	//printf("In size, byte #1:  %d, %d\n", cubDest, packetData->bytes[0]);
}
void __fastcall hSendP2PPacket(void* This, void* unused, UINT64 SteamID, void *pubData, UINT cubData, UINT eP2PSendType, UINT nChannel)
{
	//printf("hSendP2PPacket called.\n");
	
	oSendP2PPacket(This, SteamID, pubData, cubData, eP2PSendType, nChannel);
	

	PacketData* packetData = (PacketData*)pubData;

	SYSTEMTIME st;
	GetLocalTime(&st);

	fout << dec << setfill('0') << std::setw(2) << st.wHour << ':'
		<< std::setw(2) << st.wMinute << ':'
		<< std::setw(2) << st.wSecond << '.'
		<< std::setw(3) << st.wMilliseconds << ',';

	fout << "Out," << nChannel << "," << eP2PSendType << ",";
	fout << hex << setfill('0') << setw(16) << SteamID << ",";
	fout << dec << setfill('0') << std::setw(4) << cubData << ",";
	for (int i = 0; i < cubData; ++i)
		fout << hex << setw(2) << (int)packetData->bytes[i] << " ";
	fout << endl;

	//printf("Out size, byte #1:  %d, %d\n", cubData, packetData->bytes[0]);
}


//D3D9 Hooks
HRESULT WINAPI hBeginScene(LPDIRECT3DDEVICE9 pDevice)
{
	//printf("BeginScene called.\n");

	HRESULT tmp;
	tmp = oBeginScene(pDevice);
	return tmp;
}
HRESULT WINAPI hColorFill(LPDIRECT3DDEVICE9 pDevice, IDirect3DSurface9 *pSurface, RECT *pRect, D3DCOLOR color)
{
	//printf("ColorFill called.\n");

	HRESULT tmp;
	tmp = oColorFill(pDevice, pSurface, pRect, color);
	return tmp;
}
HRESULT WINAPI hCreateIndexBuffer(LPDIRECT3DDEVICE9 pDevice, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9 **ppIndexBuffer, HANDLE *pSharedHandle)
{
	//printf("hCreateIndexBuffer called.\n");

	HRESULT tmp;
	tmp = oCreateIndexBuffer(pDevice, Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle);
	return tmp;
}
HRESULT WINAPI hCreateRenderTarget(LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle)
{
	//printf("CreateRenderTarget called.\n");

	HRESULT tmp;
	tmp = oCreateRenderTarget(pDevice, Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle);
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
HRESULT WINAPI hCreateVertexShader(LPDIRECT3DDEVICE9 pDevice, DWORD *pFunction, IDirect3DVertexShader9 **ppShader)
{
	//printf("hCreateVertexShader called.\n");

	HRESULT tmp;
	tmp = oCreateVertexShader(pDevice, pFunction, ppShader);
	return tmp;
}
HRESULT WINAPI hDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
	//printf("DrawIndexedPrimitive called. Type: %d, BaseVert: %d, MinIndex: %d, NumVert: %d, StartIndex: %d, PrimCount: %d\n", Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);

	HRESULT tmp;

	if (wireframe)
	{
		pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		tmp = oDrawIndexedPrimitive(pDevice, Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);
		pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	}
	else
	{
		tmp = oDrawIndexedPrimitive(pDevice, Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);
	}

	return tmp;
}
HRESULT WINAPI hDrawIndexedPrimitiveUP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, void *pIndexData, D3DFORMAT IndexDataFormat, void *pVertexStreamZeroData, UINT VertexStreamZeroStride) 
{
	//printf("DrawIndexedPrimitiveUP called.\n");
	
	HRESULT tmp;
	tmp = oDrawIndexedPrimitiveUP(pDevice, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
	return tmp;
}
HRESULT WINAPI hDrawPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE primitiveType, UINT startVertex, UINT primitiveCount)
{
	//printf("DrawPrimitive called.\n");

	HRESULT tmp;
	tmp = oDrawPrimitive(pDevice, primitiveType, startVertex, primitiveCount);
	return tmp;
}
HRESULT WINAPI hDrawPrimitiveUP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE primitiveType, UINT primitiveCount, void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	//printf("DrawPrimitiveUP called.\n");

	HRESULT tmp;
	tmp = oDrawPrimitiveUP(pDevice, primitiveType, primitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
	return tmp;
}
HRESULT WINAPI hEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	//printf("EndScene called.\n");

	if (!gVisible) return oEndScene(pDevice);

	if (!CreateDefaultFont())
	{
		printf("Problem creating default font: returning to normal EndScene\n");
		return oEndScene(pDevice);
	}


	drawStuff();

	HRESULT tmp;
	tmp = oEndScene(pDevice);
	return tmp;
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
HRESULT WINAPI hProcessVertices(LPDIRECT3DDEVICE9 pDevice, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9 *pDestBuffer, IDirect3DVertexDeclaration9 *pVertexDecl, DWORD Flags)
{
	//printf("ProcessVertices called.\n");

	HRESULT tmp;
	tmp = oProcessVertices(pDevice, SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);
	return tmp;
}
HRESULT WINAPI hReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters)
{
	printf("hReset() called.\n");

	if (gFont)
		gFont->OnLostDevice(); // Avoid device loss errors
	if (giFont)
		giFont->OnLostDevice();


	HRESULT hReturn = oReset(pDevice, pPresentationParameters);
	if (hReturn == D3D_OK && gFont)
		gFont->OnResetDevice();
	if (hReturn == D3D_OK && giFont)
		giFont->OnResetDevice();

	gFont = NULL;
	giFont = NULL;

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
HRESULT WINAPI hSetMaterial(LPDIRECT3DDEVICE9 pDevice, D3DMATERIAL9 *pMaterial)
{
	//print("hSetMaterial called.\n");

	HRESULT tmp;
	tmp = oSetMaterial(pDevice, pMaterial);
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
HRESULT WINAPI hSetViewport(LPDIRECT3DDEVICE9 pDevice, D3DVIEWPORT9 *pViewport)
{
	//printf("SetViewport called.\n");

	HRESULT tmp;
	tmp = oSetViewport(pDevice, pViewport);
	return tmp;
}





void initInGameFunctions()
{
	DSGameFunctions.DS_LandHitAddress = 0x00E6BFE0;
}
void initSteamFunctions()
{
	HMODULE steamapiHandle;
	steamapiHandle = GetModuleHandle(TEXT("steam_api.dll"));

	DWORD *SteamNetworking;
	DWORD *SteamMatchmaking;


	__asm {
		mov eax, steamapiHandle
			add eax, 0x182ac
			mov eax, [eax]
			mov eax, [eax + 4]
			mov eax, [eax]
			mov SteamMatchmaking, eax
	}
	DWORD *pVTable = (DWORD*)(SteamMatchmaking);

	SteamMatchMakingFunctions.AddRequestLobbyListCompatibleMembersFilterAddress = pVTable[SteamMatchmakingVTable::AddRequestLobbyListCompatibleMembersFilter];
	SteamMatchMakingFunctions.AddRequestLobbyListDistanceFilterAddress = pVTable[SteamMatchmakingVTable::AddRequestLobbyListDistanceFilter];
	SteamMatchMakingFunctions.AddRequestLobbyListFilterSlotsAvailableAddress = pVTable[SteamMatchmakingVTable::AddRequestLobbyListFilterSlotsAvailable];
	SteamMatchMakingFunctions.AddRequestLobbyListNearValueFilterAddress = pVTable[SteamMatchmakingVTable::AddRequestLobbyListNearValueFilter];
	SteamMatchMakingFunctions.AddRequestLobbyListNumericalFilterAddress = pVTable[SteamMatchmakingVTable::AddRequestLobbyListNumericalFilter];
	SteamMatchMakingFunctions.AddRequestLobbyListResultCountFilterAddress = pVTable[SteamMatchmakingVTable::AddRequestLobbyListResultCountFilter];
	SteamMatchMakingFunctions.AddRequestLobbyListStringFilterAddress = pVTable[SteamMatchmakingVTable::AddRequestLobbyListStringFilter];
	SteamMatchMakingFunctions.CreateLobbyAddress = pVTable[SteamMatchmakingVTable::CreateLobby];
	SteamMatchMakingFunctions.GetLobbyByIndexAddress = pVTable[SteamMatchmakingVTable::GetLobbyByIndex];
	SteamMatchMakingFunctions.GetLobbyChatEntryAddress = pVTable[SteamMatchmakingVTable::GetLobbyChatEntry];
	SteamMatchMakingFunctions.GetLobbyDataAddress = pVTable[SteamMatchmakingVTable::GetLobbyData];
	SteamMatchMakingFunctions.GetLobbyDataByIndexAddress = pVTable[SteamMatchmakingVTable::GetLobbyDataByIndex];
	SteamMatchMakingFunctions.GetLobbyMemberByIndexAddress = pVTable[SteamMatchmakingVTable::GetLobbyMemberByIndex];
	SteamMatchMakingFunctions.GetLobbyMemberDataAddress = pVTable[SteamMatchmakingVTable::GetLobbyMemberData];
	SteamMatchMakingFunctions.GetNumLobbyMembersAddress = pVTable[SteamMatchmakingVTable::GetNumLobbyMembers];
	SteamMatchMakingFunctions.InviteUserToLobbyAddress = pVTable[SteamMatchmakingVTable::InviteUserToLobby];
	SteamMatchMakingFunctions.JoinLobbyAddress = pVTable[SteamMatchmakingVTable::JoinLobby];
	SteamMatchMakingFunctions.LeaveLobbyAddress = pVTable[SteamMatchmakingVTable::LeaveLobby];
	SteamMatchMakingFunctions.RequestLobbyListAddress = pVTable[SteamMatchmakingVTable::RequestLobbyList];
	SteamMatchMakingFunctions.SendLobbyChatMsgAddress = pVTable[SteamMatchmakingVTable::SendLobbyChatMsg];
	SteamMatchMakingFunctions.SetLobbyDataAddress = pVTable[SteamMatchmakingVTable::SetLobbyData];
	SteamMatchMakingFunctions.SetLobbyMemberDataAddress = pVTable[SteamMatchmakingVTable::SetLobbyMemberData];



	__asm {
		mov eax, steamapiHandle
			add eax, 0x182bc
			mov eax, [eax]
			mov eax, [eax + 4]
			mov eax, [eax]
			mov SteamNetworking, eax
	}
	pVTable = (DWORD*)(SteamNetworking);

	SteamNetworkingFunctions.ReadP2PPacketAddress = pVTable[SteamNetworkingVTable::ReadP2PPacket];
	SteamNetworkingFunctions.SendP2PPacketAddress = pVTable[SteamNetworkingVTable::SendP2PPacket];

}
void initDXFunctions()
{
	DWORD* pVTable = (DWORD*)pD3dDevice;
	pVTable = (DWORD*)pVTable[0];
	DXFunctions.BeginSceneAddress = pVTable[DXVTable::BeginScene];
	DXFunctions.ColorFillAddress = pVTable[DXVTable::ColorFill];
	DXFunctions.CreateRenderTargetAddress = pVTable[DXVTable::CreateRenderTarget];
	DXFunctions.CreateTextureAddress = pVTable[DXVTable::CreateTexture];
	DXFunctions.CreateVertexBufferAddress = pVTable[DXVTable::CreateVertexBuffer];
	DXFunctions.CreateVertexShaderAddress = pVTable[DXVTable::CreateVertexShader];
	DXFunctions.DrawIndexedPrimitiveAddress = pVTable[DXVTable::DrawIndexedPrimitive];
	DXFunctions.DrawIndexedPrimitiveUPAddress = pVTable[DXVTable::DrawIndexedPrimitiveUP];
	DXFunctions.DrawPrimitiveAddress = pVTable[DXVTable::DrawPrimitive];
	DXFunctions.DrawPrimitiveUPAddress = pVTable[DXVTable::DrawPrimitiveUP];
	DXFunctions.EndSceneAddress = pVTable[DXVTable::EndScene];
	DXFunctions.GetLightAddress = pVTable[DXVTable::GetLight];
	DXFunctions.GetStreamSourceAddress = pVTable[DXVTable::GetStreamSource];
	DXFunctions.LightEnableAddress = pVTable[DXVTable::LightEnable];
	DXFunctions.ProcessVerticesAddress = pVTable[DXVTable::ProcessVertices];
	DXFunctions.ResetAddress = pVTable[DXVTable::Reset];
	DXFunctions.SetLightAddress = pVTable[DXVTable::SetLight];
	DXFunctions.SetRenderStateAddress = pVTable[DXVTable::SetRenderState];
	DXFunctions.SetStreamSourceAddress = pVTable[DXVTable::SetStreamSource];
	DXFunctions.SetTextureAddress = pVTable[DXVTable::SetTexture];
	DXFunctions.SetViewportAddress = pVTable[DXVTable::SetViewport];
}




bool InsertHook(void *pTarget, void *pDetour, void *pOriginal)
{
	if (MH_CreateHook(pTarget, pDetour, reinterpret_cast<void**>(pOriginal)))
		return false;
	if (MH_EnableHook(pTarget))
		return false;

	return true;
}


DWORD ModuleCheckingThread()
{
	if (MH_Initialize() != MH_OK)
		return -1;

	//In-Game Function Hooks
	*(PDWORD)&oDS_LandHit = (DWORD)DSGameFunctions.DS_LandHitAddress;

	InsertHook((void*)DSGameFunctions.DS_LandHitAddress, &hDS_LandHit, &oDS_LandHit);

	//Steam Matchmaking Hooks
	*(PDWORD)&oAddRequestLobbyListCompatibleMembersFilter = (DWORD)SteamMatchMakingFunctions.AddRequestLobbyListCompatibleMembersFilterAddress;
	*(PDWORD)&oAddRequestLobbyListDistanceFilter = (DWORD)SteamMatchMakingFunctions.AddRequestLobbyListDistanceFilterAddress;
	*(PDWORD)&oAddRequestLobbyListFilterSlotsAvailable = (DWORD)SteamMatchMakingFunctions.AddRequestLobbyListFilterSlotsAvailableAddress;
	*(PDWORD)&oAddRequestLobbyListNearValueFilter = (DWORD)SteamMatchMakingFunctions.AddRequestLobbyListNearValueFilterAddress;
	*(PDWORD)&oAddRequestLobbyListNumericalFilter = (DWORD)SteamMatchMakingFunctions.AddRequestLobbyListNumericalFilterAddress;
	*(PDWORD)&oAddRequestLobbyListResultCountFilter = (DWORD)SteamMatchMakingFunctions.AddRequestLobbyListResultCountFilterAddress;
	*(PDWORD)&oAddRequestLobbyListStringFilter = (DWORD)SteamMatchMakingFunctions.AddRequestLobbyListStringFilterAddress;
	*(PDWORD)&oCreateLobby = (DWORD)SteamMatchMakingFunctions.CreateLobbyAddress;
	*(PDWORD)&oGetLobbyByIndex = (DWORD)SteamMatchMakingFunctions.GetLobbyByIndexAddress;
	*(PDWORD)&oGetLobbyChatEntry = (DWORD)SteamMatchMakingFunctions.GetLobbyChatEntryAddress;
	*(PDWORD)&oGetLobbyData = (DWORD)SteamMatchMakingFunctions.GetLobbyDataAddress;
	*(PDWORD)&oGetLobbyDataByIndex = (DWORD)SteamMatchMakingFunctions.GetLobbyDataByIndexAddress;
	*(PDWORD)&oGetLobbyMemberByIndex = (DWORD)SteamMatchMakingFunctions.GetLobbyMemberByIndexAddress;
	*(PDWORD)&oGetLobbyMemberData = (DWORD)SteamMatchMakingFunctions.GetLobbyMemberDataAddress;
	*(PDWORD)&oGetNumLobbyMembers = (DWORD)SteamMatchMakingFunctions.GetNumLobbyMembersAddress;
	*(PDWORD)&oInviteUserToLobby = (DWORD)SteamMatchMakingFunctions.InviteUserToLobbyAddress;
	*(PDWORD)&oJoinLobby = (DWORD)SteamMatchMakingFunctions.JoinLobbyAddress;
	*(PDWORD)&oLeaveLobby = (DWORD)SteamMatchMakingFunctions.LeaveLobbyAddress;
	*(PDWORD)&oRequestLobbyList = (DWORD)SteamMatchMakingFunctions.RequestLobbyListAddress;
	*(PDWORD)&oSendLobbyChatMsg = (DWORD)SteamMatchMakingFunctions.SendLobbyChatMsgAddress;
	*(PDWORD)&oSetLobbyData = (DWORD)SteamMatchMakingFunctions.SetLobbyDataAddress;
	*(PDWORD)&oSetLobbyMemberData = (DWORD)SteamMatchMakingFunctions.SetLobbyMemberDataAddress;
	

	/*
	//These need checking, gets and sets can crash
	InsertHook((void*)SteamMatchMakingFunctions.AddRequestLobbyListCompatibleMembersFilterAddress, &hAddRequestLobbyListCompatibleMembersFilter, &oAddRequestLobbyListCompatibleMembersFilter);
	//InsertHook((void*)SteamMatchMakingFunctions.AddRequestLobbyListDistanceFilterAddress, &hAddRequestLobbyListDistanceFilter, &oAddRequestLobbyListDistanceFilter);
	InsertHook((void*)SteamMatchMakingFunctions.AddRequestLobbyListFilterSlotsAvailableAddress, &hAddRequestLobbyListFilterSlotsAvailable, &oAddRequestLobbyListFilterSlotsAvailable);
	InsertHook((void*)SteamMatchMakingFunctions.AddRequestLobbyListNearValueFilterAddress, &hAddRequestLobbyListNearValueFilter, &oAddRequestLobbyListNearValueFilter);
	InsertHook((void*)SteamMatchMakingFunctions.AddRequestLobbyListNumericalFilterAddress, &hAddRequestLobbyListNumericalFilter, &oAddRequestLobbyListNumericalFilter);
	InsertHook((void*)SteamMatchMakingFunctions.AddRequestLobbyListResultCountFilterAddress, &hAddRequestLobbyListResultCountFilter, &oAddRequestLobbyListResultCountFilter);
	InsertHook((void*)SteamMatchMakingFunctions.AddRequestLobbyListStringFilterAddress, &hAddRequestLobbyListStringFilter, &oAddRequestLobbyListStringFilter);
	InsertHook((void*)SteamMatchMakingFunctions.CreateLobbyAddress, &hCreateIndexBuffer, &oCreateIndexBuffer);
	InsertHook((void*)SteamMatchMakingFunctions.GetLobbyByIndexAddress, &hGetLobbyByIndex, &oGetLobbyByIndex);
	InsertHook((void*)SteamMatchMakingFunctions.GetLobbyChatEntryAddress, &hGetLobbyChatEntry, &oGetLobbyChatEntry);
	InsertHook((void*)SteamMatchMakingFunctions.GetLobbyDataAddress, &hGetLobbyData, &oGetLobbyData);
	InsertHook((void*)SteamMatchMakingFunctions.GetLobbyDataByIndexAddress, &hGetLobbyDataByIndex, &oGetLobbyDataByIndex);
	InsertHook((void*)SteamMatchMakingFunctions.GetLobbyMemberByIndexAddress, &hGetLobbyMemberByIndex, &oGetLobbyMemberByIndex);
	InsertHook((void*)SteamMatchMakingFunctions.GetLobbyMemberDataAddress, &hGetLobbyMemberData, &oGetLobbyMemberData);
	InsertHook((void*)SteamMatchMakingFunctions.GetNumLobbyMembersAddress, &hGetNumLobbyMembers, &oGetNumLobbyMembers);
	InsertHook((void*)SteamMatchMakingFunctions.InviteUserToLobbyAddress, &hInviteUserToLobby, &oInviteUserToLobby);
	InsertHook((void*)SteamMatchMakingFunctions.JoinLobbyAddress, &hJoinLobby, &oJoinLobby);
	InsertHook((void*)SteamMatchMakingFunctions.LeaveLobbyAddress, &hLeaveLobby, &oLeaveLobby);
	InsertHook((void*)SteamMatchMakingFunctions.RequestLobbyListAddress, &hRequestLobbyList, &oRequestLobbyList);
	InsertHook((void*)SteamMatchMakingFunctions.SendLobbyChatMsgAddress, &hSendLobbyChatMsg, &oSendLobbyChatMsg);
	//InsertHook((void*)SteamMatchMakingFunctions.SetLobbyDataAddress, &hSetLobbyData, &oSetLobbyData);
	//InsertHook((void*)SteamMatchMakingFunctions.SetLobbyMemberDataAddress, &hSetLobbyMemberData, &oSetLobbyMemberData);
	*/


	//Steam Networking Hooks
	/*
	*(PDWORD)&oReadP2PPacket = (DWORD)SteamNetworkingFunctions.ReadP2PPacketAddress;
	*(PDWORD)&oSendP2PPacket = (DWORD)SteamNetworkingFunctions.SendP2PPacketAddress;
	*/

	
	//InsertHook((void*)SteamNetworkingFunctions.ReadP2PPacketAddress, &hReadP2PPacket, &oReadP2PPacket);
	//InsertHook((void*)SteamNetworkingFunctions.SendP2PPacketAddress, &hSendP2PPacket, &oSendP2PPacket);
	

	//D3D9 Hooks
	*(PDWORD)&oBeginScene = (DWORD)DXFunctions.BeginSceneAddress;
	*(PDWORD)&oColorFill = (DWORD)DXFunctions.ColorFillAddress;
	*(PDWORD)&oCreateIndexBuffer = (DWORD)DXFunctions.CreateIndexBufferAddress;
	*(PDWORD)&oCreateRenderTarget = (DWORD)DXFunctions.CreateRenderTargetAddress;
	*(PDWORD)&oCreateTexture = (DWORD)DXFunctions.CreateTextureAddress;
	*(PDWORD)&oCreateVertexBuffer = (DWORD)DXFunctions.CreateVertexBufferAddress;
	*(PDWORD)&oCreateVertexShader = (DWORD)DXFunctions.CreateVertexShaderAddress;
	*(PDWORD)&oDrawIndexedPrimitive = (DWORD)DXFunctions.DrawIndexedPrimitiveAddress;
	*(PDWORD)&oDrawIndexedPrimitiveUP = (DWORD)DXFunctions.DrawIndexedPrimitiveUPAddress;
	*(PDWORD)&oDrawPrimitive = (DWORD)DXFunctions.DrawPrimitiveAddress;
	*(PDWORD)&oDrawPrimitiveUP = (DWORD)DXFunctions.DrawPrimitiveUPAddress;
	*(PDWORD)&oEndScene = (DWORD)DXFunctions.EndSceneAddress;
	*(PDWORD)&oGetLight = (DWORD)DXFunctions.GetLightAddress;
	*(PDWORD)&oGetStreamSource = (DWORD)DXFunctions.GetStreamSourceAddress;
	*(PDWORD)&oLightEnable = (DWORD)DXFunctions.LightEnableAddress;
	*(PDWORD)&oProcessVertices = (DWORD)DXFunctions.ProcessVerticesAddress;
	*(PDWORD)&oReset = (DWORD)DXFunctions.ResetAddress;
	*(PDWORD)&oSetLight = (DWORD)DXFunctions.SetLightAddress;
	*(PDWORD)&oSetMaterial = (DWORD)DXFunctions.SetMaterialAddress;
	*(PDWORD)&oSetRenderState = (DWORD)DXFunctions.SetRenderStateAddress;
	*(PDWORD)&oSetStreamSource = (DWORD)DXFunctions.SetStreamSourceAddress;
	*(PDWORD)&oSetTexture = (DWORD)DXFunctions.SetTextureAddress;

	InsertHook((void*)DXFunctions.BeginSceneAddress, &hBeginScene, &oBeginScene);
	InsertHook((void*)DXFunctions.ColorFillAddress, &hColorFill, &oColorFill);
	InsertHook((void*)DXFunctions.CreateIndexBufferAddress, &hCreateIndexBuffer, &oCreateIndexBuffer);
	InsertHook((void*)DXFunctions.CreateRenderTargetAddress, &hCreateRenderTarget, &oCreateRenderTarget);
	InsertHook((void*)DXFunctions.CreateTextureAddress, &hCreateTexture, &oCreateTexture);
	InsertHook((void*)DXFunctions.CreateVertexBufferAddress, &hCreateVertexBuffer, &oCreateVertexBuffer);
	InsertHook((void*)DXFunctions.CreateVertexShaderAddress, &hCreateVertexShader, &oCreateVertexShader);
	InsertHook((void*)DXFunctions.DrawIndexedPrimitiveAddress, &hDrawIndexedPrimitive, &oDrawIndexedPrimitive);
	InsertHook((void*)DXFunctions.DrawIndexedPrimitiveUPAddress, &hDrawIndexedPrimitiveUP, &oDrawIndexedPrimitiveUP);
	InsertHook((void*)DXFunctions.DrawPrimitiveAddress, &hDrawPrimitive, &oDrawPrimitive);
	InsertHook((void*)DXFunctions.DrawPrimitiveUPAddress, &hDrawPrimitiveUP, &oDrawPrimitiveUP);
	InsertHook((void*)DXFunctions.EndSceneAddress, &hEndScene, &oEndScene);
	InsertHook((void*)DXFunctions.GetLightAddress, &hGetLight, &oGetLight);
	InsertHook((void*)DXFunctions.GetStreamSourceAddress, &hGetStreamSource, &oGetStreamSource);
	InsertHook((void*)DXFunctions.LightEnableAddress, &hLightEnable, &oLightEnable);
	InsertHook((void*)DXFunctions.ProcessVerticesAddress, &hProcessVertices, &oProcessVertices);
	InsertHook((void*)DXFunctions.ResetAddress, &hReset, &oReset);
	InsertHook((void*)DXFunctions.SetLightAddress, &hSetLight, &oSetLight);
	InsertHook((void*)DXFunctions.SetMaterialAddress, &hSetLight, &oSetMaterial);
	InsertHook((void*)DXFunctions.SetRenderStateAddress, &hSetRenderState, &oSetRenderState);
	InsertHook((void*)DXFunctions.SetStreamSourceAddress, &hSetStreamSource, &oSetStreamSource);
	InsertHook((void*)DXFunctions.SetTextureAddress, &hSetTexture, &oSetTexture);



	return 0;
}










void initDXFunctions();





bool Initialize_DirectX()
{
	//printf("Initialize_DirectX Called.\n");

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

	//printf("pD3dDevice: %p\n", pD3dDevice);



	if (!debugEXE)
	{
		DWORD OldProtect = 0;
		VirtualProtect((LPVOID)0xbe73fe, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
		VirtualProtect((LPVOID)0xbe719f, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
		VirtualProtect((LPVOID)0xbe722b, 1, PAGE_EXECUTE_READWRITE, &OldProtect);


		__asm {
			mov eax, 0xBE73FE
			mov bl, ver
			mov[eax], bl

			mov eax, 0xBE719F
			mov[eax], bl

			mov eax, 0xBE722B
			mov[eax], bl
		}
	}



	initDXFunctions();



	pD3dDevice->GetViewport(&d3dViewport);
	printf("d3dViewport.width: %d\n", d3dViewport.Width);
	printf("d3dViewport.height: %d\n", d3dViewport.Height);

	return true;
}
bool CreateDefaultFont()
{
	if (gFont == NULL)
	{
		//printf("\tgFont == NULL\n");
		pD3dDevice->GetViewport(&d3dViewport);
		fontLineSpacing = (d3dViewport.Height >= 1080 ? FONT_SPACING_1080 : FONT_SPACING_720);
		fontSize = (d3dViewport.Height >= 1080 ? FONT_SIZE_1080 : FONT_SIZE_720);

		if (HRESULT fontCreateResult = D3DXCreateFont(pD3dDevice, fontSize, 0, FW_NORMAL, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas", &gFont) != S_OK)
		{
			printf("Problem creating Font: Result = %lu  -  %08X\n", fontCreateResult, fontCreateResult);
			return false;
		}
		//printf("\tCreated gFont: %p\n", gFont);
		//printf("\tFont Size: %d\n", fontSize);
		//printf("\tFont Line Spacing: %d\n", fontLineSpacing);
	}

	if (giFont == NULL)
	{
		//printf("\tgiFont == NULL\n");
		pD3dDevice->GetViewport(&d3dViewport);
		fontLineSpacing = (d3dViewport.Height >= 1080 ? FONT_SPACING_1080 : FONT_SPACING_720);
		fontSize = (d3dViewport.Height >= 1080 ? FONT_SIZE_1080 : FONT_SIZE_720);

		if (HRESULT fontCreateResult = D3DXCreateFont(pD3dDevice, (fontSize + 2), 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas", &giFont) != S_OK)
		{
			printf("Problem creating Font: Result = %lu  -  %08X\n", fontCreateResult, fontCreateResult);
			return false;
		}
		//printf("\tCreated giFont: %p\n", giFont);
		//printf("\tFont Size: %d\n", fontSize);
		//printf("\tFont Line Spacing: %d\n", fontLineSpacing);
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

float xP(float x)
{
	return (d3dViewport.Width / 100) * x;
}
float yP(float y)
{
	return (d3dViewport.Height / 100) * y;
}


void drawStuff()
{
	bool wf = wireframe;

	if (wf)
		wireframe = !wireframe;

	__asm {
		mov chardata1, 0

		mov eax, 0x137dc70
		mov eax, [eax]
		cmp	eax, 0
		je ptrZero

		mov eax, [eax+4]
		cmp eax, 0
		je ptrZero

		mov eax, [eax]
		cmp eax, 0
		je ptrZero

		mov chardata1, eax

		ptrZero:
	}
	
	if (showstats && chardata1)
	{
		wstring text = L"";

		creatureData1 *self = (creatureData1*)chardata1;
		creatureData1 *target = (creatureData1*)lastenemy;

		if (self->currHP < 1)
			lastenemy = NULL;
		

		//HP
		text = to_wstring(self->currHP) + L" / " + to_wstring(self->maxHP);
		DrawScreenText(giFont, text, xP(15), yP(11.25), C_WHITE);

		//Stam
		text = to_wstring(self->currStam) + L" / " + to_wstring(self->maxStam);
		DrawScreenText(giFont, text, xP(15), yP(14.25), C_WHITE);

		
		/*Display 5 last hits
		for (int i = 0; i < 5; i++)
		{
			text = lastAtk[i] + L" hit " + lastDef[i] + L" for " + to_wstring(lastDmg[i]) + L" damage.";
			DrawScreenText(giFont, text, xP(10), yP(25 + (i*2)), C_WHITE);
		}
		*/

		if (lastenemy)
		{
			DrawGradientBox(pD3dDevice, xP(3), yP(25), xP(10), yP(32), C_BLACK, C_BLACK);

			text = target->modelName;
			DrawScreenText(giFont, text, xP(4), yP(26), C_WHITE);

			text = L"HP: " + to_wstring(target->currHP) + L" / " + to_wstring(target->maxHP);
			DrawScreenText(giFont, text, xP(4), yP(28), C_WHITE);
			text = L"Stam: " + to_wstring(target->currStam) + L" / " + to_wstring(target->maxStam);
			DrawScreenText(giFont, text, xP(4), yP(30), C_WHITE);

			text = L"TeamType: " + to_wstring(target->TeamType);
			DrawScreenText(giFont, text, xP(4), yP(34), C_WHITE);
			
			text = L"NPC ID: " + to_wstring(target->NPCID);
			DrawScreenText(giFont, text, xP(4), yP(36), C_WHITE);


			text = L"Resists";
			DrawScreenText(giFont, text, xP(4), yP(40), C_WHITE);
			text = L"P: " + to_wstring(target->currentPoisonResist);
			DrawScreenText(giFont, text, xP(4), yP(42), C_VIOLET);
			text = L"T: " + to_wstring(target->currentToxicResist);
			DrawScreenText(giFont, text, xP(4), yP(44), C_PURPLE);
			text = L"B: " + to_wstring(target->currentBleedResist);
			DrawScreenText(giFont, text, xP(4), yP(46), C_RED);
			text = L"C: " + to_wstring(target->currentCurseResist);
			DrawScreenText(giFont, text, xP(4), yP(48), C_DARKGRAY);

			CString fltStr1;
			CString fltStr2;
			fltStr1.Format(_T("%.1f"), target->currPoise);
			fltStr2.Format(_T("%.1f"), target->maxPoise);

			text = L"Poise: " + fltStr1 + L" / " + fltStr2;
			DrawScreenText(giFont, text, xP(4), yP(52), C_DARKGRAY);
			fltStr1.Format(_T("%.1f"), target->poiseRecoveryTimer);
			text = L"Poise recov: " + fltStr1;
			DrawScreenText(giFont, text, xP(4), yP(54), C_DARKGRAY);

			printf("Memloc: %p\n", lastenemy);



			text = lastAtk[4] + L" hit " + lastDef[4] + L" with an attack power of " + to_wstring(lastDmg[4]);
			DrawScreenText(giFont, text, xP(2), yP(99.5), C_WHITE);


		}
	}

	if (wf)
		wireframe = !wireframe;
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



void Initialize()
{
	fout << "Time,Direction,nChannel,EP2PSendType,SteamID,Size,Contents" << endl;

	HANDLE hDarkSouls = GetModuleHandleA("DarkSouls.exe");

	if (!hDarkSouls)
	{	
		printf("ERROR: Getting handle to darksouls.exe\n");
		return;
	}

	printf("Handle: %p\n", hDarkSouls);

	initInGameFunctions();
	initSteamFunctions();

	Initialize_DirectX();
	CreateDefaultFont();
	ModuleCheckingThread();

	printf("Last Error: %d\n", GetLastError());

	_beginthread(&hotkeyThread, 0, 0);


	printf("%p, %p, %p\n", (void*)SteamNetworkingFunctions.SendP2PPacketAddress, &hSendP2PPacket, &oSendP2PPacket);
	printf("Read:  %X\n", SteamNetworkingFunctions.ReadP2PPacketAddress);
	printf("Send:  %X\n", SteamNetworkingFunctions.SendP2PPacketAddress);
	

}
void Cleanup()
{
	MH_DisableHook(MH_ALL_HOOKS);
	fout.close();
}
void Run()
{
	bRunning = true;




	while (bRunning)
	{		

		

		Sleep(33);
	}
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

		if ((hforegroundWnd == hDarkSouls) || (hDarkSouls == NULL))
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
					showstats = !showstats;

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



