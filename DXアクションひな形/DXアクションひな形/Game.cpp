#include "Main.h"
#include "Renderer.h"
#include "Manager.h"
#include "Sprite.h"
#include "Game.h"
#include "Audio.h"
#include "Input.h"

#include "PlayerObject.h"
#include "LevelManager.h"
#include "PlatformEditor.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <vector>

namespace
{
	// Spawn the player on top of the shared prototype platform.
	constexpr float PLAYER_START_X = SCREEN_WIDTH / 2.0f - 30.0f;
	constexpr float PLAYER_START_Y = SCREEN_HEIGHT - 160.0f - 60.0f;

	// Intro overlay parameters.
	constexpr int   INTRO_MASK_SIZE = 1024;       // Procedural mask resolution (px).
	constexpr float INTRO_REVEAL_DURATION = 1.2f; // Seconds.
	// Furthest distance from a screen-centered point to a corner is the diagonal;
	// add margin so the reveal fully clears the screen even if the player is in a corner.
	const float     INTRO_REVEAL_RADIUS_MAX =
		sqrtf((float)SCREEN_WIDTH * (float)SCREEN_WIDTH +
			  (float)SCREEN_HEIGHT * (float)SCREEN_HEIGHT) + 64.0f;

	// Intro prompt font cell metrics on the shared ASCII font atlas.
	constexpr float PROMPT_CHAR_WIDTH  = 56.0f;
	constexpr float PROMPT_CHAR_HEIGHT = 84.0f;
	constexpr float PROMPT_Y_RATIO     = 0.72f;
}

enum class IntroState
{
	Waiting,
	Revealing,
	Done,
};

static int g_AudioBGM;
static PlayerObject* g_Player = nullptr;
static LevelManager* g_LevelManager = nullptr;

static IntroState g_IntroState = IntroState::Waiting;
static float g_IntroTime = 0.0f;
static float g_IntroRadius = 0.0f;
static float g_IntroCenterX = 0.0f;
static float g_IntroCenterY = 0.0f;

static ID3D11ShaderResourceView* g_MaskCircleTex = nullptr;
static ID3D11ShaderResourceView* g_WhitePixelTex = nullptr;
static ID3D11ShaderResourceView* g_PromptFontTex = nullptr;
static ID3D11ShaderResourceView* g_TitleTex = nullptr;   // "ようこそ" title art.
static ID3D11ShaderResourceView* g_PressTex = nullptr;   // "おす" press-key prompt art.

// ------- Title-screen image editor -------
// The title art and the press-key prompt are positioned/sized by these boxes
// so they can be adjusted in-place with the platform editor (screen space) and
// persisted to title_overrides.txt. Press 'A' on the title screen to toggle.
static PlatformEditor g_TitleEditor;
static bool g_TitleEditMode = false;
static PlatformEditor::Box g_TitleBox{};   // index 0
static PlatformEditor::Box g_PressBox{};   // index 1
static float g_TitleSaveFlash = 0.0f;

namespace
{
	constexpr char TITLE_OVERRIDE_PATH[] = "title_overrides.txt";

	void LoadTitleOverrides()
	{
		FILE* fp = fopen(TITLE_OVERRIDE_PATH, "r");
		if (!fp) return;

		char line[256];
		while (fgets(line, sizeof(line), fp))
		{
			if (line[0] == '#') continue;
			int id; float x, y, w, h;
			if (sscanf(line, "%d %f %f %f %f", &id, &x, &y, &w, &h) == 5)
			{
				if (id == 0) g_TitleBox = { x, y, w, h };
				else if (id == 1) g_PressBox = { x, y, w, h };
			}
		}
		fclose(fp);
	}

	void SaveTitleOverrides()
	{
		FILE* fp = fopen(TITLE_OVERRIDE_PATH, "w");
		if (!fp) return;
		fputs("# id x y width height  (0=title art, 1=press prompt)\n", fp);
		fprintf(fp, "0 %.3f %.3f %.3f %.3f\n",
			g_TitleBox.x, g_TitleBox.y, g_TitleBox.width, g_TitleBox.height);
		fprintf(fp, "1 %.3f %.3f %.3f %.3f\n",
			g_PressBox.x, g_PressBox.y, g_PressBox.width, g_PressBox.height);
		fclose(fp);
	}
}


static ID3D11ShaderResourceView* CreateWhitePixelSRV()
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

