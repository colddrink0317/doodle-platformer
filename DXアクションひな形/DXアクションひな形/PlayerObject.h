#pragma once

// ===================================================
// PlayerObject.h
//   Prototype Player class. Integrates:
//     - delta-time driven physics in px/s and px/s^2
//     - AABB collision correction against Platform collections
//     - The IDLE/WALK/JUMP/FALL state machine
//     - Camera scrolling through the existing screen limit box
// ===================================================

#include "Main.h"
#include "Platform.h"

#include <vector>

enum class PlayerMode
{
	Idle,
	Walk,
	Jump,
	Fall,
};

class PlayerObject
{
private:
	BoundingBox m_Collider;
	float m_VelX;
	float m_VelY;

	bool m_Grounded;
	PlayerMode m_Mode;

	bool m_DebugFly;	// Debug fly mode: ignore gravity/collision, free movement.

	ID3D11ShaderResourceView* m_Texture;

	float m_MoveSpeed;
	float m_JumpForce;
	float m_Gravity;
	float m_CameraMoveTime;

public:
	PlayerObject();
	~PlayerObject();

	void Init(float startX, float startY);
	void Uninit();

	void Update(float deltaTime, const std::vector<Platform*>& platforms);
	void Render() const;

	void SetPosition(float x, float y);
	void ResetVelocity();
	// Snap the camera scroll to the player's current position with no glide,
	// so the view is already framed before gameplay/the intro reveal begins.
	void SnapCameraToPlayer();
	const BoundingBox& GetCollider() const { return m_Collider; }
	bool IsDebugFly() const { return m_DebugFly; }

	// Toggles debug-fly mode (also used outside the normal update, e.g. on the
	// title/intro screen so the platform editor can be entered there).
	void ToggleDebugFly();

	// Debug-only update: handles the 'A' toggle and, while flying, free
	// movement + camera pan. Never runs gameplay physics, so it is safe to
	// call while gameplay is otherwise frozen (intro screen).
	void UpdateDebugOnly(float deltaTime);

private:
	void HandleInput();
	void ResolveCollisionsX(float deltaTime, const std::vector<Platform*>& platforms);
	void ResolveCollisionsY(float deltaTime, const std::vector<Platform*>& platforms);
	void UpdateCameraScroll(float deltaTime);
	void UpdateDebugFly(float deltaTime);
};
