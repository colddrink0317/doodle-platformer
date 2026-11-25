// ===================================================
// GuideText.cpp
// ===================================================

#include "Main.h"
#include "Sprite.h"
#include "GuideText.h"

GuideText::GuideText(float x, float y, float charW, float charH,
	const char* text,
	ID3D11ShaderResourceView* fontTexture)
	: m_Position{ x, y }
	, m_CharWidth(charW)
	, m_CharHeight(charH)
	, m_Text(text ? text : "")
	, m_FontTexture(fontTexture)
{
}

void GuideText::Render() const
{
	if (!m_FontTexture || m_Text.empty()) return;

	constexpr int   kGrid = 16;
	constexpr int   kAsciiBase = 32;
	constexpr float kCellUV = 1.0f / kGrid;

	const int len = static_cast<int>(m_Text.size());
	for (int i = 0; i < len; ++i)
	{
		const unsigned char ch = static_cast<unsigned char>(m_Text[i]);
		if (ch < kAsciiBase) continue;

		const int index = ch - kAsciiBase;
		const int col = index % kGrid;
		const int row = index / kGrid;

		const XMFLOAT2 texPos = { col * kCellUV, row * kCellUV };
		const XMFLOAT2 texScale = { kCellUV, kCellUV };
		const XMFLOAT2 center = { m_Position.x + (i + 0.5f) * m_CharWidth,
								  m_Position.y + m_CharHeight * 0.5f };
		const XMFLOAT2 size = { m_CharWidth, m_CharHeight };

		DrawSpriteTexCoord(true, m_FontTexture, center, size, texPos, texScale);
	}
}
