// thanks to https://github.com/edgeforce/directx8/tree/msdx8

#define _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
//or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to suppress this warning.	Recemoddar	D:\game modding\Recettear\DDSInserter\TextureInserter\TextureInserter\d3d_renderers.cpp	35		


#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x86\\d3dx9.lib")

#include <MinHook.h>
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <cstdint>
#include <unordered_map>
#include <string>

#include "d3d_renderers.h"

struct Rect {
    float left, top, right, bottom;
    Rect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
};

// Define the vertex structure with texture coordinates
struct Vertex {
    float x, y, z, rhw;
    DWORD color;
    float u, v; // Texture coordinates
};

//Shading reference
//https://www.realtimerendering.com/resources/shaderx/Direct3D.ShaderX.Vertex.and.Pixel.Shader.Tips.and.Tricks_Wolfgang.F.Engel_Wordware.Pub_2002.pdf

int screenOffsetX = 0;
int screenOffsetY = 0;
int screenScale = 1;
void setScreenOffsetScale(int x, int y, int s) { screenOffsetX = x; screenOffsetY = y; screenScale = s; }
bool configRPOveray = true;
void setConfigRPOveray(bool value) { configRPOveray = value; }
bool configRPOverayAlt = true;
void setConfigRPOverayAlt(bool value) { configRPOverayAlt = value; }
bool drawRPOverlay = false;
void setDrawRPOverlay(bool value) { drawRPOverlay = value; }
bool getDrawRPOverlay() { return drawRPOverlay; }
int drawRPtransition = 0;
void setDrawRPtransition(int value) { drawRPtransition = value; }
void changeDrawRPtransition(int value) { drawRPtransition += value; }
int getDrawRPtransition() { return drawRPtransition;}
int customerRP = 57;
void setCustomerRP(int value) { customerRP = value; }
int customerRPLvl = 5;
void setCustomerRPLvl(int value) { customerRPLvl = value; }
int logoXPos = 0;
void setlogoXPos(int value) { logoXPos = value; }


LPDIRECT3DTEXTURE9 t_loading = nullptr; // The texture for the image
LPDIRECT3DTEXTURE9 t_shopmode = nullptr;


typedef HRESULT(APIENTRY* EndScene_t) (LPDIRECT3DDEVICE9);
HRESULT APIENTRY EndScene_hook(LPDIRECT3DDEVICE9);
EndScene_t originalEndScene = nullptr;

IDirect3DDevice9* g_pD3DDevice9 = nullptr;
HMODULE hModule;

// Create a reputation structure for reading
struct CustomerReputation {
    short reputationPoints;
    short level;
};
CustomerReputation* customerReputations;
int* currentCustomer;
int* s_openStore;
int* s_buySell;
int* s_startScreenSub;
int* s_game;

// Array of customer names
const std::string customerNames[24] = {
    "00: Louie", "01: Charme", "02: Caillou", "03: Tielle",
    "04: Elan", "05: Nagi", "06: Griff", "07: Arma", "08: Blacksmith",
    "09: Woman (base)", "10: Woman (Alt.1)", "11: Townie Woman (Alt.2)",
    "12: Man (base)", "13: Man (Alt.1)", "14: Townie Man (Alt.2)",
    "15: Old Man (base)", "16: Old Man (Alt.1)", "17: Old Man (Alt.2)",
    "18: Girl (base)", "19: Girl (Alt.1)", "20: Girl (Alt.2)",
    "21: Euria", "22: Allouette", "23: Prime"
};

void LinkMemoryAddresses() {
    s_game = reinterpret_cast<int*>(0x0438b1c0);
    s_startScreenSub = reinterpret_cast<int*>(0x09643520);

    // Calculate the base address of the customer reputations array
    customerReputations = reinterpret_cast<CustomerReputation*>(0x045109a8);

    // Get the index of customer reputations array entry
    currentCustomer = reinterpret_cast<int*>(0x0730B570);

    // Get the OpenStore state
    s_openStore = reinterpret_cast<int*>(0x0438b7b0);

    // Get the Buying or selling state
    s_buySell = reinterpret_cast<int*>(0x0730b5a0);
}

void DisplayCustomerRPs() {
    if (!getDrawRPOverlay()) {
        char buffer[256];
        if (customerReputations == nullptr) {
            OutputDebugStringA("Customer reputations pointer is null.");
            return;
        }

        // Print the customer reputations
        for (int i = 0; i < 24; ++i) {
            sprintf(buffer, "Customer %i: Reputation Points = %i, Level = %i", i, customerReputations[i].reputationPoints, customerReputations[i].level);
            OutputDebugStringA(buffer);
        }
        sprintf(buffer, " - Current Customer %i", currentCustomer);
        OutputDebugStringA(buffer);
    }
}

