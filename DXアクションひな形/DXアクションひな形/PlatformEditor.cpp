// ===================================================
// PlatformEditor.cpp
// ===================================================

#include "Main.h"
#include "Renderer.h"
#include "Sprite.h"
#include "Input.h"
#include "PlatformEditor.h"

#include <cstdint>
#include <algorithm>

namespace
{
	// Smallest box extent the editor will resize down to (world px).
	constexpr float MIN_SIZE = 8.0f;

	// Size of the bottom-right resize grab box, in world px.
	constexpr float HANDLE_SIZE = 28.0f;

	// Thickness of the highlight outline edges, in world px.
	constexpr float OUTLINE_THICKNESS = 3.0f;

	const XMFLOAT4 SELECTED_COLOR = { 1.0f, 0.85f, 0.1f, 1.0f };  // amber
	const XMFLOAT4 HANDLE_COLOR   = { 0.1f, 1.0f, 0.4f, 1.0f };   // green

	bool PointInBox(float px, float py, const PlatformEditor::Box& b)
	{
		return px >= b.x && px <= b.x + b.width &&
			   py >= b.y && py <= b.y + b.height;
	}

	ID3D11ShaderResourceView* CreateWhitePixelSRV()
	{
		uint32_t pixel = 0xFFFFFFFFu;

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = 1;
		desc.Height = 1;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA init{};
		init.pSysMem = &pixel;
		init.SysMemPitch = sizeof(uint32_t);

		ID3D11Texture2D* tex = nullptr;
		GetDevice()->CreateTexture2D(&desc, &init, &tex);
		assert(tex);

		ID3D11ShaderResourceView* srv = nullptr;
		GetDevice()->CreateShaderResourceView(tex, nullptr, &srv);
		tex->Release();
		assert(srv);
		return srv;
	}
}


PlatformEditor::PlatformEditor()
	: m_ScreenSpace(false)
	, m_LevelID(0)
	, m_SelectedIndex(-1)
	, m_Drag(Drag::None)
	, m_SnapBox{ 0.0f, 0.0f, 0.0f, 0.0f }
	, m_GrabX(0.0f)
	, m_GrabY(0.0f)
	, m_PrevMouseDown(false)
	, m_MarkerTex(nullptr)
{
}

PlatformEditor::~PlatformEditor()
{
	Uninit();
}

void PlatformEditor::Init()
{
	m_MarkerTex = CreateWhitePixelSRV();
}

void PlatformEditor::Uninit()
{
	if (m_MarkerTex)
	{
		m_MarkerTex->Release();
		m_MarkerTex = nullptr;
	}
}

bool PlatformEditor::GetMouseWorld(float& worldX, float& worldY) const
{
	POINT p;
	if (!GetCursorPos(&p)) return false;
	if (!ScreenToClient(GetWindow(), &p)) return false;

	RECT rc;
	if (!GetClientRect(GetWindow(), &rc)) return false;
	const float clientW = (float)(rc.right - rc.left);
	const float clientH = (float)(rc.bottom - rc.top);
	if (clientW <= 0.0f || clientH <= 0.0f) return false;

	// Client pixels -> logical 1920x1080 space. In world space, add the camera
	// scroll; in screen space (title UI) the logical position is already final.
	worldX = (float)p.x / clientW * SCREEN_WIDTH;
	worldY = (float)p.y / clientH * SCREEN_HEIGHT;
	if (!m_ScreenSpace)
	{
		const XMFLOAT2 scroll = GetScrollOffset();
		worldX += scroll.x;
		worldY += scroll.y;
	}
	return true;
}

