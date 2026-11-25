// ===================================================
// Checkpoint.cpp
// ===================================================

#include "Main.h"
#include "Sprite.h"
#include "Checkpoint.h"

Checkpoint::Checkpoint(float x, float y, float w, float h,
	int nextLevel, float spawnX, float spawnY,
	ID3D11ShaderResourceView* texture)
	: m_TargetLevelID(nextLevel)
	, m_SpawnTargetX(spawnX)
	, m_SpawnTargetY(spawnY)
	, m_Texture(texture)
{
	m_TriggerArea = { x, y, w, h };
}

bool Checkpoint::CheckTrigger(const PlayerObject& player) const
{
	return m_TriggerArea.Intersects(player.GetCollider());
}

void Checkpoint::Render() const
{
	static const char kText[4] = { 'N', 'E', 'X', 'T' };
	constexpr int   kGrid = 16;
	constexpr int   kAsciiBase = 32;
	constexpr float kCellUV = 1.0f / kGrid;

	const float charW = m_TriggerArea.width / 4.0f;
	const float charH = m_TriggerArea.height;

	for (int i = 0; i < 4; ++i)
	{
		const int index = static_cast<unsigned char>(kText[i]) - kAsciiBase;
		const int col = index % kGrid;
		const int row = index / kGrid;

		const XMFLOAT2 texPos = { col * kCellUV, row * kCellUV };
		const XMFLOAT2 texScale = { kCellUV, kCellUV };
		const XMFLOAT2 center = { m_TriggerArea.x + (i + 0.5f) * charW,
								  m_TriggerArea.y + charH * 0.5f };
		const XMFLOAT2 size = { charW, charH };

		DrawSpriteTexCoord(true, m_Texture, center, size, texPos, texScale);
	}
}

void Checkpoint::GetSpawnPosition(float& x, float& y) const
{
	x = m_SpawnTargetX;
	y = m_SpawnTargetY;
}
