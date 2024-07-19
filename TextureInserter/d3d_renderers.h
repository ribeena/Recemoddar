#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <cstdint>


// Forward declare DirectX functions and structures
//struct IDirect3DDevice8;
//struct IDirect3DTexture8;
//LPDIRECT3DDEVICE9 g_pd3dDevice;

void setScreenOffsetScale(int x, int y, int s);
void setConfigRPOveray(bool value);
void setConfigRPOverayAlt(bool value);
void setDrawRPOverlay(bool value);
bool getDrawRPOverlay();
void setDrawRPtransition(int value);
void changeDrawRPtransition(int value);
int getDrawRPtransition();
void setCustomerRP(int value);
void setCustomerRPLvl(int value);
void setlogoXPos(int value);
int GetCustomerAlt();

DWORD WINAPI DirectXInit(__in  LPVOID lpParameter);

//bool HookD3D();
//bool CreateTestPanel();
//bool RenderTestPanel();
bool CreateTestTriangle(LPDIRECT3DDEVICE9 g_pd3dDevice);
void RenderTestTriangle(LPDIRECT3DDEVICE9 g_pd3dDevice);
void DrawOverlay(LPDIRECT3DDEVICE9 g_pD3DDevice9);
//void RenderText(IDirect3DDevice8* device, float x, float y, const char* text, uint32_t color);
//void RenderImage(IDirect3DDevice8* device, const char* imagePath, float x, float y);
