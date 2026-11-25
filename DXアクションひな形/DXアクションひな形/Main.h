#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#define NOMINMAX
#include <windows.h>
#include <assert.h>
#include <functional>

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")


#include <DirectXMath.h>
using namespace DirectX;



#include "DirectXTex.h"

#if _DEBUG
#pragma comment(lib, "DirectXTex_Debug.lib")
#else
#pragma comment(lib, "DirectXTex_Release.lib")
#endif



#pragma comment (lib, "winmm.lib")


#define SCREEN_WIDTH	(1920)
#define SCREEN_HEIGHT	(1080)

#define BLOCK_COLS		(6)	// Number of block stack columns.
#define BLOCK_ROWS		(13)// Number of block stack rows.

#define PIECE_WIDTH		(50)// Block size.
#define PIECE_HEIGHT	(50)

HWND GetWindow();
