// ===================================================
// test_image_to_platform.cpp
//   Standalone verification for the image -> platform grid converter.
//   Loads resource/youkoso.png via DirectXTex (WIC, no D3D device needed),
//   runs ConvertImageToPlatforms, and prints stats + a coverage map so the
//   conversion logic can be validated without launching the game.
//
//   Build (from a "x64 Native Tools" / vcvars64 shell):
//     cl /EHsc /std:c++17 test_image_to_platform.cpp ^
//        DirectXTex_Release.lib ole32.lib windowscodecs.lib
// ===================================================

#include <Windows.h>
#include <cstdio>
#include "DirectXTex.h"
#include "ImageToPlatform.h"

using namespace DirectX;

int main(int argc, char** argv)
{
	const wchar_t* kPath = L"resource\\youkoso.png";

	// Optional CLI overrides: test_i2p.exe [cellSize] [fillThreshold]
	const int   argCell = (argc > 1) ? atoi(argv[1]) : 64;
	const float argFill = (argc > 2) ? (float)atof(argv[2]) : 0.20f;

	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		printf("CoInitializeEx failed: 0x%08lX\n", (unsigned long)hr);
		return 1;
	}

	TexMetadata meta;
	ScratchImage loaded;
	hr = LoadFromWICFile(kPath, WIC_FLAGS_NONE, &meta, loaded);
	if (FAILED(hr))
	{
		printf("LoadFromWICFile failed: 0x%08lX (run from the project dir)\n",
			(unsigned long)hr);
		CoUninitialize();
		return 1;
	}

	// Normalize to straight RGBA8 so the converter sees a known layout.
	ScratchImage rgbaImg;
	if (meta.format != DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		hr = Convert(*loaded.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM,
			TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, rgbaImg);
		if (FAILED(hr))
		{
			printf("Convert to RGBA8 failed: 0x%08lX\n", (unsigned long)hr);
			CoUninitialize();
			return 1;
		}
	}
	else
	{
		rgbaImg = std::move(loaded);
	}

	const Image* img = rgbaImg.GetImage(0, 0, 0);
	printf("Loaded %ls : %zux%zu, rowPitch=%zu\n",
		kPath, img->width, img->height, img->rowPitch);

	ImageToPlatformConfig cfg;
	cfg.cellSize = argCell;     // grid cell size; each cell -> at most one platform
	cfg.fillThreshold = argFill;// fraction of ink in a cell to make it solid
	// Keep world == image pixels for this verification (identity transform).

	std::vector<PlatformRect> rects =
		ConvertImageToPlatforms(img->pixels, (int)img->width, (int)img->height,
			(int)img->rowPitch, cfg);

	const int cols = ((int)img->width + cfg.cellSize - 1) / cfg.cellSize;
	const int rows = ((int)img->height + cfg.cellSize - 1) / cfg.cellSize;

	printf("\nGrid: %d cols x %d rows (cell=%dpx, threshold=%.0f%%)\n",
		cols, rows, cfg.cellSize, cfg.fillThreshold * 100.0f);
	printf("Generated platforms: %zu / %d cells\n\n",
		rects.size(), cols * rows);

	// ASCII coverage map: '#' = a generated platform cell, '.' = empty.
	std::vector<char> map((size_t)cols * rows, '.');
	float minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f;
	for (const PlatformRect& r : rects)
	{
		map[(size_t)r.cellRow * cols + r.cellCol] = '#';
		if (r.x < minX) minX = r.x;
		if (r.y < minY) minY = r.y;
		if (r.x + r.width > maxX) maxX = r.x + r.width;
		if (r.y + r.height > maxY) maxY = r.y + r.height;
	}

	for (int rr = 0; rr < rows; ++rr)
	{
		fputc(' ', stdout);
		for (int cc = 0; cc < cols; ++cc) fputc(map[(size_t)rr * cols + cc], stdout);
		fputc('\n', stdout);
	}

	if (!rects.empty())
	{
		printf("\nWorld-space bounds of generated platforms:\n");
		printf("  x: [%.1f .. %.1f]   y: [%.1f .. %.1f]\n",
			minX, maxX, minY, maxY);
		printf("\nFirst few platform rects (x, y, w, h):\n");
		const size_t n = rects.size() < 8 ? rects.size() : 8;
		for (size_t i = 0; i < n; ++i)
		{
			const PlatformRect& r = rects[i];
			printf("  [%zu] (%.1f, %.1f, %.1f, %.1f)  cell(c%d,r%d)\n",
				i, r.x, r.y, r.width, r.height, r.cellCol, r.cellRow);
		}
	}

	CoUninitialize();
	return 0;
}
