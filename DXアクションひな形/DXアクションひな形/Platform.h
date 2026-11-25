#pragma once

// ===================================================
// Platform.h
//   Base platform class and AABB structure defined by prototype.md.
// ===================================================

#include "Main.h"

// AABB bounds using top-left (x, y) plus width and height.
struct BoundingBox
{
	float x;
	float y;
	float width;
	float height;

	bool Intersects(const BoundingBox& other) const
	{
		return (x < other.x + other.width && x + width > other.x &&
				y < other.y + other.height && y + height > other.y);
	}

	float GetCenterX() const { return x + width * 0.5f; }
	float GetCenterY() const { return y + height * 0.5f; }
};


// Common base for static and dynamic map objects.
class Platform
{
protected:
	BoundingBox m_Collider;
	ID3D11ShaderResourceView* m_Texture;

public:
	Platform(float x, float y, float w, float h)
		: m_Texture(nullptr)
	{
		m_Collider = { x, y, w, h };
	}

	virtual ~Platform() = default;

	virtual void Update(float deltaTime) { (void)deltaTime; }
	virtual void Render() const = 0;

	const BoundingBox& GetCollider() const { return m_Collider; }

	// Used by the in-game debug platform editor to reposition / resize a
	// platform at runtime. Static and collision platforms both render and
	// collide straight from m_Collider, so this updates visuals too.
	void SetCollider(const BoundingBox& box) { m_Collider = box; }
};
