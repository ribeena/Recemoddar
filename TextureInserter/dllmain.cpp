
#define ZYDIS_STATIC_BUILD
#define _WIN32

#include "pch.h"

#include <stdio.h>
#include <MinHook.h>
#include <fstream>
#include <cstring>
#include <map>
#include <iomanip>
#include <sstream>
#include <cstdio>

#include "SimpleIniParser.h"
#include <unordered_map>
#include <string>
#include <regex>

#if _WIN64
#pragma comment(lib, "libMinHook.x64.lib")
#else
#pragma comment(lib, "libMinHook.x86.lib")
#endif

typedef struct {
    int width;
    int height;
    int format;
} astruct;



// Structs for hooks
struct uvRect {
    float x;
    float y;
    float w;
    float h;
};

struct vector3 {
    unsigned int x;
    unsigned int y;
    int z;
};

struct imgData {
    vector3* file;
    int width;
    int height;
    int format;
};

//Variables for mod
RECT g_windowRect = { 0 };
uvRect screenSize;
uvRect vScreenSize;

std::unordered_map<std::string, int> widescreenBackgrounds;

float characterOffsets = 0;
bool letterBoxed = false;
bool debugImagesFound = false;
bool debugImagesSearched = false;
bool debugEventFiles = false;
bool debug3DFiles = false;
int eventOffscreenAdjustment = -1;
int eventOffscreenTestValue = 340;
int eventVerticalOffset = 0;

//Keeping track of X file loading
bool loadingXFile = false;
char lastFilename[512];

std::map<astruct*, int> imgPointers;



void LoadConfiguration() {
    try {
        SimpleIniParser parser("recemoddar.ini");
        widescreenBackgrounds = parser.getSection("WidescreenBackgrounds");

        // Check if the section "General" and key "CharacterOffsets" exist
        auto generalSection = parser.getSection("General");
        if (generalSection.find("CharacterOffsets") != generalSection.end()) {
            characterOffsets = (float)(generalSection["CharacterOffsets"]);
        }
        if (generalSection.find("LetterBoxed") != generalSection.end()) {
            letterBoxed = (generalSection["LetterBoxed"] == 1);
        }
        if (generalSection.find("EventOffscreenAdjustment") != generalSection.end()) {
            eventOffscreenAdjustment = generalSection["EventOffscreenAdjustment"];
            if (eventOffscreenAdjustment < 0) {
                if (screenSize.w != NULL) {
                    eventOffscreenAdjustment = ((screenSize.w / 2) - 640) / 2;
                }
                else {
                    eventOffscreenAdjustment = 0;
                }
            }
        }
        if (generalSection.find("EventOffscreenTestValue") != generalSection.end()) {
            eventOffscreenTestValue = generalSection["EventOffscreenTestValue"];
        }
        if (generalSection.find("EventVerticalOffset") != generalSection.end()) {
            eventVerticalOffset = generalSection["EventVerticalOffset"];
        }

        // Check if the section "Debug"
        auto debugSection = parser.getSection("Debug");
        if (debugSection.find("ImagesFound") != debugSection.end()) {
            debugImagesFound = debugSection["ImagesFound"]==1;
        }
        if (debugSection.find("ImagesSearched") != debugSection.end()) {
            debugImagesSearched = debugSection["ImagesSearched"] == 1;
        }
        if (debugSection.find("EventFiles") != debugSection.end()) {
            debugEventFiles = debugSection["EventFiles"] == 1;
        }

        if (debugSection.find("3DFiles") != debugSection.end()) {
            debug3DFiles = debugSection["3DFiles"] == 1;
        }
    }
    catch (const std::runtime_error& e) {
        MessageBoxA(NULL, e.what(), "Error", MB_OK);
    }
}

// Function to check if a file exists
bool fileExists(const char* filename) {
    char originalFilename[512];

    strcpy(originalFilename, filename);
    for (char* p = originalFilename; *p; ++p) {
        if (*p == '/') {
            *p = '\\';
        }
    }

    std::ifstream file(originalFilename);
    return file.good();
}

uint32_t EncodeFormat(uint32_t baseFormat, uint32_t formatId) {
    return (baseFormat & 0xFFFF) | ((formatId & 0xFF) << 24);
}

void modFilenames(const char* originalFilename, char* filename1x, char* filename2x, char* filename2xDDS, char* filename4x, char* filename4xDDS) {
    // Define the "mods/" prefix
    const char* prefix = "mods/";
    size_t prefixLength = strlen(prefix);

    // Calculate the new lengths for the filenames with the prefix
    size_t originalLength = strlen(originalFilename);
    size_t totalLength = prefixLength + originalLength;

    // Create temporary buffers to hold the modified filenames
    char* modifiedFilename = new char[totalLength + 1];

    // Prepend "mods/" to the original filename
    strcpy(modifiedFilename, prefix);
    strcat(modifiedFilename, originalFilename);

    // Find the position of the file extension in the modified filename
    char* dotPos = strrchr(modifiedFilename, '.');
    if (dotPos != nullptr) {
        // Extract the filename and extension
        size_t baseLength = dotPos - modifiedFilename;

        // Copy the base filename and add the suffixes
        strncpy(filename1x, modifiedFilename, baseLength);
        strncpy(filename2x, modifiedFilename, baseLength);
        strncpy(filename2xDDS, modifiedFilename, baseLength);
        strncpy(filename4x, modifiedFilename, baseLength);
        strncpy(filename4xDDS, modifiedFilename, baseLength);

        // Null-terminate the strings after the base filename
        filename1x[baseLength] = '\0';
        filename2x[baseLength] = '\0';
        filename2xDDS[baseLength] = '\0';
        filename4x[baseLength] = '\0';
        filename4xDDS[baseLength] = '\0';

        // Append the suffixes and the original extension
        strcat(filename1x, dotPos);

        strcat(filename2x, "_2x");
        strcat(filename2x, dotPos);

        strcat(filename2xDDS, "_2x");
        strcat(filename2xDDS, ".dds");

        strcat(filename4x, "_4x");
        strcat(filename4x, dotPos);

        strcat(filename4xDDS, "_4x");
        strcat(filename4xDDS, ".dds");
    }

    // Clean up the temporary buffer
    delete[] modifiedFilename;
}

