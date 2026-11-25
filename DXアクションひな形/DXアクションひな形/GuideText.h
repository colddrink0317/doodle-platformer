#pragma once

// ===================================================
// GuideText.h
//   Lightweight world-space text used to guide the player.
//   Rendered with the shared ASCII font atlas (consolab_ascii_512.png).
//   This is a placeholder that will later be replaced by hand-drawn images.
// ===================================================

#include "Main.h"

#include <string>

class GuideText
{
private:
	XMFLOAT2 m_Position;
	float    m_CharWidth;
	float    m_CharHeight;
	std::string m_Text;
	ID3D11ShaderResourceView* m_FontTexture;

public:
	GuideText(float x, float y, float charW, float charH,
		const char* text,
		ID3D11ShaderResourceView* fontTexture);

	void Render() const;
};