HRESULT WINAPI HookedEndScene(LPDIRECT3DDEVICE9 pDevice) {
    if (configRPOveray) {
        if (*s_openStore != 0 && *s_buySell != 0) {
            if (drawRPtransition < 10) {
                drawRPtransition++;
            }
            drawRPOverlay = true;

            if (*currentCustomer > -1) {
                setCustomerRP(customerReputations[*currentCustomer].reputationPoints);
                setCustomerRPLvl(customerReputations[*currentCustomer].level);
            }
        }
        else {
            if (drawRPtransition > 0) {
                drawRPtransition--;
            }
            else {
                drawRPOverlay = false;
            }
        }
    }

    DrawOverlay(pDevice);

    return originalEndScene(pDevice);
}

DWORD WINAPI DirectXInit(__in  LPVOID lpParameter);

DWORD WINAPI DirectXInit(__in  LPVOID lpParameter) {
    LPDIRECT3DDEVICE9 d3ddev = NULL;

    //Wait for directx9 to be loaded
    while (GetModuleHandle(L"d3d9.dll") == 0)
    {
        Sleep(100);
    }

    if (MH_Initialize() != MH_OK) {
        OutputDebugStringA("MinHook failed");
        Sleep(100);
    }

    //Connect to the memory
    LinkMemoryAddresses();

    // Get the vtable from the Direct3D device
    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);

    HWND tmpWnd = CreateWindowA("BUTTON", "Temp Window", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, hModule, NULL);
    if (tmpWnd == NULL)
    {
        OutputDebugStringA("CreateWindowA failed");
        return 0;
    }


    OutputDebugStringA("Direct3DCreate9");
    if (pD3D) {
        D3DPRESENT_PARAMETERS d3dpp = {};
        //d3dpp.Windowed = TRUE;
        //d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        OutputDebugStringA("CreateDevice with tmpWnd");

        ZeroMemory(&d3dpp, sizeof(d3dpp));
        d3dpp.Windowed = TRUE;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.hDeviceWindow = tmpWnd;
        d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

        //d3dpp.hDeviceWindow = GetForegroundWindow();

        HRESULT result = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, tmpWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3ddev);
        if (result != D3D_OK)
        {
            OutputDebugStringA("CreateDevice failed");
            pD3D->Release();
            DestroyWindow(tmpWnd);
            return 0;
        }

        DWORD* dVtable = (DWORD*)d3ddev;
        dVtable = (DWORD*)dVtable[0];

        originalEndScene = (EndScene_t)dVtable[42];

        if (MH_CreateHook((DWORD_PTR*)dVtable[42], &HookedEndScene, reinterpret_cast<void**>(&originalEndScene)) != MH_OK) {
            OutputDebugStringA("MH_CreateHook HookedEndScene failed");
        }

        if (MH_EnableHook((DWORD_PTR*)dVtable[42])) {
            OutputDebugStringA("EnableHook HookedEndScene failed");
        }


        d3ddev->Release();
        pD3D->Release();
        DestroyWindow(tmpWnd);

        OutputDebugStringA("CreateDevice released");

    }
    else {
        DestroyWindow(tmpWnd);
        OutputDebugStringA("Direct3DCreate9 - failed");
        return 0;
    }

    


    return 1;
}


Rect GetUVRect(Rect tDim, int width, int height) {
    Rect uvRect(0, 0, 0, 0);
    uvRect.left = tDim.left / static_cast<float>(width);
    uvRect.top = tDim.top / static_cast<float>(height);
    uvRect.right = tDim.right / static_cast<float>(width);
    uvRect.bottom = tDim.bottom / static_cast<float>(height);
    return uvRect;
}

DWORD CreateARGBColor(int r, int g, int b, int a) {
    DWORD color = (a << 24) | (r << 16) | (g << 8) | b;
    return color;
}


//Co-ordinates are based on 640x480 screen, and 0-1 UV map
void DrawUIOverlay(LPDIRECT3DDEVICE9 g_pD3DDevice9, const Rect& screenRect, const Rect& uvRect, LPDIRECT3DTEXTURE9& pTexture, int alpha = 255) {

    float width = screenRect.right - screenRect.left;
    float height = screenRect.bottom - screenRect.top;
    DWORD colour = CreateARGBColor(255, 255, 255, alpha);

    Vertex vertices[] = {
        { screenRect.left * screenScale + screenOffsetX, screenRect.top * screenScale + screenOffsetY, 0.0f, 1.0f, colour, uvRect.left, uvRect.top}, // Top left
        { screenRect.left * screenScale + (width * screenScale) + screenOffsetX, screenRect.top * screenScale + screenOffsetY, 0.0f, 1.0f, colour, uvRect.right, uvRect.top }, // Top right
        { screenRect.left * screenScale + screenOffsetX, screenRect.top * screenScale +(height * screenScale) + screenOffsetY, 0.0f, 1.0f, colour, uvRect.left, uvRect.bottom }, // Bottom left
        { screenRect.left * screenScale + (width * screenScale) + screenOffsetX, screenRect.top * screenScale + (height * screenScale) + screenOffsetY, 0.0f, 1.0f, colour, uvRect.right, uvRect.bottom }, // Bottom right
    };



    // Set the texture
    HRESULT hr = g_pD3DDevice9->SetTexture(0, pTexture);
    if (FAILED(hr)) {
        OutputDebugStringA("DrawUIOverlay: SetTexture failed.");
        return;
    }

    // Draw the rectangle with the texture
    g_pD3DDevice9->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    g_pD3DDevice9->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
}