/*
// Global variables for keyboard adjustments
float g_uvAdjustX = 0.0f;
float g_uvAdjustY = 0.0f;
float g_screenAdjustX = 0.0f;
float g_screenAdjustY = 0.0f;

// Interactive adjustments
void ProcessKeyInput() {
    if (GetAsyncKeyState('T') & 0x8000) {
        g_uvAdjustY += 0.001f; // Adjust UV Y-coordinate up
    }
    if (GetAsyncKeyState('G') & 0x8000) {
        g_uvAdjustY -= 0.001f; // Adjust UV Y-coordinate down
    }
    if (GetAsyncKeyState('F') & 0x8000) {
        g_uvAdjustX -= 0.001f; // Adjust UV X-coordinate left
    }
    if (GetAsyncKeyState('H') & 0x8000) {
        g_uvAdjustX += 0.001f; // Adjust UV X-coordinate right
    }
    if (GetAsyncKeyState('I') & 0x8000) {
        g_screenAdjustY += 0.1f; // Adjust screen Y-coordinate up
        if (g_screenAdjustY > 500) g_screenAdjustY -= 500.0f;
    }
    if (GetAsyncKeyState('J') & 0x8000) {
        g_screenAdjustY -= 0.1f; // Adjust screen Y-coordinate down
        if (g_screenAdjustY < -500) g_screenAdjustY += 500.0f;
    }
    if (GetAsyncKeyState('K') & 0x8000) {
        g_screenAdjustX -= 0.1f; // Adjust screen X-coordinate left
        if (g_screenAdjustX < -500) g_screenAdjustX += 500.0f;
    }
    if (GetAsyncKeyState('L') & 0x8000) {
        g_screenAdjustX += 0.1f; // Adjust screen X-coordinate right
        if(g_screenAdjustX >500) g_screenAdjustX -= 500.0f;
    }
}
*/

////////////////////
// Hooked functions

typedef void (*LoadImageType)(int, astruct*, char*, int, int);
LoadImageType OriginalLoadImage = nullptr;

void HookedLoadImage(int formatType, astruct* imageDataPtr, char* filename, int width, int height) {
    char debugMessage[512];
    char newFilename[512];
    char filename1x[512];
    char filename2x[512];
    char filename2xDDS[512];
    char filename4x[512];
    char filename4xDDS[512];
    char originalFilename[512];

    //Check for modded .x files
    const char* prefix = "mods/";
    const size_t prefix_len = strlen(prefix);

    if (debugImagesSearched) {
        sprintf_s(debugMessage, "Searching for Modded Image: %s", filename);
        OutputDebugStringA(debugMessage);
    }

    // Check if filename starts with "mods/"
    if (strncmp(filename, prefix, prefix_len) == 0) {
        // Remove "mods/" by shifting the string left
        memmove(filename, filename + prefix_len, strlen(filename) - prefix_len + 1);
    }

    int scaleFlag = 0;

    int newwidth = width;
    int newheight = height;

    strcpy_s(originalFilename, filename);

    strcpy_s(lastFilename, filename);

    // Find the position of the file extension
    char* dotPos = strrchr(originalFilename, '.');
    if (dotPos != nullptr) {
        modFilenames(originalFilename, filename1x, filename2x, filename2xDDS, filename4x, filename4xDDS);

        if (fileExists(filename4xDDS)) {
            strcpy_s(newFilename, filename4xDDS);
            filename = newFilename;
            newwidth *= 4;
            newheight *= 4;
            scaleFlag = 4;
            if (debugImagesFound) {
                sprintf_s(debugMessage, "4HD DDS File found: %s (%i,%i,%i) - encoded from %i", filename, width, height, formatType, imageDataPtr->format);
                OutputDebugStringA(debugMessage);
            }
        } else if (fileExists(filename4x)) {
            strcpy_s(newFilename, filename4x);
            filename = newFilename;
            newwidth *= 4;
            newheight *= 4;
            scaleFlag = 4;
            if (debugImagesFound) {
                sprintf_s(debugMessage, "4HD File found: %s (%i,%i,%i) - encoded from %i", filename, width, height, formatType, imageDataPtr->format);
                OutputDebugStringA(debugMessage);
            }
        }
        else if (fileExists(filename2xDDS)) {
            strcpy_s(newFilename, filename2xDDS);
            filename = newFilename;
            newwidth *= 2;
            newheight *= 2;
            scaleFlag = 2;
            if (debugImagesFound) {
                sprintf_s(debugMessage, "2HD DDS File found: %s (%i,%i,%i) - encoded from %i", filename, width, height, formatType, imageDataPtr->format);
                OutputDebugStringA(debugMessage);
            }
        }
        else if (fileExists(filename2x)) {
            strcpy_s(newFilename, filename2x);
            filename = newFilename;
            newwidth *= 2;
            newheight *= 2;
            scaleFlag = 2;
            if (debugImagesFound) {
                sprintf_s(debugMessage, "2HD File found: %s (%i,%i,%i) - encoded from %i", filename, width, height, formatType, imageDataPtr->format);
                OutputDebugStringA(debugMessage);
            }
        }
        else if (fileExists(filename1x)) {
            strcpy_s(newFilename, filename1x);
            filename = newFilename;
            if (debugImagesFound) {
                sprintf_s(debugMessage, "Mod File found: %s (%i,%i,%i) - encoded from %i", filename, width, height, formatType, imageDataPtr->format);
                OutputDebugStringA(debugMessage);
            }
        }

        //Widescreen fixes for backgrounds, double resolution
        auto it = widescreenBackgrounds.find(originalFilename);
        if (it != widescreenBackgrounds.end()) {
            scaleFlag = it->second;
        }
    }

    // Update the image data pointer with width and height
    imageDataPtr->width = newwidth;
    imageDataPtr->height = newheight;

    OriginalLoadImage(formatType, imageDataPtr, filename, newwidth, newheight);

    imgPointers[imageDataPtr] = scaleFlag;
}

// Function pointer type for the Load3d function
typedef int(*Load3D_t)(int** param_1, char* param_2, int param_3);
Load3D_t originalLoad3D = nullptr;

// Hook function
int HookedLoad3D(int** param_1, char* param_2, int param_3) {
    // Buffer for the new path
    char mods_path[512];
    sprintf_s(mods_path, "mods/%s", param_2);

    char debugMessage[512];

    // Check if the file exists in the "mods/" directory
    if (fileExists(mods_path)) {
        if (debug3DFiles) {
            sprintf_s(debugMessage, "Found modded 3D: %s", param_2);
            OutputDebugStringA(debugMessage);
        }
        return originalLoad3D(param_1, mods_path, param_3);
    }
    else {
        if (debug3DFiles) {
            sprintf_s(debugMessage, "No modded 3D: %s", mods_path);
            OutputDebugStringA(debugMessage);
        }
        return originalLoad3D(param_1, param_2, param_3);
    }
}
// Function pointer type for the Load3dwithAnim function
typedef void*(__thiscall* Load3DwithAnim_t)(void* this_ptr, char* filepath, uint32_t param_2, uint32_t param_3);
Load3DwithAnim_t originalLoad3DwithAnim = nullptr;