// Build a square RGBA mask whose interior circle is fully transparent
// (alpha 0) and whose corners are opaque black (alpha 1), with a 1.5 px
// feathered edge so the reveal boundary is anti-aliased.
//
// The circle's diameter equals the texture width minus a small border to
// guarantee that pixels along the mask edge stay fully opaque, which
// makes the seam against the surrounding black rectangles invisible.
static ID3D11ShaderResourceView* CreateCircleMaskSRV(int sizePx)
{
	std::vector<uint32_t> pixels((size_t)sizePx * (size_t)sizePx);

	const float center = (sizePx - 1) * 0.5f;
	const float radius = sizePx * 0.5f - 2.0f;
	const float feather = 1.5f;

	for (int y = 0; y < sizePx; ++y)
	{
		for (int x = 0; x < sizePx; ++x)
		{
			const float dx = (float)x - center;
			const float dy = (float)y - center;
			const float d  = sqrtf(dx * dx + dy * dy);

			float t = (d - (radius - feather)) / (2.0f * feather);
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;

			const uint8_t a = (uint8_t)(t * 255.0f);
			// R=0, G=0, B=0, A=a packed as RGBA8 little-endian.
			pixels[(size_t)y * (size_t)sizePx + (size_t)x] = ((uint32_t)a) << 24;
		}
	}

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = (UINT)sizePx;
	desc.Height = (UINT)sizePx;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA init{};
	init.pSysMem = pixels.data();
	init.SysMemPitch = (UINT)(sizePx * sizeof(uint32_t));

	ID3D11Texture2D* tex = nullptr;
	GetDevice()->CreateTexture2D(&desc, &init, &tex);
	assert(tex);

	ID3D11ShaderResourceView* srv = nullptr;
	GetDevice()->CreateShaderResourceView(tex, nullptr, &srv);
	tex->Release();
	assert(srv);
	return srv;
}

static void DrawBlackRect(float left, float top, float right, float bottom)
{
	if (left < 0.0f)            left = 0.0f;
	if (top < 0.0f)             top = 0.0f;
	if (right > SCREEN_WIDTH)   right = SCREEN_WIDTH;
	if (bottom > SCREEN_HEIGHT) bottom = SCREEN_HEIGHT;
	if (right <= left || bottom <= top) return;

	const float w = right - left;
	const float h = bottom - top;
	const XMFLOAT2 center = { left + w * 0.5f, top + h * 0.5f };
	const XMFLOAT2 size   = { w, h };
	const XMFLOAT4 black  = { 0.0f, 0.0f, 0.0f, 1.0f };

	DrawSprite(false, g_WhitePixelTex, center, size, 0.0f, black);
}

