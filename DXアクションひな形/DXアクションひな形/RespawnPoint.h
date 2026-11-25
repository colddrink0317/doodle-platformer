#pragma once

// ===================================================
// RespawnPoint.h
//   Built-in respawn marker for a level.
//   Pure data: it is NOT a physical entity the player can
//   touch and it is NOT rendered. It only describes the
//   position the player should be teleported to when they
//   fall off this level.
// ===================================================

class RespawnPoint
{
private:
	float m_SpawnX;
	float m_SpawnY;

public:
	RespawnPoint(float spawnX, float spawnY)
		: m_SpawnX(spawnX), m_SpawnY(spawnY) {
	}

	float GetSpawnX() const { return m_SpawnX; }
	float GetSpawnY() const { return m_SpawnY; }

	void GetSpawnPosition(float& x, float& y) const
	{
		x = m_SpawnX;
		y = m_SpawnY;
	}

	void SetSpawnPosition(float x, float y)
	{
		m_SpawnX = x;
		m_SpawnY = y;
	}
};