void* __fastcall HookedLoad3DwithAnim(void* this_ptr, void* edx, char* filepath, uint32_t param_2, uint32_t param_3) {
    // Buffer for the new path
    char mods_path[512];
    sprintf_s(mods_path, "mods/%s", filepath);

    char debugMessage[512];

    // Check if the file exists in the "mods/" directory
    if (fileExists(mods_path)) {
        if (debug3DFiles) {
            sprintf_s(debugMessage, "Found modded 3D (Anim): %s", filepath);
            OutputDebugStringA(debugMessage);
        }
        return originalLoad3DwithAnim(this_ptr, mods_path, param_2, param_3);
    }
    else {
        if (debug3DFiles) {
            sprintf_s(debugMessage, "No modded 3D (Anim): %s", mods_path);
            OutputDebugStringA(debugMessage);
        }
        return originalLoad3DwithAnim(this_ptr, filepath, param_2, param_3);
    }
}

// DDS file header structure (simplified)
struct DDSHeader {
    char magic[4];
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    // Other fields are not relevant for our purpose
};

// Function to check if the file is a DDS file and extract dimensions
bool LoadDDSDimensions(const std::string& filePath, int& width, int& height) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return false;
    }

    DDSHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(DDSHeader));
    if (!file) {
        return false;
    }

    if (std::strncmp(header.magic, "DDS ", 4) != 0) {
        return false;
    }

    width = header.width;
    height = header.height;
    return true;
}

// Function pointer type for the original function
typedef void(*LoadxFileImages_t)(astruct* param_1, void* param_2);
LoadxFileImages_t originalLoadxFileImages = nullptr;

// Hook function, ignoring full FILE type by just taking first variable of it
void HookedLoadxFileImages(astruct* param_1, char* param_2) {

    char debugMessage[512];

    if (debugImagesSearched) {
        sprintf_s(debugMessage, "Searching for Modded Image (3D): %s", param_2);
        OutputDebugStringA(debugMessage);
    }

    const char* prefix = "mods/";
    const size_t prefix_len = strlen(prefix);

    // Check if filePath starts with "mods/"
    if (strncmp(param_2, prefix, prefix_len) == 0) {
        // Remove "mods/" by shifting the string left
        memmove(param_2, param_2 + prefix_len, strlen(param_2) - prefix_len + 1);
    }

    // String for the new path
    char mods_path[512];
    bool modthere = false;
    bool isDDS = false;

    //sprintf_s(debugMessage, "Mod 3D param_1: %i, %i, %i", param_1->width, param_1->height, param_1->format);
    //OutputDebugStringA(debugMessage);

    int formatType = 2;//2 = .bmp

    char originalFilename[512];
    char filename1x[512];
    char filename2x[512];
    char filename2xDDS[512];
    char filename4x[512];
    char filename4xDDS[512];
    strcpy_s(originalFilename, param_2);

    // Find the position of the file extension
    char* dotPos = strrchr(originalFilename, '.');

    // Check for .bmp or .tga extension
    if (dotPos != nullptr) {
        if (strcmp(dotPos, ".tga") == 0) {
            formatType = 12;
        }
    }

    if (dotPos != nullptr) {
        modFilenames(originalFilename, filename1x, filename2x, filename2xDDS, filename4x, filename4xDDS);

        if (fileExists(filename4xDDS)) {
            strcpy_s(mods_path, filename4xDDS);
            modthere = true;
            isDDS = true;
            if (debugImagesFound) {
                sprintf_s(debugMessage, "4HD DDS 3D image File found: %s", param_2);
                OutputDebugStringA(debugMessage);
            }
        }
        else if (fileExists(filename4x)) {
            strcpy_s(mods_path, filename4x);
            modthere = true;
            if (debugImagesFound) {
                sprintf_s(debugMessage, "4HD 3D image File found: %s", param_2);
                OutputDebugStringA(debugMessage);
            }
        }
        else if (fileExists(filename2xDDS)) {
            strcpy_s(mods_path, filename2xDDS);
            modthere = true;
            isDDS = true;
            if (debugImagesFound) {
                sprintf_s(debugMessage, "2HD 3D image DDS File found: %s", param_2);
                OutputDebugStringA(debugMessage);
            }
        }
        else if (fileExists(filename2x)) {
            strcpy_s(mods_path, filename2x);
            modthere = true;
            if (debugImagesFound) {
                sprintf_s(debugMessage, "2HD 3D image File found: %s", param_2);
                OutputDebugStringA(debugMessage);
            }
        }
        else if (fileExists(filename1x)) {
            strcpy_s(mods_path, filename1x);
            modthere = true;
            if (debugImagesFound) {
                sprintf_s(debugMessage, "3D image File found: %s", param_2);
                OutputDebugStringA(debugMessage);
            }
        }
    }

    if (modthere) {
        int width = 0;
        int height = 0;

        if (isDDS) {
            if (!LoadDDSDimensions(mods_path, width, height)) {
                OutputDebugStringA("Error: Unable to load DDS dimensions.");
                originalLoadxFileImages(param_1, param_2);
                return;
            }
        }
        else {
            std::ifstream file(mods_path, std::ios::binary);

            if (file) {
                std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

                if (buffer.size() >= 18) { // Ensure the buffer is large enough to contain necessary header information
                    if (formatType == 12) {
                        width = *(unsigned short*)(&buffer[12]);
                        height = *(unsigned short*)(&buffer[14]);
                    }
                    else {
                        width = *(unsigned int*)(&buffer[18]);
                        height = *(unsigned int*)(&buffer[22]);
                    }
                }
                else {
                    OutputDebugStringA("Error: Buffer too small to extract dimensions for modded texture");
                    originalLoadxFileImages(param_1, param_2);
                    return;
                }
            }
            else {
                OutputDebugStringA("Error: Unable to load file modded file");
                originalLoadxFileImages(param_1, param_2);
                return;
            }
        }

        loadingXFile = true;
        OriginalLoadImage(formatType, param_1, mods_path, width, height);
        return;
    }

    // Call the original function
    originalLoadxFileImages(param_1, param_2);
}

//Search map for current flag
bool GetFlagForImageData(astruct* imageDataPtr, int& flag) {
    auto it = imgPointers.find(imageDataPtr);
    if (it != imgPointers.end()) {
        flag = it->second;
        return true;
    }
    return false;
}

