// ===================================================
// LevelManager.cpp
// ===================================================

#include "Main.h"
#include "Renderer.h"
#include "Sprite.h"
#include "Input.h"
#include "LevelManager.h"
#include "StaticPlatform.h"
#include "CollisionPlatform.h"
#include "ImageToPlatform.h"

#include <algorithm>
#include <float.h>
#include <vector>
#include <string.h>
#include <cstdint>
#include <cstdio>

namespace
{
	constexpr float PLAYER_WIDTH = 60.0f;
	constexpr float PLAYER_HEIGHT = 60.0f;

	constexpr float PLATFORM_WIDTH = 1000.0f;
	constexpr float PLATFORM_HEIGHT = 32.0f;
	constexpr float PLATFORM_LEFT = SCREEN_WIDTH / 2.0f - PLATFORM_WIDTH / 2.0f;
	constexpr float PLATFORM_TOP = SCREEN_HEIGHT - 160.0f;
	constexpr float PLATFORM_RIGHT = PLATFORM_LEFT + PLATFORM_WIDTH;
	constexpr float PLATFORM_GAP = 300.0f;
	constexpr float NARROW_PLATFORM_WIDTH = PLATFORM_WIDTH * 0.5f;
	constexpr float NARROW_PLATFORM_GAP = 120.0f;
	constexpr float VERTICAL_STEP_HEIGHT = 200.0f;

	constexpr float CHECKPOINT_WIDTH = 128.0f;
	constexpr float CHECKPOINT_HEIGHT = 32.0f;

	constexpr float PLAYER_SPAWN_X = SCREEN_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f;
	constexpr float PLAYER_SPAWN_Y = PLATFORM_TOP - PLAYER_HEIGHT;

	// How far below the lowest platform's bottom edge before the player
	// is considered "fallen off" and is respawned.
	constexpr float FALL_SAFETY_MARGIN = 600.0f;

	constexpr float GUIDE_CHAR_WIDTH = 32.0f;
	constexpr float GUIDE_CHAR_HEIGHT = 48.0f;
	constexpr float GUIDE_TOP = SCREEN_HEIGHT - 600.0f;

	float GetExtendedPlatformLeft(int platformIndex)
	{
		return PLATFORM_LEFT + platformIndex * (PLATFORM_WIDTH + PLATFORM_GAP);
	}

	float GetCenteredCheckpointX(float platformLeft)
	{
		return platformLeft + PLATFORM_WIDTH / 2.0f - CHECKPOINT_WIDTH / 2.0f;
	}

	float GetCenteredCheckpointX(float platformLeft, float platformWidth)
	{
		return platformLeft + platformWidth / 2.0f - CHECKPOINT_WIDTH / 2.0f;
	}

	float GetNarrowPlatformLeft(int column)
	{
		const float baseLeft = GetExtendedPlatformLeft(3)
			+ PLATFORM_WIDTH / 2.0f - NARROW_PLATFORM_WIDTH / 2.0f;
		return baseLeft + column * (NARROW_PLATFORM_WIDTH + NARROW_PLATFORM_GAP);
	}

	float GetNarrowPlatformTop(int stepIndex)
	{
		return PLATFORM_TOP - stepIndex * VERTICAL_STEP_HEIGHT;
	}

	float GetStepCheckpointX(float targetLeft, bool targetIsRight)
	{
		return targetIsRight
			? targetLeft
			: targetLeft + NARROW_PLATFORM_WIDTH - CHECKPOINT_WIDTH;
	}

	float GetStepGuideX(float lowerLeft, float targetLeft, bool targetIsRight)
	{
		const float lowerEdge = targetIsRight
			? lowerLeft + NARROW_PLATFORM_WIDTH
			: lowerLeft;
		const float targetEdge = targetIsRight
			? targetLeft
			: targetLeft + NARROW_PLATFORM_WIDTH;
		return (lowerEdge + targetEdge) * 0.5f - GUIDE_CHAR_WIDTH * 0.5f;
	}

	float GetStepGuideY(float lowerTop, float targetTop)
	{
		return (lowerTop + targetTop) * 0.5f - GUIDE_CHAR_HEIGHT * 0.5f;
	}

	// Level 11's far pit-bottom landing (the "kowaiyo" spot). Levels 12+
	// continue seamlessly from here: since level transitions never teleport
	// the player, each new level's first image must physically overlap the
	// world position of the checkpoint that leads into it.
	float GetFinalPitCenterX()
	{
		const float startLeft = GetNarrowPlatformLeft(0);
		const float lowerLeft = startLeft - NARROW_PLATFORM_WIDTH - NARROW_PLATFORM_GAP;
		return lowerLeft + PLATFORM_WIDTH * 0.5f;
	}

	float GetFinalPitTopY()
	{
		const float startTop = GetNarrowPlatformTop(4);
		const float lowerTop = startTop + 10000.0f;
		return lowerTop - 350.0f;
	}

	// -------- Debug respawn-jump menu layout (screen space, top-left) --------
	constexpr float RESPAWN_BTN_X = 20.0f;
	constexpr float RESPAWN_BTN_Y0 = 150.0f;
	constexpr float RESPAWN_BTN_W = 90.0f;
	constexpr float RESPAWN_BTN_H = 32.0f;
	constexpr float RESPAWN_BTN_GAP = 6.0f;
	constexpr float RESPAWN_CHAR_W = 14.0f;
	constexpr float RESPAWN_CHAR_H = 22.0f;

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

	// Mouse position in the 1920x1080 logical screen space (HUD space, no
	// camera scroll applied) used to hit-test the respawn-jump buttons.
	bool GetMouseScreenPos(float& x, float& y)
	{
		POINT p;
		if (!GetCursorPos(&p)) return false;
		if (!ScreenToClient(GetWindow(), &p)) return false;

		RECT rc;
		if (!GetClientRect(GetWindow(), &rc)) return false;
		const float clientW = (float)(rc.right - rc.left);
		const float clientH = (float)(rc.bottom - rc.top);
		if (clientW <= 0.0f || clientH <= 0.0f) return false;

		x = (float)p.x / clientW * SCREEN_WIDTH;
		y = (float)p.y / clientH * SCREEN_HEIGHT;
		return true;
	}
}


LevelManager::LevelManager()
	: m_CurrentLevelID(1)
	, m_PlayerRef(nullptr)
	, m_BlockTexture(nullptr)
	, m_FontTexture(nullptr)
	, m_LastRespawnLevel(1)
	, m_LastRespawnX(PLAYER_SPAWN_X)
	, m_LastRespawnY(PLAYER_SPAWN_Y)
	, m_SaveFlashTimer(0.0f)
	, m_UITexture(nullptr)
	, m_RespawnMenuMouseDown(false)
	, m_RespawnMarkerTexture(nullptr)
{
}