void DrawOverlay(LPDIRECT3DDEVICE9 g_pD3DDevice9) {
    if (!g_pD3DDevice9) return;

    // Save the current state
    DWORD renderStateA, renderStateSB, renderStateDB;
    g_pD3DDevice9->GetRenderState(D3DRS_ALPHABLENDENABLE, &renderStateA);
    g_pD3DDevice9->GetRenderState(D3DRS_SRCBLEND, &renderStateSB);
    g_pD3DDevice9->GetRenderState(D3DRS_DESTBLEND, &renderStateDB);

    // Update render state
    g_pD3DDevice9->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pD3DDevice9->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pD3DDevice9->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    // Load overlay textures
    if (!t_shopmode) {
        D3DXCreateTextureFromFile(g_pD3DDevice9, L"mods\\overlay1.dds", &t_shopmode);
    }

    if (*s_startScreenSub == 0 && *s_game == 0) {
        // Draw textures
        DrawUIOverlay(g_pD3DDevice9, Rect(logoXPos + 430.0, 180.0, logoXPos + 430.0 + 74.0, 180.0 + 29.0),
            GetUVRect(Rect(0, 513, 244, 513 + 98), 512, 1024), t_shopmode);
    }

    if (getDrawRPOverlay()) {

        int alpha = drawRPtransition * 26;
        if (alpha > 255) alpha = 255;
        // Draw Reputation Points

        int x = 15;
        int y = 5;

        //backing
        float size = 64.0f;
        DrawUIOverlay(g_pD3DDevice9, Rect(640.0 - (x + size), 480.0 - (y +size), 640.0 - x, 480.0-y),
            GetUVRect(Rect(0, 256, 256, 512), 512, 1024), t_shopmode, alpha);

        float perc = (float)(customerRP/10) / 10.0;
        if (customerRPLvl >= 8) {
            perc = 1;
        }

        int offsetHeight = (int)((1-perc) * 256.0f);
        if (perc > 0.01) {
            //fill amount
            DrawUIOverlay(g_pD3DDevice9, Rect(640.0 - (x + size), 480.0 - (y+(size * perc)), 640.0 - x, 480.0-y),
                GetUVRect(Rect(0, offsetHeight, 256, 256), 512, 1024), t_shopmode, alpha);
        }

        //number
        size = 22.0f;//Around 33%
        offsetHeight = customerRPLvl * 86;
        DrawUIOverlay(g_pD3DDevice9, Rect(640.0 - (x + size + 21.0), 480.0 - (y+size + 21.0), 640.0 - (x + 21.0), 480.0 - (y+21.0)),
            GetUVRect(Rect(256, offsetHeight, 256 + 86, offsetHeight + 86), 512, 1024), t_shopmode, alpha);

        if (configRPOverayAlt) {
            //Alt A
            if (*currentCustomer == 10 || *currentCustomer == 13 || *currentCustomer == 16 || *currentCustomer == 19) {
                DrawUIOverlay(g_pD3DDevice9, Rect(640.0 - (x + size + 50.0), 480.0 - (y + size + 21.0), 640.0 - (x + 50.0), 480.0 - (y + 21.0)),
                    GetUVRect(Rect(0, 613, 128, 613 + 128), 512, 1024), t_shopmode, alpha);
            }

            //Alt B
            if (*currentCustomer == 11 || *currentCustomer == 14 || *currentCustomer == 17 || *currentCustomer == 20) {
                DrawUIOverlay(g_pD3DDevice9, Rect(640.0 - (x + size + 50.0), 480.0 - (y + size + 21.0), 640.0 - (x + 50.0), 480.0 - (y + 21.0)),
                    GetUVRect(Rect(0, 613 + 128, 128, 613 + 128 + 128), 512, 1024), t_shopmode, alpha);
            }
        }
    }

    // Restore the state
    g_pD3DDevice9->SetRenderState(D3DRS_ALPHABLENDENABLE, renderStateA);
    g_pD3DDevice9->SetRenderState(D3DRS_SRCBLEND, renderStateSB);
    g_pD3DDevice9->SetRenderState(D3DRS_DESTBLEND, renderStateDB);
}