typedef void*(*OriginalUI_UV_Layout_Type)(void*, void*, void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
OriginalUI_UV_Layout_Type OriginalUI_UV_Layout = nullptr;

void UI_UV_Layout(void* param1, void* param2, void* param3,
    uint32_t param4, uint32_t param5, uint32_t param6,
    uint32_t param7, uint32_t param8, uint32_t param9) {

    // Call the key input processing function
    //ProcessKeyInput(); //This helps making adjustments interactive

    uvRect* screenRect = (uvRect*)param1;
    uvRect* uvr = (uvRect*)param2;

    imgData* imgDataPtr = ((imgData*)param3);

    int formatId = 0;
    
    if (GetFlagForImageData((astruct*)param3, formatId)) {

        if (formatId == 2) {
            uvr->x *= 2;
            uvr->y *= 2;
            uvr->w *= 2;
            uvr->h *= 2;
        }
        else if (formatId == 4) {
            uvr->x *= 4;
            uvr->y *= 4;
            uvr->w *= 4;
            uvr->h *= 4;
        }
        else if (formatId == 6) { // Widescreen background, 2x fix
            // Calculate the aspect ratio of the window
            float screenAspect = screenSize.w / screenSize.h;

            // Original image dimensions
            float imgWidth = 2048.0f;
            float imgHeight = 1024.0f;
            float imgAspect = imgWidth / imgHeight;

            // Adjust UV coordinates based on aspect ratio
            if (screenAspect < imgAspect) {
                // Window is wider than the image
                uvr->w = imgHeight * screenAspect; // Adjust width based on aspect ratio
                uvr->h = imgHeight;
                uvr->x = (imgWidth - uvr->w) / 2; // Center horizontally
                uvr->y = 0;
            }
            else {
                // Window is taller than the image
                uvr->w = imgWidth;
                uvr->h = imgWidth / screenAspect; // Adjust height based on aspect ratio
                uvr->x = 0;
                uvr->y = (imgHeight - uvr->h) / 2; // Center vertically
            }

            // Adjust the screen rectangle to fill the window
            screenRect->x = (640.0f * 2.0f - screenSize.w) / 4.0f;
            screenRect->y = (480.0f * 2.0f - screenSize.h) / 4.0f;
            screenRect->w = screenSize.w / 2.0f;
            screenRect->h = screenSize.h / 2.0f;
        }
        else if (formatId == 8) { // Widescreen stretched
            // Adjust the screen rectangle to fill the window
            screenRect->x = (640.0f * 2.0f - screenSize.w) / 4.0f;
            screenRect->y = (480.0f * 2.0f - screenSize.h) / 4.0f;
            screenRect->w = screenSize.w / 2.0f;
            screenRect->h = screenSize.h / 2.0f;
        }
        
    }

    // Call the original function
    OriginalUI_UV_Layout((void*)screenRect, (void*)uvr, (void*)imgDataPtr, param4, param5, param6, param7, param8, param9);
}

typedef void* (*GetWindowRect_0047b2e7_Type)(HWND, UINT, unsigned int, int*);
GetWindowRect_0047b2e7_Type Original_GetWindowRect = nullptr;

void Hooked_GetWindowRect_0047b2e7(HWND param_1, UINT param_2, unsigned int param_3, int* param_4) {
    //store window size
    ::GetWindowRect(param_1, &g_windowRect);

    screenSize = uvRect{};
    screenSize.x = 0;
    screenSize.y = 0;
    screenSize.w = (float)(g_windowRect.right - g_windowRect.left);
    screenSize.h = (float)(g_windowRect.bottom - g_windowRect.top);

    float ratio = screenSize.w / screenSize.h;
    vScreenSize = uvRect{};
    vScreenSize.x = 0;
    vScreenSize.y = 0;
    vScreenSize.w = ratio*480;
    vScreenSize.h = 480;

    // Call the original function
    Original_GetWindowRect(param_1, param_2, param_3, param_4);
}

//Messing around hook code
typedef int(__cdecl *Originalunknown_t)(char* param_1, char* param_2, int length);
Originalunknown_t Original_unknown = nullptr;

int Hooked_unknown(char* param_1, char* param_2, int length) {
    // Log the parameters
    char buffer[256];
    //snprintf(buffer, sizeof(buffer), "param1: x=%f, y=%f, w=%f, h=%f", param_1->x, param_1->y, param_1->w, param_1->h);
    //snprintf(buffer, sizeof(buffer), "param1: %i param2: %i", param_1, param_2);

    //OutputDebugStringA(buffer);
    /*
    snprintf(buffer, sizeof(buffer), "param2: x=%f, y=%f, w=%f, h=%f", param_2->x, param_2->y, param_2->w, param_2->h);
    OutputDebugStringA(buffer);
     */
    //let's mess with the screen
    //uvRect* screenRect = (uvRect*)param_1;
    //screenRect->w /= 2;
    //screenRect->h /= 2;

    
    //param_1 /= 2;
    //param_1->w /= 2;
    //param_1->h /= 2;
    //param_2 /= 2;
    //*(float*)(param_1 + 0x20) /= 2;
    //*(float*)(param_1 + 0x24) /= 2;

    // Call the original function
    //bool result = Original_unknown(param_1, param_2, length);

    size_t lenstr = strlen(param_1);
    if (lenstr > 0 && param_1[lenstr - 1] == ':') {
        snprintf(buffer, sizeof(buffer), "check starts with: %s, %s", param_1, param_2);
        OutputDebugStringA(buffer);
    }
    return Original_unknown(param_1, param_2, length);
}


/*
SafetyHookInline g_LoadImage_hook{};

// Hook function
void SHHookedLoadImage(int formatType, astruct* imageDataPtr, char* filename, int width, int height) {
    OutputDebugStringA("SH_LoadImage: HookedLoadImage entered");

    // Your hook logic here
    char buffer[256];
    sprintf(buffer, "FormatType: %d, Width: %d, Height: %d", formatType, width, height);
    OutputDebugStringA(buffer);

    g_LoadImage_hook.stdcall<void>(formatType, imageDataPtr, filename, width, height);

    OutputDebugStringA("SH_LoadImage: HookedLoadImage exiting");
}
*/

// Hook UVRect for screen drawing function prototype
void SHAdjustUVRect(safetyhook::Context& ctx, float x, float y, float width, float height, bool relative) {
    //OutputDebugStringA("SH_AdjustUVRect: Hooked function entered");
    // Calculate the addresses based on stack pointer (ESP) or other registers
    float* drawRectX = reinterpret_cast<float*>(ctx.ebp - 0x20); // drawRect.x
    float* drawRectY = reinterpret_cast<float*>(ctx.ebp - 0x1C); // drawRect.y
    float* drawRectW = reinterpret_cast<float*>(ctx.ebp - 0x18); // drawRect.w
    float* drawRectH = reinterpret_cast<float*>(ctx.ebp - 0x14); // drawRect.h

    if (relative) {
        if (x) {
            *drawRectX = (x/640)*vScreenSize.w;
        }
        else {
            *drawRectX = (*drawRectX / 640) * vScreenSize.w;
        }
        if (y) {
            *drawRectY = (y / 480) * vScreenSize.h;
        }
        else {
            *drawRectY = (*drawRectY / 480) * vScreenSize.h;
        }
    }
    else {
        // Modifying the values based on the provided parameters
        if (x) {
            *drawRectX = x;
        }
        if (y) {
            *drawRectY = y;
        }
    }

    if (width) {
        *drawRectW = width;
    }
    if (height) {
        *drawRectH = height;
    }
}

// Safety hooks
std::map<uintptr_t, SafetyHookMid> shMidHooks;

// Create the hook object
SafetyHookMid AdjustMarketTear_hook{};

void SH_Uninitialize() {
    for (auto& pair : shMidHooks) {
        pair.second.reset();
    }
    shMidHooks.clear();
}

using MidHookFn = void (*)(safetyhook::Context& ctx);
void SH_MidHookFactory(uintptr_t address, MidHookFn destination_fn) {

    SafetyHookMid hook = safetyhook::create_mid(reinterpret_cast<void*>(address), destination_fn);
    if (hook) {
    }
    else {
        OutputDebugStringA("SH_Hooked SafetyHook mid-function setup failed!");
    }

    shMidHooks[address] = std::move(hook);
}

void ChangeToLoadXImageSettings(safetyhook::Context& ctx) {
    // Fix call int LoadImage
    //     ProcessImageData(D3D_073dfcbc,imageBytes,tempResult,width,height,1,0,0,1,0xff,0xff,0,0,0,imageDataPtr);
    // to match
    //     ProcessImageData(D3D_073dfcbc,imageBuffer,local_10,width,height,0,0,0,1,0xff,0xff,0,0,0,param_1);
    if (loadingXFile) {
        int* flag = reinterpret_cast<int*>(ctx.esp + 0x18);
        *flag = 0;
    }
    loadingXFile = false;
}

bool ReadFileToBuffer(const char* filename, std::vector<char>& buffer) {
    //TODO - adjust lines
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    if (size == -1) return false;
    file.seekg(0, std::ios::beg);
    buffer.resize(size);
    file.read(buffer.data(), size);
    return true;
}

void AdjustIVTBufferHook(safetyhook::Context& ctx) {
    // Assuming `interprettedFilename` is at a specific offset from the frame pointer
    char* interprettedFilename = reinterpret_cast<char*>(ctx.ebp - 0x434); // Adjust offset if necessary

    if (debugEventFiles) {
        char buffer[512];
        sprintf_s(buffer, "Showing event: %s", interprettedFilename);
        OutputDebugStringA(buffer);
    }
    // Buffer for the new path (must be static to persist after function exits)
    static char mods_path[512];
    sprintf_s(mods_path, "mods/iv/i%s", interprettedFilename);

    // EBX holds the address of the readBuffer
    char* readBuffer = reinterpret_cast<char*>(ctx.ebx);

    std::vector<char> moddedBuffer;

    if (ReadFileToBuffer(mods_path, moddedBuffer)) {
        // Replace the content of readBuffer with the content of the modded file
        memcpy(readBuffer, moddedBuffer.data(), moddedBuffer.size());
        readBuffer[moddedBuffer.size()] = '\0'; // Null-terminate the buffer

        if (debugEventFiles) {
            OutputDebugStringA("Loaded modded event.");
        }
    }
    
    // Process the buffer to adjust move and moveto values
    std::string buffer(readBuffer);
    std::stringstream ss(buffer);
    std::string line;
    std::string result;

    std::regex moveRegex(R"(chr:(\d+):(move|moveto):(-?\d+),(\d+))");

    while (std::getline(ss, line)) {
        std::smatch match;
        if (std::regex_search(line, match, moveRegex)) {
            int chr = std::stoi(match[1].str());
            std::string command = match[2].str();
            int x = std::stoi(match[3].str());
            int y = std::stoi(match[4].str());

            if (eventOffscreenTestValue > 0) {
                if (x > eventOffscreenTestValue || x < eventOffscreenTestValue * -1) {
                    x += (x > 0) ? eventOffscreenAdjustment : (eventOffscreenAdjustment * -1);
                }
            }

            y += eventVerticalOffset;

            std::stringstream modifiedLine;
            modifiedLine << "chr:" << chr << ":" << command << ":" << x << "," << y;
            result += modifiedLine.str() + "\n";
        }
        else {
            result += line + "\n";
        }
    }

    // Copy the modified content back to readBuffer
    strcpy_s(readBuffer, result.size() + 1, result.c_str());
}

void AdjustMarketTear(safetyhook::Context& ctx) {
    if (letterBoxed) {
        SHAdjustUVRect(ctx, characterOffsets * -2, NULL, 512, 512, true);
    }
    else {
        SHAdjustUVRect(ctx, characterOffsets * -2, 32, 512, 512, true);
    }
}

// drawRect.x for Recette and Guildmaster
void AdjustMarketRecette(safetyhook::Context& ctx) {
    float* drawRectX = reinterpret_cast<float*>(ctx.ebp - 0x20); 
    if (letterBoxed) {
        SHAdjustUVRect(ctx, *drawRectX - (characterOffsets), NULL, 512, 512, true);
    }
    else {
        SHAdjustUVRect(ctx, *drawRectX - (characterOffsets), 32, 512, 512, true);
    }
}

void AdjustMarketBuyItemsWindow(safetyhook::Context& ctx) {
    float* drawRectX = reinterpret_cast<float*>(ctx.ebp - 0x34); // drawRect.x

    *drawRectX = ((*drawRectX-240) / 640) * vScreenSize.w+240;
}

void AdjustInitMarketWindow(safetyhook::Context& ctx) {
    float* drawRectX = reinterpret_cast<float*>(ctx.ebp - 0x48); // drawRect.x

    *drawRectX = ((*drawRectX - 256) / 640) * vScreenSize.w + 256;
}

void AdjustMarketWindow(safetyhook::Context& ctx) {
    float* drawRectX = reinterpret_cast<float*>(ctx.ebp - 0x34); // drawRect.x

    *drawRectX = ((*drawRectX - 32) / 640) * vScreenSize.w + 32;
}

void AdjustFusion(safetyhook::Context& ctx) {
    float* local_50 = reinterpret_cast<float*>(ctx.ebp - 0x4C); // offscreen x

    //char buffer[256];
    //sprintf(buffer, "local_50: %f", *local_50);
    //OutputDebugStringA(buffer);

    *local_50 = (*local_50/640)*vScreenSize.w;
}

void AdjustDungeonMap(safetyhook::Context& ctx) {
    float* drawW = reinterpret_cast<float*>(ctx.ebp - 0x10);
    float* drawX = reinterpret_cast<float*>(ctx.ebp - 0x28);
    float* uvW = reinterpret_cast<float*>(ctx.ebp - 0x20);

    // Original image dimensions
    float imgWidth = 750.0f;

    // Window is taller than the image
    *uvW = imgWidth;
    *drawX = (640.0f * 2.0f - screenSize.w) / 4.0f + (imgWidth-640);
    *drawW = imgWidth;
}

void AdjustDungeonChar(safetyhook::Context& ctx) {
    float* local_2c_u = reinterpret_cast<float*>(ctx.ebp - 0x28);
    float* local_2c_v = reinterpret_cast<float*>(ctx.ebp - 0x28 + 4);
    float* local_2c_uw = reinterpret_cast<float*>(ctx.ebp - 0x28 + 8);
    float* local_2c_vh = reinterpret_cast<float*>(ctx.ebp - 0x28 + 12);

    // Modify the local variables
    if (letterBoxed) {
        *local_2c_u = characterOffsets * -1;
    }
    else {
        *local_2c_v = -32;
        *local_2c_u = characterOffsets * -1;
        *local_2c_uw += 31;
        *local_2c_vh += 31;
    }
}

void AdjustEquipmentBack(safetyhook::Context& ctx) {
    short* local_8 = reinterpret_cast<short*>(ctx.ebp - 0x4);
    float* local_2c_u = reinterpret_cast<float*>(ctx.ebp - 0x28);
    float* local_2c_v = reinterpret_cast<float*>(ctx.ebp - 0x24);

    *local_2c_u -= characterOffsets*2;

    // Get the extra items to move offscreen
    // Setting local_8 didn't work
    if (*local_8 >= (short)472) {
    
        *local_2c_v += 500;
    }
    else {
        if (!letterBoxed) {
            *local_2c_v += 20;
        }
    }
}

void AdjustEquipmentInfo(safetyhook::Context& ctx) {
    float* valueY = reinterpret_cast<float*>(ctx.esp + 4);  // The second argument to UI_ItemWithText_0046add8
    float* valueX = reinterpret_cast<float*>(ctx.esp + 0);

    *valueX -= characterOffsets*2;
    // Get the extra items to move offscreen
    if (*valueY > 440) {
        *valueY += 500.0f;
    }
    else {
        if (!letterBoxed) {
            *valueY += 20;
        }
    }
}

void AdjustDungeonCharDescriptTxt(safetyhook::Context& ctx) {
    float* valueX = reinterpret_cast<float*>(ctx.esp + 0);
    *valueX -= characterOffsets * 2;
}

void AdjustDungeonCharPrice(safetyhook::Context& ctx) {
    float* valueX = reinterpret_cast<float*>(ctx.esp + 0);
    float* valueY = reinterpret_cast<float*>(ctx.esp + 4);
    *valueX -= characterOffsets * 2 + 32;
    *valueY += 2;

    if (!letterBoxed) {
        *valueY += 20;
    }
}

void AdjustDungeonCharPriceBack(safetyhook::Context& ctx) {
    float* valueU = reinterpret_cast<float*>(ctx.ebp - 0x18);
    //float* valueV = reinterpret_cast<float*>(ctx.esp + 14); //320
    float* valueUW = reinterpret_cast<float*>(ctx.ebp - 0x10);
    float* valueVH = reinterpret_cast<float*>(ctx.ebp - 0xC);
    float* valueX = reinterpret_cast<float*>(ctx.ebp - 0x28);
    float* valueY = reinterpret_cast<float*>(ctx.ebp - 0x24);
    float* valueW = reinterpret_cast<float*>(ctx.ebp - 0x20);
    float* valueH = reinterpret_cast<float*>(ctx.ebp - 0x1C);

    //New UV, expanded to include more obvious background
    *valueU = 244.0;
    *valueUW = 488.0;
    *valueVH = 365;

    *valueW = 488.0- 244.0;
    *valueH = 365 - 320;

    *valueX -= characterOffsets * 2 + 42;
    if (!letterBoxed) {
        *valueY += 20;
    }
}


void AdjustStartBGHook(safetyhook::Context& ctx) {
    float* width = reinterpret_cast<float*>(ctx.ebp - 0x34);
    float* height = reinterpret_cast<float*>(ctx.ebp - 0x30);
    float* x = reinterpret_cast<float*>(ctx.ebp - 0x3c);
    float* y = reinterpret_cast<float*>(ctx.ebp - 0x38);

    // Adjust the screen rectangle to fill the window
    *width = screenSize.w/2.0f;
    *height = screenSize.h / 2.0f;
    *x = (640.0f * 2.0f - screenSize.w) / 4.0f;
    *y = (480.0f * 2.0f - screenSize.h) / 4.0f;

    //Adjust UV ratio
    float* v = reinterpret_cast<float*>(ctx.ebp - 0x14);
    float* vheight = reinterpret_cast<float*>(ctx.ebp - 0x20);
    *vheight = *v + (640 * (screenSize.h / screenSize.w));
}

void AdjustStartBGFadeHook(safetyhook::Context& ctx) {
    float* width = reinterpret_cast<float*>(ctx.ebp - 0x34);
    float* height = reinterpret_cast<float*>(ctx.ebp - 0x30);
    float* x = reinterpret_cast<float*>(ctx.ebp - 0x3c);
    float* y = reinterpret_cast<float*>(ctx.ebp - 0x38);

    // Adjust the screen rectangle to fill the window
    *width = screenSize.w / 2.0f;
    *height = screenSize.h / 2.0f;
    *x = (640.0f * 2.0f - screenSize.w) / 4.0f;
    *y = (480.0f * 2.0f - screenSize.h) / 4.0f;
}

void AdjustStartLogo(safetyhook::Context& ctx) {
    float* x = reinterpret_cast<float*>(ctx.ebp - 0x3c);
    *x = ((*x-64) / 640) * vScreenSize.w+64;
}

void AdjustStartC(safetyhook::Context& ctx) {
    float* y = reinterpret_cast<float*>(ctx.ebp - 0x38);
    if (!letterBoxed) {
        *y += 28;
    }
}

void AdjustEventFade(safetyhook::Context& ctx) {
    float* screenRect_x = reinterpret_cast<float*>(ctx.ebp - 0x5c);
    float* screenRect_y = reinterpret_cast<float*>(ctx.ebp - 0x58);
    float* screenRect_w = reinterpret_cast<float*>(ctx.ebp - 0x54);
    float* screenRect_h = reinterpret_cast<float*>(ctx.ebp - 0x50);

    *screenRect_x = (640.0f * 2.0f - screenSize.w) / 4.0f;
    *screenRect_y = (480.0f * 2.0f - screenSize.h) / 4.0f;
    *screenRect_w = screenSize.w / 2.0f;
    *screenRect_h = screenSize.h / 2.0f;
}

void AdjustPauseChar(safetyhook::Context& ctx) {
    float* screenRect_w = reinterpret_cast<float*>(ctx.ebp - 0x1C);
    float* screenRect_h = reinterpret_cast<float*>(ctx.ebp - 0x18);

    if (!letterBoxed) {
        *screenRect_w += 32;
        *screenRect_h += 32;
    }
}

unsigned int pauseSubmenu = -1;

void AdjustPauseRecTe(safetyhook::Context& ctx) {
    float* screenRect_x = reinterpret_cast<float*>(ctx.ebp - 0x40);
    float* screenRect_y = reinterpret_cast<float*>(ctx.ebp - 0x3C);

    // Read the global variable
    int menuTransitionValue = *reinterpret_cast<int*>(0x074b2880);
    
    // Read the value of DAT_074b2878
    int index = *reinterpret_cast<int*>(0x074b2878);
    // Read the value from the array at DAT_074b2844 using the index
    int currentMenu = *reinterpret_cast<int*>(0x074b2844 + index * sizeof(int));

    if (pauseSubmenu != currentMenu) {
        //char buffer[256];
        //sprintf(buffer, "pauseSubmenu set to %i", currentMenu);
        //OutputDebugStringA(buffer);
        pauseSubmenu = currentMenu;
    }

    if (pauseSubmenu == 0 || pauseSubmenu == 1) {//no sub, items
        *screenRect_x += characterOffsets * 1.5 + (((float)menuTransitionValue) / 10 * -320);
    }
    else if (pauseSubmenu == 6 || pauseSubmenu == 3){//encyclopedia, save
        *screenRect_x += characterOffsets * 1.5;
        //alpha to 0
        unsigned int alphaFadeAmount = (unsigned int)menuTransitionValue * 0x1a;
        if (255 < alphaFadeAmount) {
            alphaFadeAmount = 255;
        }
        alphaFadeAmount = 255 - alphaFadeAmount;

        //set forth param
        unsigned int* color_param = reinterpret_cast<unsigned int*>(ctx.esp + 0xC);
        *color_param = alphaFadeAmount << 24 | 0x00ffffff;
    }
    else if (pauseSubmenu == 2) {//options
        float perc = (float)menuTransitionValue / 10.0;
        *screenRect_x += characterOffsets * 1.5 + (perc * (-320 - characterOffsets));
    }
    else {
        *screenRect_x += characterOffsets * 1.5;
    }

    if (!letterBoxed) {
        *screenRect_y += 32;
    }
}

void AdjustPauseMainMenu(safetyhook::Context& ctx) {
    float* screenRect_x = reinterpret_cast<float*>(ctx.ebp - 0x40);

    float amount = characterOffsets * 4;
    if (*screenRect_x <= 432) {
        amount = (*screenRect_x/432)*(characterOffsets * 4);
    }


    *screenRect_x += amount;

}

void AdjustPauseMenuTitle(safetyhook::Context& ctx) {
    float* screenRect_x = reinterpret_cast<float*>(ctx.ebp - 0x40);
    float* screenRect_y = reinterpret_cast<float*>(ctx.ebp - 0x3C);

    *screenRect_x -= 368 + 128 + (characterOffsets/2);
    if (!letterBoxed) {
        *screenRect_y += 38;
    }
}

void AdjustPauseMenuCal(safetyhook::Context& ctx) {
    float* local_8 = reinterpret_cast<float*>(ctx.ebp - 0x4);

    *local_8 += (640.0f * 2.0f - screenSize.w) / 4.0f;
}

void AdjustButton3Prompt(safetyhook::Context& ctx) {
    float* screenRect_h = reinterpret_cast<float*>(ctx.ebp - 0x30);

    if (!letterBoxed) {
        *screenRect_h += 32;
    }
}

// Function to save the buffer as BMP (example implementation)
void SaveBufferAsBMP(const char* filename, const void* buffer, int width, int height, int bpp) {
    // BMP file header
    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER infoHeader = {};

    fileHeader.bfType = 0x4D42; // 'BM'
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (width * height * (bpp / 8));
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = bpp;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = width * height * (bpp / 8);

    std::ofstream ofs(filename, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    ofs.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    ofs.write(reinterpret_cast<const char*>(buffer), infoHeader.biSizeImage);
    ofs.close();
}

void BeforeImageRelease(safetyhook::Context& ctx) {
    // Extract necessary parameters from the stack frame
    //char* filename = reinterpret_cast<char*>(ctx.ebp + 0x34);
    int width = *reinterpret_cast<int*>(ctx.ebp + 0x14);
    int height = *reinterpret_cast<int*>(ctx.ebp + 0x18);
    void* imageBytes = *reinterpret_cast<void**>(ctx.esp + 0);

    // Parameter search for string
    /*for (int offset = 0x10; offset < 0x50; offset += 0x4) {
        char* possibleFilename = reinterpret_cast<char*>(ctx.ebp + offset);
        if (IsBadReadPtr(possibleFilename, 1)) {
            continue;
        }
        if (strlen(possibleFilename) > 0 && strlen(possibleFilename) < 256) {
            char buffer[256];
            sprintf(buffer, "%s at (%X)", possibleFilename, offset);
            OutputDebugStringA(buffer);
        }
    }*/

    // Define the special filename to check
    const char* specialFilename = "bmp/chr/chr00.bmp";

    if (strlen(lastFilename) > 0) {
        //char buffer[256];
        //sprintf(buffer, "Clearing img buffer: %s (%i,%i)", lastFilename, width, height);
        //OutputDebugStringA(buffer);

        // Check if the filename matches the special filename
        if (strcmp(lastFilename, specialFilename) == 0) {
            // Save the buffer as BMP
            SaveBufferAsBMP("output.bmp", imageBytes, width, height, 24); // Assuming 32 bpp
            OutputDebugStringA("Saved to output.bmp");
        }

        strcpy_s(lastFilename, "");
    }
}

void CreateMinHooks() {

    //Start MinHook
    if (MH_Initialize() != MH_OK) {
        OutputDebugStringA("MH_Initialize failed");
        return;
    }

    //////////////////////////
    // Create hooks

    // Create the Load Image hook
    if (MH_CreateHook((LPVOID)0x0047193C, &HookedLoadImage, reinterpret_cast<LPVOID*>(&OriginalLoadImage)) != MH_OK) {
        OutputDebugStringA("MH_CreateHook HD and redirect image failed");
        return;
    }

    if (MH_CreateHook((LPVOID)0x00472836, &HookedLoad3D, reinterpret_cast<LPVOID*>(&originalLoad3D)) != MH_OK) {
        OutputDebugStringA("MH_CreateHook 3D redirect failed");
        return;
    }

    if (MH_CreateHook((LPVOID)0x00403b6e, &HookedLoad3DwithAnim, reinterpret_cast<LPVOID*>(&originalLoad3DwithAnim)) != MH_OK) {
        OutputDebugStringA("MH_CreateHook 3D (Animated) redirect failed");
        return;
    }

    if (MH_CreateHook((LPVOID)0x00471b24, &HookedLoadxFileImages, reinterpret_cast<LPVOID*>(&originalLoadxFileImages)) != MH_OK) {
        OutputDebugStringA("MH_CreateHook 3D image redirect failed");
        return;
    }

    if (MH_CreateHook((LPVOID)0x00404efc, &UI_UV_Layout, reinterpret_cast<LPVOID*>(&OriginalUI_UV_Layout)) != MH_OK) {
        OutputDebugStringA("MH_CreateHook UVTrianglesHook failed");
        return;
    }

    // Create the hook for get window function
    if (MH_CreateHook((LPVOID)0x0047b2e7, &Hooked_GetWindowRect_0047b2e7, reinterpret_cast<LPVOID*>(&Original_GetWindowRect)) != MH_OK) {
        OutputDebugStringA("MH_CreateHook Window Rect failed");
        return;
    }

    /////////////////////////////////////////
    // Activate all the hooks
    if (MH_EnableHook((LPVOID)0x0047193C) != MH_OK) {
        OutputDebugStringA("MH_EnableHook 3D redirect failed");
        return;
    }

    if (MH_EnableHook((LPVOID)0x00403b6e) != MH_OK) {
        OutputDebugStringA("MH_EnableHook 3D (Animated) redirect failed");
        return;
    }

    if (MH_EnableHook((LPVOID)0x00472836) != MH_OK) {
        OutputDebugStringA("MH_EnableHook HD image failed");
        return;
    }

    if (MH_EnableHook((LPVOID)0x00471b24) != MH_OK) {
        OutputDebugStringA("MH_EnableHook 3D image redirect failed");
        return;
    }

    if (MH_EnableHook((LPVOID)0x00404efc) != MH_OK) {
        OutputDebugStringA("MH_EnableHook UVTrianglesHook failed");
        return;
    }

    if (MH_EnableHook((LPVOID)0x0047b2e7) != MH_OK) {
        OutputDebugStringA("MH_EnableHook Window Rect failed");
        return;
    }

    // hook 0x004797af (char, int, int, int) = no output?
    // remove DebugLog function
    //if (MH_EnableHook((LPVOID)0x004797af) != MH_OK) {
    //    OutputDebugStringA("MH_EnableHook DebugLog failed");
    //    return;
    //}

    OutputDebugStringA("Hooked modded files redirect successfully");
}

extern "C" __declspec(dllexport) void Init() {
    //Get configuration file
    LoadConfiguration();

    CreateMinHooks();

    /////////////////////////////////////////
    // SafetyHooks for midfunction hooks
    //
    
    // Fix LoadImage hook
    SH_MidHookFactory(0x00471ab2, ChangeToLoadXImageSettings);

    // Start screen
    SH_MidHookFactory(0x0049c84e, AdjustStartLogo);
    SH_MidHookFactory(0x0049c7d8, AdjustStartC);
    SH_MidHookFactory(0x0049c6fb, AdjustStartBGHook);
    SH_MidHookFactory(0x0049c763, AdjustStartBGFadeHook);

    // Create the mid-function hook, this address is before the 'finaliseuv' function
    SH_MidHookFactory(0x00494bc5, AdjustMarketTear);
    SH_MidHookFactory(0x00494c68, AdjustMarketRecette);
    SH_MidHookFactory(0x0046b0cb, AdjustMarketBuyItemsWindow);//Buy sell
    SH_MidHookFactory(0x0046bd18, AdjustMarketWindow);//
    SH_MidHookFactory(0x004940fe, AdjustInitMarketWindow);//The first market window
    //SH_MidHookFactory(0x00493749, AdjustFusionTitleAndText);
    SH_MidHookFactory(0x00493683, AdjustFusion);//The offset for fusion menu

    SH_MidHookFactory(0x0046be31, AdjustButton3Prompt);

    //Adventurers Guild
    SH_MidHookFactory(0x0045d617, AdjustDungeonMap);
    SH_MidHookFactory(0x0045cda4, AdjustDungeonChar);
    SH_MidHookFactory(0x0045cef1, AdjustEquipmentBack);
    SH_MidHookFactory(0x0045d175, AdjustEquipmentInfo);
    SH_MidHookFactory(0x0045cf6b, AdjustDungeonCharPriceBack);
    SH_MidHookFactory(0x0045d019, AdjustDungeonCharPrice);
    SH_MidHookFactory(0x0045d054, AdjustDungeonCharDescriptTxt);
    SH_MidHookFactory(0x0045d091, AdjustDungeonCharDescriptTxt);
    SH_MidHookFactory(0x0045d0ce, AdjustDungeonCharDescriptTxt);
    SH_MidHookFactory(0x0045d10b, AdjustDungeonCharDescriptTxt);

    //Event render and files
    SH_MidHookFactory(0x0046de8b, AdjustIVTBufferHook);
    SH_MidHookFactory(0x0046d7da, AdjustEventFade);
    SH_MidHookFactory(0x0046ce66, AdjustEventFade);

    //Pause menus
    SH_MidHookFactory(0x00480c44, AdjustPauseChar);
    SH_MidHookFactory(0x0048216a, AdjustPauseRecTe);
    SH_MidHookFactory(0x00482368, AdjustPauseMainMenu);
    SH_MidHookFactory(0x004823ff, AdjustPauseMainMenu);
    SH_MidHookFactory(0x0048248f, AdjustPauseMenuTitle);
    SH_MidHookFactory(0x004824c8, AdjustPauseMenuCal);

    //Image stuff
    //SH_MidHookFactory(0x00471b19, BeforeImageRelease);

    // Swap to Safety Hook only...?
    // Address of the LoadImage function
    // Create the inline hook
    //g_LoadImage_hook = safetyhook::create_inline(reinterpret_cast<void*>(0x0047193C), reinterpret_cast<void*>(SHHookedLoadImage));
    //if (!g_LoadImage_hook) {
    //    OutputDebugStringA("SH_Hooked Failed to create SafetyHook!");
    //    return;
    //}
    //OutputDebugStringA("SH_Hooked SafetyHook successfully set up!");

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        OutputDebugStringA("DLL_PROCESS_ATTACH");
        Init();
        break;
    case DLL_PROCESS_DETACH:
        MH_Uninitialize();

        SH_Uninitialize();
        break;
    }
    return TRUE;
}