LevelManager::~LevelManager()
{
	Uninit();
}

void LevelManager::Init(PlayerObject* player)
{
	m_PlayerRef = player;

	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"resource\\consolab_ascii_512.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(GetDevice(), image.GetImages(),
		image.GetImageCount(), metadata, &m_FontTexture);
	assert(m_FontTexture);

	m_PlatformEditor.Init();
	m_UITexture = CreateWhitePixelSRV();

	{
		TexMetadata bubbleMeta;
		ScratchImage bubbleImage;
		LoadFromWICFile(L"resource\\bubble.png", WIC_FLAGS_NONE, &bubbleMeta, bubbleImage);
		CreateShaderResourceView(GetDevice(), bubbleImage.GetImages(),
			bubbleImage.GetImageCount(), bubbleMeta, &m_RespawnMarkerTexture);
		assert(m_RespawnMarkerTexture);
	}

	PreloadLevels();
	m_CurrentLevelID = 1;

	// Pull in the starting level's built-in respawn point (if any),
	// so the player respawns sensibly even without progressing.
	AdoptLevelRespawnPoint(m_CurrentLevelID);
}

void LevelManager::Uninit()
{
	m_PlatformEditor.Uninit();
	if (m_UITexture)
	{
		m_UITexture->Release();
		m_UITexture = nullptr;
	}
	if (m_RespawnMarkerTexture)
	{
		m_RespawnMarkerTexture->Release();
		m_RespawnMarkerTexture = nullptr;
	}
	ClearLevels();
	if (m_BlockTexture)
	{
		m_BlockTexture->Release();
		m_BlockTexture = nullptr;
	}
	if (m_FontTexture)
	{
		m_FontTexture->Release();
		m_FontTexture = nullptr;
	}
	m_PlayerRef = nullptr;
}

void LevelManager::ClearLevels()
{
	for (auto& kv : m_LevelDatabase)
	{
		for (Platform* p : kv.second.platforms) delete p;
		for (Checkpoint* c : kv.second.checkpoints) delete c;
		for (GuideText* g : kv.second.guideTexts) delete g;
		for (ImageDecoration& d : kv.second.decorations)
		{
			if (d.texture) { d.texture->Release(); d.texture = nullptr; }
		}
		delete kv.second.respawnPoint;
		kv.second.respawnPoint = nullptr;
	}
	m_LevelDatabase.clear();
}

void LevelManager::PreloadLevels()
{
	ClearLevels();
	for (const LevelDef& def : CreateLevelDefinitions())
		BuildLevel(def);

	ApplyPlatformOverrides();
}

// File kept beside the resources, in the working directory.
static constexpr char OVERRIDE_PATH[] = "platform_overrides.txt";

void LevelManager::LoadOverrides()
{
	m_Overrides.clear();

	FILE* fp = fopen(OVERRIDE_PATH, "r");
	if (!fp) return;

	// Read line by line so comment lines (starting with '#') are skipped;
	// a plain fscanf loop would stall on the header comment and read nothing.
	char line[256];
	while (fgets(line, sizeof(line), fp))
	{
		if (line[0] == '#') continue;

		int level, index;
		float x, y, w, h;
		if (sscanf(line, "%d %d %f %f %f %f", &level, &index, &x, &y, &w, &h) == 6)
		{
			m_Overrides[{ level, index }] = PlatformEditor::Box{ x, y, w, h };
		}
	}
	fclose(fp);
}

void LevelManager::SaveOverrides() const
{
	FILE* fp = fopen(OVERRIDE_PATH, "w");
	if (!fp) return;

	fputs("# levelID editItemIndex x y width height\n", fp);
	for (const auto& kv : m_Overrides)
	{
		const PlatformEditor::Box& b = kv.second;
		fprintf(fp, "%d %d %.3f %.3f %.3f %.3f\n",
			kv.first.first, kv.first.second, b.x, b.y, b.width, b.height);
	}
	fclose(fp);
}

void LevelManager::ApplyPlatformOverrides()
{
	LoadOverrides();
	for (const auto& kv : m_Overrides)
	{
		const int levelID = kv.first.first;
		const int index = kv.first.second;

		auto it = m_LevelDatabase.find(levelID);
		if (it == m_LevelDatabase.end()) continue;

		LevelData& data = it->second;
		if (index < 0 || index >= (int)data.editItems.size()) continue;

		ApplyEditItemBox(data, data.editItems[index], kv.second);
	}
}

PlatformEditor::Box LevelManager::GetEditItemBox(
	const LevelData& data, const EditItem& item) const
{
	if (item.kind == EditItem::Kind::Platform)
	{
		if (item.platformIndex >= 0 && item.platformIndex < (int)data.platforms.size()
			&& data.platforms[item.platformIndex])
		{
			const BoundingBox& b = data.platforms[item.platformIndex]->GetCollider();
			return { b.x, b.y, b.width, b.height };
		}
		return { 0.0f, 0.0f, 0.0f, 0.0f };
	}

	if (item.kind == EditItem::Kind::Checkpoint)
	{
		if (item.checkpointIndex >= 0 && item.checkpointIndex < (int)data.checkpoints.size()
			&& data.checkpoints[item.checkpointIndex])
		{
			const BoundingBox& b = data.checkpoints[item.checkpointIndex]->GetTriggerArea();
			return { b.x, b.y, b.width, b.height };
		}
		return { 0.0f, 0.0f, 0.0f, 0.0f };
	}

	if (item.kind == EditItem::Kind::RespawnMarker)
	{
		if (!data.respawnPoint) return { 0.0f, 0.0f, 0.0f, 0.0f };

		// Mirrors RenderRespawnMarker's box exactly, so the drag handle lines
		// up with the bubble sprite the player actually sees.
		float spawnX, spawnY;
		data.respawnPoint->GetSpawnPosition(spawnX, spawnY);
		const float w = PLAYER_WIDTH * 1.4f;
		const float h = PLAYER_HEIGHT * 1.4f;
		return {
			spawnX + PLAYER_WIDTH * 0.5f - w * 0.5f,
			spawnY + PLAYER_HEIGHT * 0.5f - h * 0.5f,
			w, h };
	}

	// ImageGroup: prefer the sprite box (covers the whole artwork); fall back
	// to the union of the collider blocks when there is no decoration.
	if (item.decorationIndex >= 0 && item.decorationIndex < (int)data.decorations.size())
	{
		const ImageDecoration& d = data.decorations[item.decorationIndex];
		return { d.centerX - d.width * 0.5f, d.centerY - d.height * 0.5f, d.width, d.height };
	}

	float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
	for (int i = item.colliderBegin; i < item.colliderEnd && i < (int)data.platforms.size(); ++i)
	{
		if (!data.platforms[i]) continue;
		const BoundingBox& b = data.platforms[i]->GetCollider();
		minX = std::min(minX, b.x);
		minY = std::min(minY, b.y);
		maxX = std::max(maxX, b.x + b.width);
		maxY = std::max(maxY, b.y + b.height);
	}
	if (minX > maxX) return { 0.0f, 0.0f, 0.0f, 0.0f };
	return { minX, minY, maxX - minX, maxY - minY };
}

