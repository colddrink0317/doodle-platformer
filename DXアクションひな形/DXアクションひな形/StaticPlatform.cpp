// ===================================================
// StaticPlatform.cpp
// ===================================================

#include "Main.h"
#include "Renderer.h"
#include "Sprite.h"
#include "StaticPlatform.h"

StaticPlatform::StaticPlatform(float x, float y, float w, float h,
	ID3D11ShaderResourceView* texture,
	XMFLOAT2 texCoordPos,
	XMFLOAT2 texCoordScale,
	bool useTexCoord)
	: Platform(x, y, w, h)
	, m_TexCoordPos(texCoordPos)
	, m_TexCoordScale(texCoordScale)
	, m_UseTexCoord(useTexCoord)
{
	m_Texture = texture;
}

void StaticPlatform::Render() const
{
	// BoundingBox uses top-left origin, while Sprite uses center origin.
	XMFLOAT2 center = { m_Collider.GetCenterX(), m_Collider.GetCenterY() };
	XMFLOAT2 size = { m_Collider.width, m_Collider.height };

	if (m_UseTexCoord)
	{
		DrawSpriteTexCoord(true, m_Texture, center, size, m_TexCoordPos, m_TexCoordScale);
	}
	else
	{
		DrawSprite(true, m_Texture, center, size);
	}
}