void PlatformEditor::Update(int levelID,
	const std::vector<Box>& boxes,
	const std::function<void(int, const Box&)>& onEdit,
	const std::function<void()>& onSave)
{
	// Selection does not survive a level change.
	if (levelID != m_LevelID)
	{
		m_LevelID = levelID;
		m_SelectedIndex = -1;
		m_Drag = Drag::None;
	}

	if (m_SelectedIndex >= (int)boxes.size()) m_SelectedIndex = -1;

	float wx, wy;
	if (!GetMouseWorld(wx, wy))
	{
		m_PrevMouseDown = false;
		return;
	}

	const bool down = GetKeyPress(VK_LBUTTON);
	const bool pressed = down && !m_PrevMouseDown;

	if (pressed)
	{
		// Pick the smallest box under the cursor. This makes a click on the
		// artwork select the tightest matching item, and avoids a large
		// surrounding box swallowing clicks meant for something on top of it.
		m_SelectedIndex = -1;
		m_Drag = Drag::None;
		float bestArea = 0.0f;
		for (int i = 0; i < (int)boxes.size(); ++i)
		{
			if (!PointInBox(wx, wy, boxes[i])) continue;
			const float area = boxes[i].width * boxes[i].height;
			if (m_SelectedIndex < 0 || area < bestArea)
			{
				m_SelectedIndex = i;
				bestArea = area;
			}
		}

		if (m_SelectedIndex >= 0)
		{
			const Box& b = boxes[m_SelectedIndex];
			m_SnapBox = b;
			m_GrabX = wx;
			m_GrabY = wy;

			// Grabbing the bottom-right corner resizes; elsewhere moves.
			const bool onHandle =
				wx >= b.x + b.width - HANDLE_SIZE &&
				wy >= b.y + b.height - HANDLE_SIZE;
			m_Drag = onHandle ? Drag::Resize : Drag::Move;
		}
	}

	if (down && m_SelectedIndex >= 0 && m_Drag != Drag::None && onEdit)
	{
		Box nb = m_SnapBox;
		if (m_Drag == Drag::Move)
		{
			nb.x = m_SnapBox.x + (wx - m_GrabX);
			nb.y = m_SnapBox.y + (wy - m_GrabY);
		}
		else // Resize, anchored at the top-left.
		{
			nb.width = std::max(MIN_SIZE, m_SnapBox.width + (wx - m_GrabX));
			nb.height = std::max(MIN_SIZE, m_SnapBox.height + (wy - m_GrabY));
		}
		onEdit(m_SelectedIndex, nb);
	}

	if (!down)
	{
		m_Drag = Drag::None;
	}

	if (GetKeyTrigger('S') && onSave)
	{
		onSave();
	}

	m_PrevMouseDown = down;
}

void PlatformEditor::DrawOutline(const Box& b, const XMFLOAT4& color) const
{
	if (!m_MarkerTex) return;

	const bool scr = !m_ScreenSpace;
	const float t = OUTLINE_THICKNESS;
	DrawSprite(scr, m_MarkerTex, { b.x + b.width * 0.5f, b.y }, { b.width, t }, 0.0f, color);
	DrawSprite(scr, m_MarkerTex, { b.x + b.width * 0.5f, b.y + b.height }, { b.width, t }, 0.0f, color);
	DrawSprite(scr, m_MarkerTex, { b.x, b.y + b.height * 0.5f }, { t, b.height }, 0.0f, color);
	DrawSprite(scr, m_MarkerTex, { b.x + b.width, b.y + b.height * 0.5f }, { t, b.height }, 0.0f, color);
}

void PlatformEditor::Render(const std::vector<Box>& boxes) const
{
	if (m_SelectedIndex < 0 || m_SelectedIndex >= (int)boxes.size()) return;

	const Box& b = boxes[m_SelectedIndex];
	DrawOutline(b, SELECTED_COLOR);

	// Resize grab marker in the bottom-right corner.
	const float hx = b.x + b.width - HANDLE_SIZE * 0.5f;
	const float hy = b.y + b.height - HANDLE_SIZE * 0.5f;
	DrawSprite(!m_ScreenSpace, m_MarkerTex, { hx, hy }, { HANDLE_SIZE, HANDLE_SIZE }, 0.0f, HANDLE_COLOR);
}