void LevelManager::ApplyEditItemBox(LevelData& data, const EditItem& item,
	const PlatformEditor::Box& box)
{
	if (item.kind == EditItem::Kind::Platform)
	{
		if (item.platformIndex >= 0 && item.platformIndex < (int)data.platforms.size()
			&& data.platforms[item.platformIndex])
		{
			data.platforms[item.platformIndex]->SetCollider(
				BoundingBox{ box.x, box.y, box.width, box.height });
		}
		return;
	}

	if (item.kind == EditItem::Kind::Checkpoint)
	{
		if (item.checkpointIndex >= 0 && item.checkpointIndex < (int)data.checkpoints.size()
			&& data.checkpoints[item.checkpointIndex])
		{
			data.checkpoints[item.checkpointIndex]->SetTriggerArea(
				BoundingBox{ box.x, box.y, box.width, box.height });
		}
		return;
	}

	if (item.kind == EditItem::Kind::RespawnMarker)
	{
		// Position only -- a respawn point has no size of its own, so a
		// resize drag (box.width/height) is ignored; inverse of the box
		// built in GetEditItemBox.
		if (data.respawnPoint)
		{
			const float w = PLAYER_WIDTH * 1.4f;
			const float h = PLAYER_HEIGHT * 1.4f;
			data.respawnPoint->SetSpawnPosition(
				box.x + w * 0.5f - PLAYER_WIDTH * 0.5f,
				box.y + h * 0.5f - PLAYER_HEIGHT * 0.5f);
		}
		return;
	}

	// ImageGroup: transform the sprite + every collision block uniformly from
	// the item's current box to the requested box (translate + scale, anchored
	// at the top-left). Computing from the current box each frame telescopes,
	// so dragging stays drift-free.
	const PlatformEditor::Box old = GetEditItemBox(data, item);
	if (old.width <= 0.0f || old.height <= 0.0f) return;

	const float scaleX = box.width / old.width;
	const float scaleY = box.height / old.height;

	if (item.decorationIndex >= 0 && item.decorationIndex < (int)data.decorations.size())
	{
		ImageDecoration& d = data.decorations[item.decorationIndex];
		d.width *= scaleX;
		d.height *= scaleY;
		// The decoration box equals the group box, so re-anchor it directly.
		d.centerX = box.x + box.width * 0.5f;
		d.centerY = box.y + box.height * 0.5f;
	}

	for (int i = item.colliderBegin; i < item.colliderEnd && i < (int)data.platforms.size(); ++i)
	{
		if (!data.platforms[i]) continue;
		BoundingBox b = data.platforms[i]->GetCollider();
		b.x = box.x + (b.x - old.x) * scaleX;
		b.y = box.y + (b.y - old.y) * scaleY;
		b.width *= scaleX;
		b.height *= scaleY;
		data.platforms[i]->SetCollider(b);
	}
}

std::vector<PlatformEditor::Box> LevelManager::BuildEditBoxes(const LevelData& data) const
{
	std::vector<PlatformEditor::Box> boxes;
	boxes.reserve(data.editItems.size());
	for (const EditItem& item : data.editItems)
		boxes.push_back(GetEditItemBox(data, item));
	return boxes;
}

void LevelManager::AddSurfacePlatform(LevelData& data, float x, float y)
{
	AddSurfacePlatform(data, x, y, PLATFORM_WIDTH);
}

void LevelManager::AddSurfacePlatform(LevelData& data, float x, float y, float width)
{
	// Surface platforms now share the exact image-platform pipeline: the plat00
	// artwork is rendered undistorted and its colliders follow the drawn shape.
	// (x, y) is the platform's top-left and 'width' its span; the height follows
	// the image's aspect ratio. The ink top is anchored at y so the player still
	// stands at the intended surface height.
	AddImagePlatforms(data, L"resource\\plat00.png", x + width * 0.5f, y, width);
}

void LevelManager::AddDefaultSurfacePlatform(LevelData& data)
{
	AddSurfacePlatform(data, PLATFORM_LEFT, PLATFORM_TOP);
}

void LevelManager::AddImagePlatforms(LevelData& data, const wchar_t* path,
	float targetCenterX, float targetTopY, float targetWidth)
{
	TexMetadata metadata;
	ScratchImage loaded;
	if (FAILED(LoadFromWICFile(path, WIC_FLAGS_NONE, &metadata, loaded)))
		return;

	// Normalize to straight RGBA8 so the converter sees a known layout.
	ScratchImage converted;
	const Image* img = nullptr;
	if (metadata.format != DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		if (FAILED(Convert(*loaded.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM,
			TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted)))
			return;
		img = converted.GetImage(0, 0, 0);
	}
	else
	{
		img = loaded.GetImage(0, 0, 0);
	}
	if (!img) return;

	ImageToPlatformConfig cfg;
	cfg.cellSize = 24;
	cfg.fillThreshold = 0.08f;
	std::vector<PlatformRect> rects = ConvertImageToPlatforms(
		img->pixels, (int)img->width, (int)img->height,
		(int)img->rowPitch, cfg);
	if (rects.empty()) return;

	// Ink bounding box (in source-image pixels) of the drawn content.
	float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
	for (const PlatformRect& r : rects)
	{
		minX = std::min(minX, r.x);
		minY = std::min(minY, r.y);
		maxX = std::max(maxX, r.x + r.width);
		maxY = std::max(maxY, r.y + r.height);
	}

	const float inkWidth = maxX - minX;
	if (inkWidth <= 0.0f) return;

	// One uniform image->world transform drives BOTH the visual sprite and
	// the colliders, so the standable pixels line up exactly with the art.
	//   world = origin + imagePixel * scale
	// chosen so the ink box maps to (targetCenterX, targetTopY) at targetWidth.
	const float scale = targetWidth / inkWidth;
	const float leftX = targetCenterX - targetWidth * 0.5f;
	const float originX = leftX - minX * scale;
	const float originY = targetTopY - minY * scale;

	// Visual: render the original image as-is (transparent background kept).
	ID3D11ShaderResourceView* srv = nullptr;
	CreateShaderResourceView(GetDevice(), loaded.GetImages(),
		loaded.GetImageCount(), metadata, &srv);
	if (srv)
	{
		ImageDecoration deco;
		deco.texture = srv;
		deco.width = (float)img->width * scale;
		deco.height = (float)img->height * scale;
		deco.centerX = originX + deco.width * 0.5f;
		deco.centerY = originY + deco.height * 0.5f;
		data.decorations.push_back(deco);
	}

	// Collision: invisible colliders over the drawn pixels only, aligned to
	// the same transform as the sprite. The player stands on the artwork.
	for (const PlatformRect& r : rects)
	{
		const float wx = originX + r.x * scale;
		const float wy = originY + r.y * scale;
		data.platforms.push_back(new CollisionPlatform(
			wx, wy, r.width * scale, r.height * scale));
	}
}

