#pragma once

// ===================================================
// ImageToPlatform.h
//   Converts a source image (RGBA8 pixels) into a set of axis-aligned
//   platform rectangles using grid segmentation: the image is divided
//   into fixed-size cells, and any cell whose "ink" coverage exceeds a
//   threshold becomes one rectangular platform.
//
//   This module is intentionally free of any engine / DirectX includes
//   so the conversion logic can be unit-tested standalone. Callers map
//   the resulting rectangles onto whatever collider type they use
//   (e.g. StaticPlatform / BoundingBox).
// ===================================================

#include <vector>
#include <cstdint>

// Lightweight rectangle in world space (top-left origin), matching the
// layout of the engine's BoundingBox so callers can copy fields directly.
struct PlatformRect
{
	float x;
	float y;
	float width;
	float height;

	// Source grid cell this rect came from (handy for logging / debugging).
	int   cellCol;
	int   cellRow;
};

struct ImageToPlatformConfig
{
	// Grid cell size in source pixels. Larger = fewer, coarser platforms.
	int cellSize = 32;

	// Fraction of "ink" pixels within a cell required to treat it as solid.
	float fillThreshold = 0.25f;

	// A pixel is visible when its alpha is strictly above this value.
	uint8_t alphaThreshold = 128;

	// A visible pixel brighter than this on every channel is treated as
	// background (e.g. a white canvas) and does NOT count as ink.
	uint8_t whiteCutoff = 230;

	// Placement transform: world = imagePixel * worldScale + worldOffset.
	float worldScaleX = 1.0f;
	float worldScaleY = 1.0f;
	float worldOffsetX = 0.0f;
	float worldOffsetY = 0.0f;
};

// Decides whether a single RGBA8 pixel counts as "ink" (drawn content).
inline bool IsInkPixel(const uint8_t* px, const ImageToPlatformConfig& cfg)
{
	const uint8_t r = px[0];
	const uint8_t g = px[1];
	const uint8_t b = px[2];
	const uint8_t a = px[3];

	if (a <= cfg.alphaThreshold) return false;            // transparent
	if (r >= cfg.whiteCutoff &&
		g >= cfg.whiteCutoff &&
		b >= cfg.whiteCutoff) return false;               // background white
	return true;
}

// Converts RGBA8 image data into grid-aligned platform rectangles.
//   rgba     : pointer to top row of R8G8B8A8 pixel data
//   width    : image width in pixels
//   height   : image height in pixels
//   rowPitch : bytes per row (>= width * 4; honors padding)
//   cfg      : segmentation + placement parameters
inline std::vector<PlatformRect> ConvertImageToPlatforms(
	const uint8_t* rgba, int width, int height, int rowPitch,
	const ImageToPlatformConfig& cfg)
{
	std::vector<PlatformRect> rects;
	if (!rgba || width <= 0 || height <= 0 || cfg.cellSize <= 0) return rects;

	const int cell = cfg.cellSize;
	const int cols = (width + cell - 1) / cell;
	const int rows = (height + cell - 1) / cell;

	for (int cr = 0; cr < rows; ++cr)
	{
		const int y0 = cr * cell;
		const int y1 = (y0 + cell < height) ? y0 + cell : height;

		for (int cc = 0; cc < cols; ++cc)
		{
			const int x0 = cc * cell;
			const int x1 = (x0 + cell < width) ? x0 + cell : width;

			int inkCount = 0;
			int total = 0;
			for (int py = y0; py < y1; ++py)
			{
				const uint8_t* row = rgba + (size_t)py * rowPitch;
				for (int px = x0; px < x1; ++px)
				{
					if (IsInkPixel(row + (size_t)px * 4, cfg)) ++inkCount;
					++total;
				}
			}

			if (total > 0 &&
				(float)inkCount / (float)total >= cfg.fillThreshold)
			{
				PlatformRect rect;
				rect.x = x0 * cfg.worldScaleX + cfg.worldOffsetX;
				rect.y = y0 * cfg.worldScaleY + cfg.worldOffsetY;
				rect.width = cell * cfg.worldScaleX;
				rect.height = cell * cfg.worldScaleY;
				rect.cellCol = cc;
				rect.cellRow = cr;
				rects.push_back(rect);
			}
		}
	}

	return rects;
}