static void DrawIntroMask()
{
	const float cx = g_IntroCenterX;
	const float cy = g_IntroCenterY;
	const float R  = g_IntroRadius;

	const float pngLeft   = cx - R;
	const float pngRight  = cx + R;
	const float pngTop    = cy - R;
	const float pngBottom = cy + R;

	// Overlap the rectangles 1 px into the PNG bounds to hide any seam
	// that could appear if the mask's outermost pixels aren't perfectly opaque.
	constexpr float overlap = 1.0f;

	DrawBlackRect(0.0f, 0.0f, pngLeft + overlap, (float)SCREEN_HEIGHT);
	DrawBlackRect(pngRight - overlap, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
	DrawBlackRect(pngLeft, 0.0f, pngRight, pngTop + overlap);
	DrawBlackRect(pngLeft, pngBottom - overlap, pngRight, (float)SCREEN_HEIGHT);

	if (R > 0.0f && g_MaskCircleTex)
	{
		const XMFLOAT2 pos  = { cx, cy };
		const XMFLOAT2 size = { 2.0f * R, 2.0f * R };
		DrawSprite(false, g_MaskCircleTex, pos, size);
	}
}

static void DrawTitleScreen()
{
	// Title art drawn in screen space from the editable boxes.
	if (g_TitleTex)
	{
		const XMFLOAT2 center = { g_TitleBox.x + g_TitleBox.width * 0.5f,
								  g_TitleBox.y + g_TitleBox.height * 0.5f };
		DrawSprite(false, g_TitleTex, center, { g_TitleBox.width, g_TitleBox.height });
	}

	if (g_PressTex)
	{
		const XMFLOAT2 center = { g_PressBox.x + g_PressBox.width * 0.5f,
								  g_PressBox.y + g_PressBox.height * 0.5f };
		DrawSprite(false, g_PressTex, center, { g_PressBox.width, g_PressBox.height });
	}
}

// Draws a string from the ASCII font atlas in screen space, anchored at (x, y).
static void DrawScreenText(const char* text, float x, float y, float charW, float charH)
{
	if (!g_PromptFontTex || !text) return;

	constexpr int   kGrid = 16;
	constexpr int   kAsciiBase = 32;
	constexpr float kCellUV = 1.0f / (float)kGrid;

	for (int i = 0; text[i] != '\0'; ++i)
	{
		const unsigned char ch = (unsigned char)text[i];
		if (ch < kAsciiBase) continue;

		const int index = ch - kAsciiBase;
		const int col = index % kGrid;
		const int row = index / kGrid;

		const XMFLOAT2 texPos   = { col * kCellUV, row * kCellUV };
		const XMFLOAT2 texScale = { kCellUV, kCellUV };
		const XMFLOAT2 center   = { x + (i + 0.5f) * charW, y + charH * 0.5f };
		const XMFLOAT2 size     = { charW, charH };

		DrawSpriteTexCoord(false, g_PromptFontTex, center, size, texPos, texScale);
	}
}

static void DrawDebugText()
{
	DrawScreenText("DEBUG MODE", 16.0f, 16.0f, 28.0f, 42.0f);
}


void InitGame()
{
	// Sync key buffers before the intro's any-key listener becomes active,
	// otherwise the all-zero initial OldKeyState would make any key the OS
	// currently reports as held look like a fresh trigger on the first frame.
	PrimeInput();

	g_Player = new PlayerObject();
	g_Player->Init(PLAYER_START_X, PLAYER_START_Y);

	g_LevelManager = new LevelManager();
	g_LevelManager->Init(g_Player);

	// Frame the camera on the player immediately. The level and player exist
	// during the intro but gameplay (and thus camera follow) is frozen until
	// the reveal finishes; without this snap the scroll stays at its leftover
	// value and visibly glides to the player once gameplay starts.
	g_Player->SnapCameraToPlayer();

	g_AudioBGM = LoadAudio("Asset\\Audio\\BGM.wav");
	//PlayAudio(g_AudioBGM, true);

	g_MaskCircleTex = CreateCircleMaskSRV(INTRO_MASK_SIZE);
	g_WhitePixelTex = CreateWhitePixelSRV();

	{
		TexMetadata metadata;
		ScratchImage image;
		LoadFromWICFile(L"resource\\consolab_ascii_512.png", WIC_FLAGS_NONE, &metadata, image);
		CreateShaderResourceView(GetDevice(), image.GetImages(),
			image.GetImageCount(), metadata, &g_PromptFontTex);
		assert(g_PromptFontTex);
	}

	{
		TexMetadata metadata;
		ScratchImage image;
		LoadFromWICFile(L"resource\\youkoso.png", WIC_FLAGS_NONE, &metadata, image);
		CreateShaderResourceView(GetDevice(), image.GetImages(),
			image.GetImageCount(), metadata, &g_TitleTex);
		assert(g_TitleTex);

		LoadFromWICFile(L"resource\\osu.png", WIC_FLAGS_NONE, &metadata, image);
		CreateShaderResourceView(GetDevice(), image.GetImages(),
			image.GetImageCount(), metadata, &g_PressTex);
		assert(g_PressTex);
	}

	// Title-image editor: screen-space boxes seeded from the default layout,
	// then overridden by any saved title_overrides.txt.
	g_TitleEditor.Init();
	g_TitleEditor.SetScreenSpace(true);
	g_TitleEditMode = false;
	g_TitleSaveFlash = 0.0f;
	{
		constexpr float kAspect = 2266.0f / 1488.0f;
		const float tw = 800.0f, th = tw / kAspect;
		g_TitleBox = { SCREEN_WIDTH * 0.5f - tw * 0.5f,
			SCREEN_HEIGHT * 0.34f - th * 0.5f, tw, th };
		const float pw = 360.0f, ph = pw / kAspect;
		g_PressBox = { SCREEN_WIDTH * 0.5f - pw * 0.5f,
			SCREEN_HEIGHT * PROMPT_Y_RATIO - ph * 0.5f, pw, ph };
	}
	LoadTitleOverrides();

	// Snap the reveal center to the player's current screen-space position.
	// During the intro the player is frozen and the camera scroll is unchanged,
	// so this stays valid for the entire reveal animation.
	const BoundingBox& b = g_Player->GetCollider();
	XMFLOAT2 scroll = GetScrollOffset();
	g_IntroCenterX = b.GetCenterX() - scroll.x;
	g_IntroCenterY = b.GetCenterY() - scroll.y;

	g_IntroState  = IntroState::Waiting;
	g_IntroTime   = 0.0f;
	g_IntroRadius = 0.0f;
}

void UninitGame()
{
	g_TitleEditor.Uninit();

	UnloadAudio(g_AudioBGM);

	if (g_PressTex)
	{
		g_PressTex->Release();
		g_PressTex = nullptr;
	}
	if (g_TitleTex)
	{
		g_TitleTex->Release();
		g_TitleTex = nullptr;
	}
	if (g_PromptFontTex)
	{
		g_PromptFontTex->Release();
		g_PromptFontTex = nullptr;
	}
	if (g_WhitePixelTex)
	{
		g_WhitePixelTex->Release();
		g_WhitePixelTex = nullptr;
	}
	if (g_MaskCircleTex)
	{
		g_MaskCircleTex->Release();
		g_MaskCircleTex = nullptr;
	}

	if (g_LevelManager)
	{
		g_LevelManager->Uninit();
		delete g_LevelManager;
		g_LevelManager = nullptr;
	}
	if (g_Player)
	{
		g_Player->Uninit();
		delete g_Player;
		g_Player = nullptr;
	}
}

void UpdateGame(float deltaTime)
{
	if (g_IntroState == IntroState::Waiting)
	{
		// Press 'A' on the title screen to edit the title images in place.
		// While editing, the reveal mask is hidden (see DrawGame) and ENTER is
		// suppressed so editing can't accidentally start the game.
		if (GetKeyTrigger('A')) g_TitleEditMode = !g_TitleEditMode;

		if (g_TitleEditMode)
		{
			if (g_TitleSaveFlash > 0.0f) g_TitleSaveFlash -= deltaTime;

			const std::vector<PlatformEditor::Box> boxes = { g_TitleBox, g_PressBox };
			g_TitleEditor.Update(0, boxes,
				[](int index, const PlatformEditor::Box& nb)
				{
					if (index == 0)      g_TitleBox = nb;
					else if (index == 1) g_PressBox = nb;
				},
				[]() { SaveTitleOverrides(); g_TitleSaveFlash = 1.5f; });
			return;
		}

		if (GetKeyTrigger(VK_RETURN))
		{
			const BoundingBox& b = g_Player->GetCollider();
			XMFLOAT2 scroll = GetScrollOffset();
			g_IntroCenterX = b.GetCenterX() - scroll.x;
			g_IntroCenterY = b.GetCenterY() - scroll.y;

			g_IntroState  = IntroState::Revealing;
			g_IntroTime   = 0.0f;
			g_IntroRadius = 0.0f;
		}
		return;
	}

	if (g_IntroState == IntroState::Revealing)
	{
		g_IntroTime += deltaTime;
		float t = g_IntroTime / INTRO_REVEAL_DURATION;
		if (t >= 1.0f)
		{
			t = 1.0f;
			g_IntroState = IntroState::Done;
		}
		g_IntroRadius = t * INTRO_REVEAL_RADIUS_MAX;
		return;
	}

	if (g_LevelManager) g_LevelManager->Update(deltaTime);

	if (GetKeyTrigger(VK_RETURN))
	{
		SetScene(SCENE_RESULT);
	}
}

void DrawGame()
{
	if (g_LevelManager) g_LevelManager->Render();

	// Drop the reveal mask while in-game level editing or title-image editing,
	// so the subject being edited is fully visible.
	const bool levelEditing = g_LevelManager && g_LevelManager->IsEditing();

	if (g_IntroState != IntroState::Done && !levelEditing && !g_TitleEditMode)
	{
		DrawIntroMask();
	}

	if (g_Player) g_Player->Render();

	if (g_IntroState == IntroState::Waiting && !levelEditing)
	{
		DrawTitleScreen();

		// Title editor: highlight the selected image and its resize handle.
		if (g_TitleEditMode)
		{
			g_TitleEditor.Render({ g_TitleBox, g_PressBox });
			DrawScreenText("TITLE EDIT", 16.0f, 16.0f, 28.0f, 42.0f);
			if (g_TitleSaveFlash > 0.0f)
				DrawScreenText("SAVED", SCREEN_WIDTH * 0.5f - 100.0f, 80.0f, 40.0f, 60.0f);
		}
	}

	if (g_Player && g_Player->IsDebugFly())
	{
		DrawDebugText();
	}
}