void LevelManager::AddGuideText(LevelData& data, float x, float y,
	float charW, float charH, const char* text)
{
	data.guideTexts.push_back(new GuideText(x, y, charW, charH, text, m_FontTexture));
}

void LevelManager::SetLevelRespawnPoint(LevelData& data, float spawnX, float spawnY)
{
	// Replace any previously stored respawn marker for this level.
	delete data.respawnPoint;
	data.respawnPoint = new RespawnPoint(spawnX, spawnY);
}

std::vector<LevelManager::LevelDef> LevelManager::CreateLevelDefinitions() const
{
	std::vector<LevelDef> defs;

	const float defaultRespawnY = PLATFORM_TOP - PLAYER_HEIGHT;

	// -------- Level 1: default platform + youkoso image, checkpoint -> 2 --------
	{
		LevelDef d;
		d.levelID = 1;
		d.platforms.push_back({ PLATFORM_LEFT, PLATFORM_TOP, PLATFORM_WIDTH });
		d.checkpoints.push_back({
			PLATFORM_RIGHT - CHECKPOINT_WIDTH, PLATFORM_TOP - CHECKPOINT_HEIGHT,
			2, PLAYER_SPAWN_X, PLAYER_SPAWN_Y });
		// Built-in respawn at the default platform center.
		d.hasRespawn = true;
		d.respawnX = PLAYER_SPAWN_X;
		d.respawnY = PLAYER_SPAWN_Y;
		// Platforms drawn from the hand-drawn "みぎ" (right) image, nudging
		// the player toward the right-edge checkpoint.
		d.imagePlatforms.push_back({ L"resource\\migi.png",
			SCREEN_WIDTH / 2.0f, GUIDE_TOP, 300.0f });
		defs.push_back(std::move(d));
	}

	// -------- Level 2: default platform, left-edge checkpoint -> 3 --------
	{
		LevelDef d;
		d.levelID = 2;
		d.platforms.push_back({ PLATFORM_LEFT, PLATFORM_TOP, PLATFORM_WIDTH });
		d.checkpoints.push_back({
			PLATFORM_LEFT, PLATFORM_TOP - CHECKPOINT_HEIGHT,
			3, PLAYER_SPAWN_X, PLAYER_SPAWN_Y });
		// Level 2 intentionally has no built-in respawn point.
		// "ひだり" (left) image points the player at the left-edge checkpoint.
		d.imagePlatforms.push_back({ L"resource\\hidari.png",
			SCREEN_WIDTH / 2.0f, GUIDE_TOP, 300.0f });
		defs.push_back(std::move(d));
	}

	// -------- Level 3: default platform, elevated checkpoint -> 4 --------
	{
		LevelDef d;
		d.levelID = 3;
		d.platforms.push_back({ PLATFORM_LEFT, PLATFORM_TOP, PLATFORM_WIDTH });
		d.checkpoints.push_back({
			SCREEN_WIDTH / 2.0f - CHECKPOINT_WIDTH / 2.0f,
			PLATFORM_TOP - PLAYER_HEIGHT - 200.0f,
			4, PLAYER_SPAWN_X, PLAYER_SPAWN_Y });
		// Own respawn near the left edge, differing from Level 1's default.
		d.hasRespawn = true;
		d.respawnX = PLATFORM_LEFT + 200.0f;
		d.respawnY = defaultRespawnY;
		// "jump" image teaches the jump that reaches the elevated checkpoint.
		d.imagePlatforms.push_back({ L"resource\\jump.png",
			SCREEN_WIDTH / 2.0f, GUIDE_TOP, 300.0f });
		defs.push_back(std::move(d));
	}

	// -------- Levels 4-6: a pair of extended platforms walking rightward.
	// Level (4+i) spans platform indices i and i+1, checkpoint centered on
	// the newest platform, target (5+i). Only Level 4 has a built-in respawn.
	{
		for (int i = 0; i <= 2; ++i)
		{
			LevelDef d;
			d.levelID = 4 + i;
			d.platforms.push_back({ GetExtendedPlatformLeft(i), PLATFORM_TOP, PLATFORM_WIDTH });
			d.platforms.push_back({ GetExtendedPlatformLeft(i + 1), PLATFORM_TOP, PLATFORM_WIDTH });

			const float newestPlatformLeft = GetExtendedPlatformLeft(i + 1);
			const float guidePlatformLeft = GetExtendedPlatformLeft(i);
			const float respawnX = newestPlatformLeft + PLATFORM_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f;

			d.checkpoints.push_back({
				GetCenteredCheckpointX(newestPlatformLeft), PLATFORM_TOP - CHECKPOINT_HEIGHT,
				5 + i, respawnX, defaultRespawnY });

			if (i == 0)
			{
				d.hasRespawn = true;
				d.respawnX = respawnX;
				d.respawnY = defaultRespawnY;
			}

			// "いけ" (go!) image above the starting platform urges rightward.
			d.imagePlatforms.push_back({ L"resource\\ike.png",
				guidePlatformLeft + PLATFORM_WIDTH * 0.5f,
				PLATFORM_TOP - 350.0f, 300.0f });

			defs.push_back(std::move(d));
		}
	}

	// -------- Levels 7-10: zig-zag of narrow stepping platforms.
	// Each level places a lower platform and a target platform one step up,
	// alternating direction. Only Level 7 has a built-in respawn.
	{
		struct StepDef { int lowerCol, lowerStep, targetCol, targetStep; bool targetIsRight; const wchar_t* img; };
		const StepDef steps[] = {
			{ 0, 0, 1, 1, true,  L"resource\\migi.png"   },  // Level 7
			{ 1, 1, 0, 2, false, L"resource\\hidari.png" },  // Level 8
			{ 0, 2, 1, 3, true,  L"resource\\migi.png"   },  // Level 9
			{ 1, 3, 0, 4, false, L"resource\\hidari.png" },  // Level 10
		};

		for (int i = 0; i < 4; ++i)
		{
			const StepDef& s = steps[i];
			LevelDef d;
			d.levelID = 7 + i;

			const float lowerLeft = GetNarrowPlatformLeft(s.lowerCol);
			const float lowerTop = GetNarrowPlatformTop(s.lowerStep);
			const float targetLeft = GetNarrowPlatformLeft(s.targetCol);
			const float targetTop = GetNarrowPlatformTop(s.targetStep);

			d.platforms.push_back({ lowerLeft, lowerTop, NARROW_PLATFORM_WIDTH });
			d.platforms.push_back({ targetLeft, targetTop, NARROW_PLATFORM_WIDTH });

			const float targetSpawnX = targetLeft + NARROW_PLATFORM_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f;
			const float targetSpawnY = targetTop - PLAYER_HEIGHT;

			d.checkpoints.push_back({
				GetStepCheckpointX(targetLeft, s.targetIsRight), targetTop - CHECKPOINT_HEIGHT,
				8 + i, targetSpawnX, targetSpawnY });

			if (i == 0)
			{
				d.hasRespawn = true;
				d.respawnX = lowerLeft + NARROW_PLATFORM_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f;
				d.respawnY = lowerTop - PLAYER_HEIGHT;
			}

			// Direction image (right/left) sits between the two step platforms.
			d.imagePlatforms.push_back({ s.img,
				GetStepGuideX(lowerLeft, targetLeft, s.targetIsRight) + GUIDE_CHAR_WIDTH * 0.5f,
				GetStepGuideY(lowerTop, targetTop), 120.0f });

			defs.push_back(std::move(d));
		}
	}

	// -------- Level 11: start platform + kowaiyo image as the distant pit landing --------
	{
		LevelDef d;
		d.levelID = 11;

		const float startLeft = GetNarrowPlatformLeft(0);
		const float startTop = GetNarrowPlatformTop(4);
		const float lowerLeft = startLeft - NARROW_PLATFORM_WIDTH - NARROW_PLATFORM_GAP;
		const float lowerTop = startTop + 10000.0f;
		const float kowaiyoCenterX = lowerLeft + PLATFORM_WIDTH * 0.5f;
		const float kowaiyoWidth = 300.0f;
		const float kowaiyoTopY = lowerTop - 350.0f;

		d.platforms.push_back({ startLeft, startTop, NARROW_PLATFORM_WIDTH });

		d.hasRespawn = true;
		d.respawnX = startLeft + NARROW_PLATFORM_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f;
		d.respawnY = startTop - PLAYER_HEIGHT;

		// "とぶ" (jump/leap) image hovers beside the start platform.
		d.imagePlatforms.push_back({ L"resource\\tobu.png",
			startLeft - 150.0f, startTop - 350.0f, 300.0f });

		// "こわいよ" image is the sole landing platform at the far pit bottom
		// (its own ink-shaped colliders replace the old plain bar platform).
		d.imagePlatforms.push_back({ L"resource\\kowaiyo.png",
			kowaiyoCenterX, kowaiyoTopY, kowaiyoWidth });

		// Checkpoint above the "yo" (right end of the image), staged for the
		// next level once it exists (TriggerSeamlessTransition no-ops until then).
		d.checkpoints.push_back({
			kowaiyoCenterX + kowaiyoWidth * 0.5f - CHECKPOINT_WIDTH - 40.0f,
			kowaiyoTopY - CHECKPOINT_HEIGHT,
			12,
			kowaiyoCenterX - PLAYER_WIDTH / 2.0f, kowaiyoTopY - PLAYER_HEIGHT });

		defs.push_back(std::move(d));
	}

	// All of levels 12+ share the same Y plane as Level 11's kowaiyo landing
	// (GetFinalPitTopY()) and continue rightward from it in X. Level
	// transitions never teleport the player (TriggerSeamlessTransition only
	// swaps which level's colliders are active), so each new level's first
	// image is deliberately placed to overlap the exact world position of
	// the checkpoint that leads into it -- otherwise the player falls out
	// from under themselves the instant the level switches, and (lacking a
	// respawn of their own) gets bounced all the way back to Level 11's
	// respawn point. Every level below also gets its own hasRespawn as a
	// safety net for the same reason.
	const float chainTopY = GetFinalPitTopY();
	const float chainWidth = 300.0f;
	const float chain12X = GetFinalPitCenterX();          // overlaps the kowaiyo spot
	const float chain13X = chain12X;                      // chinamini overlaps Level 12's checkpoint
	const float chainNekoX = chain13X + 400.0f;            // nekohai
	const float chainInuX = chain13X + 800.0f;             // inuhai
	const float chainLoopBackX = chain13X + 1200.0f;       // re-entry landing spot for the retry loop
	const float chain140X = chainInuX;                     // bubu overlaps the "inu" checkpoint
	const float chain140NextX = chain140X + 400.0f;        // nekohaidesu
	const float chain150X = chainNekoX;                    // seikai overlaps the "neko" checkpoint
	const float chain141X = chain140NextX;                 // yarinaosu overlaps Level 140's checkpoint
	const float chain151X = chain150X;                     // ometedou overlaps Level 150's checkpoint

	// -------- Level 12: ochitarasinu image, checkpoint -> 13 --------
	{
		LevelDef d;
		d.levelID = 12;

		d.imagePlatforms.push_back({ L"resource\\ochitarasinu.png", chain12X, chainTopY, chainWidth });

		// Respawn reset to Level 11's "NEXT" checkpoint's current (editor-
		// adjusted) position, per platform_overrides.txt entry "11 3".
		d.hasRespawn = true;
		d.respawnX = 5223.827f;
		d.respawnY = 10083.186f;

		d.checkpoints.push_back({
			chain12X - CHECKPOINT_WIDTH * 0.5f, chainTopY - CHECKPOINT_HEIGHT,
			13,
			chain13X - PLAYER_WIDTH / 2.0f, chainTopY - PLAYER_HEIGHT });

		defs.push_back(std::move(d));
	}

	// -------- Level 13: branch point. nekohai / inuhai images;
	// two checkpoints send the player down the "inu" (140) or "neko" (150)
	// path. A duplicate chinamini image at chainLoopBackX is the landing
	// spot when Level 141's retry checkpoint sends the player back here.
	// (The entry-side chinamini at chain13X, overlapping Level 12's
	// checkpoint, was removed on request -- the built-in respawn below no
	// longer uses chain13X, since that spot has no floor; it targets
	// nekohai instead, the first solid ground reachable in this level.) --------
	{
		LevelDef d;
		d.levelID = 13;

		d.imagePlatforms.push_back({ L"resource\\nekohai.png", chainNekoX, chainTopY, chainWidth });
		d.imagePlatforms.push_back({ L"resource\\inuhai.png", chainInuX, chainTopY, chainWidth });
		d.imagePlatforms.push_back({ L"resource\\chinamini.png", chainLoopBackX, chainTopY, chainWidth });

		// Respawn reset to nekohai's current position (platform_overrides.txt
		// "13 0") -- chain13X is an empty pit now that the entry chinamini
		// image was removed, so nekohai is the first solid ground reachable.
		d.hasRespawn = true;
		d.respawnX = 5123.827f;
		d.respawnY = 9511.556f;

		// Choosing "neko" -> Level 150.
		d.checkpoints.push_back({
			chainNekoX - CHECKPOINT_WIDTH * 0.5f, chainTopY - CHECKPOINT_HEIGHT,
			150,
			chain150X - PLAYER_WIDTH / 2.0f, chainTopY - PLAYER_HEIGHT });

		// Choosing "inu" -> Level 140.
		d.checkpoints.push_back({
			chainInuX - CHECKPOINT_WIDTH * 0.5f, chainTopY - CHECKPOINT_HEIGHT,
			140,
			chain140X - PLAYER_WIDTH / 2.0f, chainTopY - PLAYER_HEIGHT });

		defs.push_back(std::move(d));
	}

	// -------- Level 140 (14-1, "inu" path): bubu, nekohaidesu, checkpoint -> 141 --------
	{
		LevelDef d;
		d.levelID = 140;

		d.imagePlatforms.push_back({ L"resource\\bubu.png", chain140X, chainTopY, chainWidth });
		d.imagePlatforms.push_back({ L"resource\\nekohaidesu.png", chain140NextX, chainTopY, chainWidth });

		// Respawn reset to Level 13's "inu" NEXT checkpoint's current
		// position, per platform_overrides.txt entry "13 4".
		d.hasRespawn = true;
		d.respawnX = 6175.0f;
		d.respawnY = 9376.0f;

		d.checkpoints.push_back({
			chain140NextX - CHECKPOINT_WIDTH * 0.5f, chainTopY - CHECKPOINT_HEIGHT,
			141,
			chain141X - PLAYER_WIDTH / 2.0f, chainTopY - PLAYER_HEIGHT });

		defs.push_back(std::move(d));
	}

	// -------- Level 150 (14-2, "neko" path): seikai, checkpoint -> 151 --------
	{
		LevelDef d;
		d.levelID = 150;

		d.imagePlatforms.push_back({ L"resource\\seikai.png", chain150X, chainTopY, chainWidth });

		// Respawn reset to Level 13's "neko" NEXT checkpoint's current
		// position, per platform_overrides.txt entry "13 3".
		d.hasRespawn = true;
		d.respawnX = 5131.0f;
		d.respawnY = 9366.0f;

		d.checkpoints.push_back({
			chain150X - CHECKPOINT_WIDTH * 0.5f, chainTopY - CHECKPOINT_HEIGHT,
			151,
			chain151X - PLAYER_WIDTH / 2.0f, chainTopY - PLAYER_HEIGHT });

		defs.push_back(std::move(d));
	}

	// -------- Level 141 (15-1, follows 140): yarinaosu, checkpoint loops back to Level 13 --------
	{
		LevelDef d;
		d.levelID = 141;

		d.imagePlatforms.push_back({ L"resource\\yarinaosu.png", chain141X, chainTopY, chainWidth });

		// Respawn reset to Level 140's NEXT checkpoint's current position,
		// per platform_overrides.txt entry "140 2".
		d.hasRespawn = true;
		d.respawnX = 7204.0f;
		d.respawnY = 9899.0f;

		d.checkpoints.push_back({
			chain141X - CHECKPOINT_WIDTH * 0.5f, chainTopY - CHECKPOINT_HEIGHT,
			13,
			chainLoopBackX - PLAYER_WIDTH / 2.0f, chainTopY - PLAYER_HEIGHT });

		defs.push_back(std::move(d));
	}

	// -------- Level 151 (15-2, follows 150): ometedou, ending (no further checkpoint).
	// The left-side seikai image was removed on request; ometedou now sits at
	// chain151X (Level 150's arrival spot) so the player still has ground
	// under them the instant they land here. --------
	{
		LevelDef d;
		d.levelID = 151;

		d.imagePlatforms.push_back({ L"resource\\ometedou.png", chain151X, chainTopY, chainWidth });

		// Respawn reset to Level 150's NEXT checkpoint's current position,
		// per platform_overrides.txt entry "150 1".
		d.hasRespawn = true;
		d.respawnX = 5774.0f;
		d.respawnY = 9856.0f;

		defs.push_back(std::move(d));
	}

	return defs;
}

