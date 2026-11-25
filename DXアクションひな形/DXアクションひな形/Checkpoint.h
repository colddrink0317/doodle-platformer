#pragma once

// ===================================================
// Checkpoint.h
//   Notifies LevelManager to switch to the target level after triggering.
// ===================================================

#include "Platform.h"
#include "PlayerObject.h"

class Checkpoint
{
private:
	BoundingBox m_TriggerArea;
	int m_TargetLevelID;
	float m_SpawnTargetX;
	float m_SpawnTargetY;
	ID3D11ShaderResourceView* m_Texture;

public:
	Checkpoint(float x, float y, float w, float h,
		int nextLevel, float spawnX, float spawnY,
		ID3D11ShaderResourceView* texture);

	bool CheckTrigger(const PlayerObject& player) const;
	void Render() const;

	int GetTargetLevel() const { return m_TargetLevelID; }
	void GetSpawnPosition(float& x, float& y) const;

	const BoundingBox& GetTriggerArea() const { return m_TriggerArea; }
	void SetTriggerArea(const BoundingBox& area) { m_TriggerArea = area; }
};
