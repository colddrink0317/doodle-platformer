#pragma once

// ===================================================
// CollisionPlatform.h
//   A collision-only platform: it owns an AABB collider that the player
//   can stand on, but renders nothing. Used to give a separately-rendered
//   image (e.g. a hand-drawn sprite) pixel-aligned standing collision
//   without drawing any block tiles of its own.
// ===================================================

#include "Platform.h"

class CollisionPlatform : public Platform
{
public:
	CollisionPlatform(float x, float y, float w, float h)
		: Platform(x, y, w, h)
	{
	}

	// Intentionally draws nothing; the visual is the source image sprite.
	void Render() const override {}
};