void LevelManager::BuildLevel(const LevelDef& def)
{
	LevelData& data = m_LevelDatabase[def.levelID];

	for (const PlatformDef& p : def.platforms)
	{
		const float width = (p.width > 0.0f) ? p.width : PLATFORM_WIDTH;

		// Surface platforms spawn the same way image platforms do (a sprite +
		// a run of colliders), so group them into one ImageGroup edit item.
		const int pBefore = (int)data.platforms.size();
		const int dBefore = (int)data.decorations.size();
		AddSurfacePlatform(data, p.x, p.y, width);
		const int pAfter = (int)data.platforms.size();
		const int dAfter = (int)data.decorations.size();

		if (pAfter > pBefore || dAfter > dBefore)
		{
			EditItem item;
			item.kind = EditItem::Kind::ImageGroup;
			item.decorationIndex = (dAfter > dBefore) ? dBefore : -1;
			item.colliderBegin = pBefore;
			item.colliderEnd = pAfter;
			data.editItems.push_back(item);
		}
	}

	for (const ImagePlatformDef& ip : def.imagePlatforms)
	{
		// Group everything one image spawns (its sprite + collider run) into a
		// single edit item so the editor moves / resizes the whole image.
		const int pBefore = (int)data.platforms.size();
		const int dBefore = (int)data.decorations.size();
		AddImagePlatforms(data, ip.path, ip.centerX, ip.topY, ip.width);
		const int pAfter = (int)data.platforms.size();
		const int dAfter = (int)data.decorations.size();

		if (pAfter > pBefore || dAfter > dBefore)
		{
			EditItem item;
			item.kind = EditItem::Kind::ImageGroup;
			item.decorationIndex = (dAfter > dBefore) ? dBefore : -1;
			item.colliderBegin = pBefore;
			item.colliderEnd = pAfter;
			data.editItems.push_back(item);
		}
	}

	for (const CheckpointDef& c : def.checkpoints)
	{
		data.checkpoints.push_back(new Checkpoint(
			c.x, c.y, CHECKPOINT_WIDTH, CHECKPOINT_HEIGHT,
			c.targetLevel, c.spawnX, c.spawnY, m_FontTexture));

		EditItem item;
		item.kind = EditItem::Kind::Checkpoint;
		item.checkpointIndex = (int)data.checkpoints.size() - 1;
		data.editItems.push_back(item);
	}

	for (const GuideTextDef& g : def.guideTexts)
	{
		AddGuideText(data, g.x, g.y, g.charW, g.charH, g.text.c_str());
	}

	if (def.hasRespawn)
	{
		SetLevelRespawnPoint(data, def.respawnX, def.respawnY);

		// Appended last so it never shifts the index of any platform/image/
		// checkpoint edit item already saved in platform_overrides.txt.
		EditItem item;
		item.kind = EditItem::Kind::RespawnMarker;
		data.editItems.push_back(item);
	}
}

