#pragma once

// ===================================================
// StaticPlatform.h
//   Static platform. Shares an externally loaded texture pointer,
//   with lifetime managed by LevelManager.
// ===================================================

#include "Platform.h"

class StaticPlatform : public Platform
{
private:
	XMFLOAT2 m_TexCoordPos;
	XMFLOAT2 m_TexCoordScale;
	bool m_UseTexCoord;

public:
	StaticPlatform(float x, float y, float w, float h,
		ID3D11ShaderResourceView* texture,
		XMFLOAT2 texCoordPos = { 0.0f, 0.0f },
		XMFLOAT2 texCoordScale = { 1.0f, 1.0f },
		bool useTexCoord = true);

	void Render() const override;
};
