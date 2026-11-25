// ===================================================
// PlayerObject.cpp
// ===================================================

#include "Main.h"
#include "Renderer.h"
#include "Sprite.h"
#include "Input.h"
#include "PlayerObject.h"

namespace
{
	constexpr float PLAYER_SIZE_X = 60.0f;
	constexpr float PLAYER_SIZE_Y = 60.0f;

	// Convert the original player.cpp px/frame values to prototype px/s units.
	constexpr float MOVE_SPEED = 400.0f;
	constexpr float JUMP_FORCE = -800.0f;
	constexpr float GRAVITY = 1500.0f;
	constexpr float MAX_FALL_SPEED = 4680.0f; // 78 px/frame * 60

	// Debug fly mode free-movement speed (px/s).
	constexpr float DEBUG_FLY_SPEED = 500.0f;

	// Camera keeps the player slightly below screen center.
	constexpr float CAMERA_CENTER_X = SCREEN_WIDTH / 2.0f;
	constexpr float CAMERA_PLAYER_VIEW_Y = SCREEN_HEIGHT * 0.6f;
	constexpr float CAMERA_START_SPEED_RATE = 0.9f;
	constexpr float CAMERA_END_SPEED_RATE = 1.0f;
	constexpr float CAMERA_SPEED_RAMP_TIME = 2.0f;
	constexpr float CAMERA_MOVE_EPSILON = 1.0f;

	// 画像平台の継ぎ目などに生じる極小の突起で横移動が引っかからないようにする閾値（px）
	constexpr float STEP_TOLERANCE = 6.0f;
	constexpr float CAMERA_VERTICAL_CATCHUP_SPEED = 1200.0f;

	float MoveTowards(float current, float target, float maxDelta)
	{
		float diff = target - current;
		if (fabsf(diff) <= maxDelta)
		{
			return target;
		}
		return current + ((diff > 0.0f) ? maxDelta : -maxDelta);
	}
}


PlayerObject::PlayerObject()
	: m_Collider{ 0.0f, 0.0f, PLAYER_SIZE_X, PLAYER_SIZE_Y }
	, m_VelX(0.0f)
	, m_VelY(0.0f)
	, m_Grounded(false)
	, m_Mode(PlayerMode::Fall)
	, m_DebugFly(false)
	, m_Texture(nullptr)
	, m_MoveSpeed(MOVE_SPEED)
	, m_JumpForce(JUMP_FORCE)
	, m_Gravity(GRAVITY)
	, m_CameraMoveTime(0.0f)
{
}

PlayerObject::~PlayerObject()
{
	Uninit();
}

void PlayerObject::Init(float startX, float startY)
{
	m_Collider.x = startX;
	m_Collider.y = startY;
	m_Collider.width = PLAYER_SIZE_X;
	m_Collider.height = PLAYER_SIZE_Y;
	m_VelX = 0.0f;
	m_VelY = 0.0f;
	m_Grounded = false;
	m_Mode = PlayerMode::Fall;
	m_CameraMoveTime = 0.0f;

	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"resource\\Player.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(GetDevice(), image.GetImages(),
		image.GetImageCount(), metadata, &m_Texture);
	assert(m_Texture);
}

void PlayerObject::Uninit()
{
	if (m_Texture)
	{
		m_Texture->Release();
		m_Texture = nullptr;
	}
}

void PlayerObject::SetPosition(float x, float y)
{
	m_Collider.x = x;
	m_Collider.y = y;
}

void PlayerObject::ResetVelocity()
{
	m_VelX = 0.0f;
	m_VelY = 0.0f;
}

void PlayerObject::HandleInput()
{
	if (GetKeyPress(VK_LEFT))
	{
		m_VelX = -m_MoveSpeed;
	}
	else if (GetKeyPress(VK_RIGHT))
	{
		m_VelX = m_MoveSpeed;
	}
	else
	{
		m_VelX *= 0.8f;
		if (fabsf(m_VelX) < 1.0f)
		{
			m_VelX = 0.0f;
		}
	}

	if ((GetKeyTrigger('Z') || GetKeyTrigger(VK_SPACE)) && m_Grounded)
	{
		m_VelY = m_JumpForce;
		m_Grounded = false;
		m_Mode = PlayerMode::Jump;
	}
}

void PlayerObject::ResolveCollisionsX(float deltaTime, const std::vector<Platform*>& platforms)
{
	m_Collider.x += m_VelX * deltaTime;

	for (Platform* p : platforms)
	{
		if (!p) continue;
		const BoundingBox& pb = p->GetCollider();
		if (!m_Collider.Intersects(pb)) continue;

		// 平台上端が足元よりわずかに高いだけ（極小の突起）なら、横移動を止めずに
		// その上へ乗り上げる。これで一見平らな床で引っかかるのを防ぐ。
		const float feet = m_Collider.y + m_Collider.height;
		const float protrusion = feet - pb.y;
		if (protrusion > 0.0f && protrusion <= STEP_TOLERANCE)
		{
			m_Collider.y = pb.y - m_Collider.height;
			continue;
		}

		if (m_VelX > 0.0f)
		{
			m_Collider.x = pb.x - m_Collider.width;
		}
		else if (m_VelX < 0.0f)
		{
			m_Collider.x = pb.x + pb.width;
		}
		m_VelX = 0.0f;
	}
}