void LevelManager::Update(float deltaTime)
{
	auto it = m_LevelDatabase.find(m_CurrentLevelID);
	if (it == m_LevelDatabase.end()) return;

	LevelData& current = it->second;

	if (m_PlayerRef)
	{
		m_PlayerRef->Update(deltaTime, current.platforms);

		// While in debug-fly mode the platform editor takes over: edit the
		// layout with the mouse and skip respawn / checkpoint interactions so
		// free-flying past a platform edge or marker has no side effects.
		if (m_PlayerRef->IsDebugFly())
		{
			// The respawn-jump menu takes priority: a click there teleports
			// straight to the chosen level's respawn point, changing
			// m_CurrentLevelID (and invalidating 'current'), so bail out
			// immediately instead of also feeding the click to the editor.
			if (HandleRespawnMenuClick()) return;

			RunEditor(current, deltaTime);
			return;
		}

		// Respawn before any other interaction so the player cannot
		// trigger checkpoints while falling out of the world.
		if (IsPlayerFallenOff(current))
		{
			RespawnPlayer();
			return;
		}

		for (Checkpoint* cp : current.checkpoints)
		{
			if (cp && cp->CheckTrigger(*m_PlayerRef))
			{
				TriggerSeamlessTransition(cp);
				break;
			}
		}
	}
}

void LevelManager::RunEditor(LevelData& current, float deltaTime)
{
	if (m_SaveFlashTimer > 0.0f) m_SaveFlashTimer -= deltaTime;

	const std::vector<PlatformEditor::Box> boxes = BuildEditBoxes(current);
	m_PlatformEditor.Update(m_CurrentLevelID, boxes,
		[this, &current](int index, const PlatformEditor::Box& nb)
		{
			if (index < 0 || index >= (int)current.editItems.size()) return;
			ApplyEditItemBox(current, current.editItems[index], nb);
			m_Overrides[{ m_CurrentLevelID, index }] = nb;
		},
		[this]()
		{
			SaveOverrides();
			m_SaveFlashTimer = 1.5f;   // seconds to show the "SAVED" flash
		});
}

void LevelManager::UpdatePlatformEditor(float deltaTime)
{
	if (!m_PlayerRef) return;

	// Handle the 'A' toggle and (while flying) free camera movement, without
	// running any gameplay physics — safe to call while gameplay is frozen.
	m_PlayerRef->UpdateDebugOnly(deltaTime);

	if (!m_PlayerRef->IsDebugFly()) return;

	auto it = m_LevelDatabase.find(m_CurrentLevelID);
	if (it == m_LevelDatabase.end()) return;

	RunEditor(it->second, deltaTime);
}

bool LevelManager::IsPlayerFallenOff(const LevelData& current) const
{
	if (!m_PlayerRef) return false;

	float maxBottom = -FLT_MAX;
	for (Platform* p : current.platforms)
	{
		if (!p) continue;
		const BoundingBox& b = p->GetCollider();
		maxBottom = std::max(maxBottom, b.y + b.height);
	}

	// No platforms at all: treat anything past one screen below the top as fallen.
	if (maxBottom == -FLT_MAX) maxBottom = SCREEN_HEIGHT;

	return m_PlayerRef->GetCollider().y > maxBottom + FALL_SAFETY_MARGIN;
}

void LevelManager::RespawnPlayer()
{
	if (!m_PlayerRef) return;

	if (m_LevelDatabase.find(m_LastRespawnLevel) != m_LevelDatabase.end())
	{
		m_CurrentLevelID = m_LastRespawnLevel;
	}

	m_PlayerRef->SetPosition(m_LastRespawnX, m_LastRespawnY);
	m_PlayerRef->ResetVelocity();
	RecenterOnPlayer();
}

void LevelManager::RecenterOnPlayer()
{
	const BoundingBox& playerBounds = m_PlayerRef->GetCollider();
	const XMFLOAT2 currentScroll = GetScrollOffset();
	const float targetScrollX = playerBounds.GetCenterX() - SCREEN_WIDTH / 2.0f;
	const float targetScrollY = playerBounds.GetCenterY() - SCREEN_HEIGHT / 2.0f;
	UpdateScrollOffset(targetScrollX - currentScroll.x, targetScrollY - currentScroll.y);
}

std::vector<int> LevelManager::GetRespawnLevelIDs() const
{
	std::vector<int> ids;
	for (const auto& kv : m_LevelDatabase)
	{
		if (kv.second.respawnPoint) ids.push_back(kv.first);
	}
	std::sort(ids.begin(), ids.end());
	return ids;
}

void LevelManager::JumpToLevelRespawn(int levelID)
{
	if (!m_PlayerRef) return;

	auto it = m_LevelDatabase.find(levelID);
	if (it == m_LevelDatabase.end() || !it->second.respawnPoint) return;

	m_CurrentLevelID = levelID;
	AdoptLevelRespawnPoint(levelID);

	m_PlayerRef->SetPosition(m_LastRespawnX, m_LastRespawnY);
	m_PlayerRef->ResetVelocity();
	RecenterOnPlayer();
}

bool LevelManager::HandleRespawnMenuClick()
{
	const std::vector<int> ids = GetRespawnLevelIDs();

	float mx, my;
	const bool haveMouse = GetMouseScreenPos(mx, my);
	const bool down = haveMouse && GetKeyPress(VK_LBUTTON);
	const bool pressed = down && !m_RespawnMenuMouseDown;
	m_RespawnMenuMouseDown = down;

	if (!pressed) return false;

	for (size_t i = 0; i < ids.size(); ++i)
	{
		const float bx = RESPAWN_BTN_X;
		const float by = RESPAWN_BTN_Y0 + (float)i * (RESPAWN_BTN_H + RESPAWN_BTN_GAP);
		if (mx >= bx && mx <= bx + RESPAWN_BTN_W &&
			my >= by && my <= by + RESPAWN_BTN_H)
		{
			JumpToLevelRespawn(ids[i]);
			return true;
		}
	}
	return false;
}