void PlayerObject::ResolveCollisionsY(float deltaTime, const std::vector<Platform*>& platforms)
{
	m_VelY += m_Gravity * deltaTime;
	if (m_VelY > MAX_FALL_SPEED) m_VelY = MAX_FALL_SPEED;

	m_Collider.y += m_VelY * deltaTime;

	bool landed = false;
	for (Platform* p : platforms)
	{
		if (!p) continue;
		const BoundingBox& pb = p->GetCollider();
		if (!m_Collider.Intersects(pb)) continue;

		if (m_VelY > 0.0f)
		{
			m_Collider.y = pb.y - m_Collider.height;
			m_VelY = 0.0f;
			landed = true;
		}
		else if (m_VelY < 0.0f)
		{
			m_Collider.y = pb.y + pb.height;
			m_VelY = 0.0f;
		}
	}

	m_Grounded = landed;
}

void PlayerObject::SnapCameraToPlayer()
{
	const XMFLOAT2 scroll = GetScrollOffset();
	const float targetScrollX = m_Collider.GetCenterX() - CAMERA_CENTER_X;
	const float targetScrollY = m_Collider.GetCenterY() - CAMERA_PLAYER_VIEW_Y;
	UpdateScrollOffset(targetScrollX - scroll.x, targetScrollY - scroll.y);
	m_CameraMoveTime = 0.0f;
}

void PlayerObject::UpdateCameraScroll(float deltaTime)
{
	XMFLOAT2 scroll = GetScrollOffset();
	float playerCenterX = m_Collider.GetCenterX();
	float playerCenterY = m_Collider.GetCenterY();
	float targetScrollX = playerCenterX - CAMERA_CENTER_X;
	float targetScrollY = playerCenterY - CAMERA_PLAYER_VIEW_Y;

	if (fabsf(m_VelX) > CAMERA_MOVE_EPSILON)
	{
		m_CameraMoveTime += deltaTime;
		if (m_CameraMoveTime > CAMERA_SPEED_RAMP_TIME)
		{
			m_CameraMoveTime = CAMERA_SPEED_RAMP_TIME;
		}
	}
	else
	{
		m_CameraMoveTime = 0.0f;
	}

	float rampRate = m_CameraMoveTime / CAMERA_SPEED_RAMP_TIME;
	float speedRate = CAMERA_START_SPEED_RATE
		+ ((CAMERA_END_SPEED_RATE - CAMERA_START_SPEED_RATE) * rampRate);
	float maxXDelta = (m_MoveSpeed * speedRate) * deltaTime;
	float verticalSpeed = fabsf(m_VelY) + CAMERA_VERTICAL_CATCHUP_SPEED;
	const float baseCameraSpeed = m_MoveSpeed * speedRate;
	if (verticalSpeed < baseCameraSpeed)
	{
		verticalSpeed = baseCameraSpeed;
	}
	float maxYDelta = verticalSpeed * deltaTime;
	float nextScrollX = MoveTowards(scroll.x, targetScrollX, maxXDelta);
	float nextScrollY = MoveTowards(scroll.y, targetScrollY, maxYDelta);

	UpdateScrollOffset(nextScrollX - scroll.x, nextScrollY - scroll.y);
}

void PlayerObject::UpdateDebugFly(float deltaTime)
{
	m_VelX = 0.0f;
	m_VelY = 0.0f;

	if (GetKeyPress(VK_LEFT))       m_VelX = -DEBUG_FLY_SPEED;
	else if (GetKeyPress(VK_RIGHT)) m_VelX = DEBUG_FLY_SPEED;

	if (GetKeyPress(VK_UP))         m_VelY = -DEBUG_FLY_SPEED;
	else if (GetKeyPress(VK_DOWN))  m_VelY = DEBUG_FLY_SPEED;

	// Free movement: no gravity, no collision resolution.
	m_Collider.x += m_VelX * deltaTime;
	m_Collider.y += m_VelY * deltaTime;

	m_Grounded = false;

	UpdateCameraScroll(deltaTime);
}

void PlayerObject::ToggleDebugFly()
{
	m_DebugFly = !m_DebugFly;
	m_VelX = 0.0f;
	m_VelY = 0.0f;
}

void PlayerObject::UpdateDebugOnly(float deltaTime)
{
	if (GetKeyTrigger('A'))
	{
		ToggleDebugFly();
	}
	if (m_DebugFly)
	{
		UpdateDebugFly(deltaTime);
	}
}

void PlayerObject::Update(float deltaTime, const std::vector<Platform*>& platforms)
{
	// Toggle debug fly mode with the A key.
	if (GetKeyTrigger('A'))
	{
		ToggleDebugFly();
	}

	if (m_DebugFly)
	{
		UpdateDebugFly(deltaTime);
		return;
	}

	HandleInput();

	ResolveCollisionsX(deltaTime, platforms);
	ResolveCollisionsY(deltaTime, platforms);

	// State only reflects the physics result and does not change movement.
	if (m_Grounded)
	{
		if (fabsf(m_VelX) > 0.01f) m_Mode = PlayerMode::Walk;
		else m_Mode = PlayerMode::Idle;
	}
	else
	{
		m_Mode = (m_VelY < 0.0f) ? PlayerMode::Jump : PlayerMode::Fall;
	}

	UpdateCameraScroll(deltaTime);
}

void PlayerObject::Render() const
{
	XMFLOAT2 center = { m_Collider.GetCenterX(), m_Collider.GetCenterY() };
	XMFLOAT2 size = { m_Collider.width, m_Collider.height };
	DrawSprite(true, m_Texture, center, size);
}