void LevelManager::RenderRespawnMenu() const
{
	if (!m_UITexture) return;

	const std::vector<int> ids = GetRespawnLevelIDs();

	DrawScreenText("RESPAWN JUMP", RESPAWN_BTN_X, RESPAWN_BTN_Y0 - 40.0f,
		RESPAWN_CHAR_W, RESPAWN_CHAR_H);

	for (size_t i = 0; i < ids.size(); ++i)
	{
		const float bx = RESPAWN_BTN_X;
		const float by = RESPAWN_BTN_Y0 + (float)i * (RESPAWN_BTN_H + RESPAWN_BTN_GAP);
		const bool isCurrent = (ids[i] == m_CurrentLevelID);

		const XMFLOAT4 color = isCurrent
			? XMFLOAT4{ 0.2f, 0.6f, 1.0f, 0.9f }   // blue: current level
			: XMFLOAT4{ 0.15f, 0.15f, 0.15f, 0.75f }; // dark: other levels

		// Fixed HUD position: scr=false, same as DrawScreenText, so the
		// button stays put on screen regardless of camera scroll.
		DrawSprite(false, m_UITexture,
			{ bx + RESPAWN_BTN_W * 0.5f, by + RESPAWN_BTN_H * 0.5f },
			{ RESPAWN_BTN_W, RESPAWN_BTN_H }, 0.0f, color);

		char label[16];
		snprintf(label, sizeof(label), "L%d", ids[i]);
		const float textW = (float)strlen(label) * RESPAWN_CHAR_W;
		DrawScreenText(label,
			bx + (RESPAWN_BTN_W - textW) * 0.5f,
			by + (RESPAWN_BTN_H - RESPAWN_CHAR_H) * 0.5f,
			RESPAWN_CHAR_W, RESPAWN_CHAR_H);
	}
}

void LevelManager::RenderRespawnMarker(const LevelData& data) const
{
	if (!m_RespawnMarkerTexture || !data.respawnPoint) return;

	float spawnX, spawnY;
	data.respawnPoint->GetSpawnPosition(spawnX, spawnY);

	// spawnX/Y are the player's top-left landing position; center the
	// bubble marker over where the player's body will be.
	const XMFLOAT2 center = {
		spawnX + PLAYER_WIDTH * 0.5f,
		spawnY + PLAYER_HEIGHT * 0.5f };

	DrawSprite(true, m_RespawnMarkerTexture, center,
		{ PLAYER_WIDTH * 1.4f, PLAYER_HEIGHT * 1.4f });
}

void LevelManager::Render() const
{
	auto it = m_LevelDatabase.find(m_CurrentLevelID);
	if (it == m_LevelDatabase.end()) return;

	// Draw image decorations first so collider-backed artwork sits behind
	// gameplay markers (checkpoints, guide text).
	for (const ImageDecoration& d : it->second.decorations)
	{
		if (d.texture)
		{
			DrawSprite(true, d.texture,
				XMFLOAT2{ d.centerX, d.centerY },
				XMFLOAT2{ d.width, d.height });
		}
	}

	for (Platform* p : it->second.platforms)
	{
		if (p) p->Render();
	}
	for (Checkpoint* c : it->second.checkpoints)
	{
		if (c) c->Render();
	}
	for (GuideText* g : it->second.guideTexts)
	{
		if (g) g->Render();
	}

	// Editor highlight overlay (only shows a selection while debug-fly is on).
	if (m_PlayerRef && m_PlayerRef->IsDebugFly())
	{
		m_PlatformEditor.Render(BuildEditBoxes(it->second));
		RenderRespawnMenu();
		RenderRespawnMarker(it->second);

		// Confirmation flash after a successful save (S key).
		if (m_SaveFlashTimer > 0.0f)
		{
			const char* msg = "SAVED";
			constexpr float charW = 40.0f;
			constexpr float charH = 60.0f;
			const float textW = (float)strlen(msg) * charW;
			DrawScreenText(msg, (SCREEN_WIDTH - textW) * 0.5f, 80.0f, charW, charH);
		}
	}
}

void LevelManager::DrawScreenText(const char* text, float x, float y,
	float charW, float charH) const
{
	if (!m_FontTexture || !text) return;

	constexpr int   kGrid = 16;       // 16x16 ASCII atlas.
	constexpr int   kAsciiBase = 32;
	constexpr float kCellUV = 1.0f / (float)kGrid;

	for (int i = 0; text[i] != '\0'; ++i)
	{
		const unsigned char ch = (unsigned char)text[i];
		if (ch < kAsciiBase) continue;

		const int index = ch - kAsciiBase;
		const int col = index % kGrid;
		const int row = index / kGrid;

		const XMFLOAT2 texPos = { col * kCellUV, row * kCellUV };
		const XMFLOAT2 texScale = { kCellUV, kCellUV };
		const XMFLOAT2 center = { x + (i + 0.5f) * charW, y + charH * 0.5f };
		const XMFLOAT2 size = { charW, charH };

		DrawSpriteTexCoord(false, m_FontTexture, center, size, texPos, texScale);
	}
}

void LevelManager::TriggerSeamlessTransition(Checkpoint* triggerCP)
{
	if (!triggerCP || !m_PlayerRef) return;

	int nextLevel = triggerCP->GetTargetLevel();
	auto it = m_LevelDatabase.find(nextLevel);
	if (it == m_LevelDatabase.end())
	{
		// TODO: Enter here after Level 2 is added. For now this is a noop.
		return;
	}

	m_CurrentLevelID = nextLevel;

	// Entering a new level may bring in a built-in respawn point.
	AdoptLevelRespawnPoint(m_CurrentLevelID);

	// If the new level has its own respawn point (the draggable bubble),
	// land there regardless of which checkpoint sent the player here --
	// entry no longer depends on the two levels' geometry lining up.
	// Levels without one keep the old seamless (no-teleport) behavior.
	if (it->second.respawnPoint)
	{
		m_PlayerRef->SetPosition(m_LastRespawnX, m_LastRespawnY);
		m_PlayerRef->ResetVelocity();
		RecenterOnPlayer();
	}
}

void LevelManager::AdoptLevelRespawnPoint(int levelID)
{
	auto it = m_LevelDatabase.find(levelID);
	if (it == m_LevelDatabase.end()) return;

	const RespawnPoint* rp = it->second.respawnPoint;
	if (!rp) return;

	rp->GetSpawnPosition(m_LastRespawnX, m_LastRespawnY);
	m_LastRespawnLevel = levelID;
}
