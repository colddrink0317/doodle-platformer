#pragma once

// ===================================================
// LevelManager.h
//   Prototype level manager. It owns:
//     - Platform and Checkpoint collections for each level
//     - A non-owning reference to the player
//   Three simple prototype levels with transition flow left in place.
// ===================================================

#include "Main.h"
#include "Platform.h"
#include "Checkpoint.h"
#include "GuideText.h"
#include "PlayerObject.h"
#include "RespawnPoint.h"
#include "PlatformEditor.h"

#include <vector>
#include <unordered_map>
#include <string>
#include <map>
#include <utility>
#include <functional>

class LevelManager
{
private:
	// ---------------------------------------------------------------
	// Pure-data level description. A level's content (platforms,
	// checkpoints, guide text, respawn) is declared as data and consumed
	// by a single generic builder (BuildLevel), instead of one hand-written
	// BuildLevelN() per level. These structs hold no GPU resources; the
	// builder supplies textures from m_BlockTexture / m_FontTexture.
	// ---------------------------------------------------------------
	struct PlatformDef        // -> AddSurfacePlatform
	{
		float x = 0.0f;
		float y = 0.0f;
		float width = 0.0f;   // 0 means "use PLATFORM_WIDTH default"
	};

	struct ImagePlatformDef   // -> AddImagePlatforms
	{
		const wchar_t* path = nullptr;
		float centerX = 0.0f;
		float topY = 0.0f;
		float width = 0.0f;
	};

	struct CheckpointDef      // -> new Checkpoint(...)
	{
		float x = 0.0f;
		float y = 0.0f;
		int   targetLevel = 0;
		float spawnX = 0.0f;
		float spawnY = 0.0f;
	};

	struct GuideTextDef       // -> AddGuideText
	{
		float x = 0.0f;
		float y = 0.0f;
		float charW = 0.0f;
		float charH = 0.0f;
		std::string text;
	};

	struct LevelDef
	{
		int levelID = 0;
		std::vector<PlatformDef>      platforms;
		std::vector<ImagePlatformDef> imagePlatforms;
		std::vector<CheckpointDef>    checkpoints;
		std::vector<GuideTextDef>     guideTexts;
		bool  hasRespawn = false;
		float respawnX = 0.0f;
		float respawnY = 0.0f;
	};

	// A full-image sprite rendered for its original appearance (no tiling).
	// Its collision is provided separately by CollisionPlatform colliders.
	struct ImageDecoration
	{
		ID3D11ShaderResourceView* texture = nullptr; // owned; released on clear
		float centerX = 0.0f;
		float centerY = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
	};

	// One unit the platform editor can grab. A standalone surface platform is
	// a single collider; an image platform is a whole group (its decoration
	// sprite plus the contiguous run of CollisionPlatform colliders it spawned)
	// so the editor moves / resizes the entire image at once.
	struct EditItem
	{
		enum class Kind { Platform, ImageGroup, Checkpoint, RespawnMarker } kind = Kind::Platform;
		int platformIndex = -1;     // Platform: index into platforms.
		int decorationIndex = -1;   // ImageGroup: index into decorations (-1 if none).
		int colliderBegin = 0;      // ImageGroup: [begin, end) range into platforms.
		int colliderEnd = 0;
		int checkpointIndex = -1;   // Checkpoint: index into checkpoints.
		// RespawnMarker: none needed, a level has at most one (data.respawnPoint).
	};

	struct LevelData
	{
		std::vector<Platform*> platforms;
		std::vector<Checkpoint*> checkpoints;
		std::vector<GuideText*> guideTexts;
		std::vector<ImageDecoration> decorations;
		std::vector<EditItem> editItems;
		// Built-in respawn marker for this level (data only, may be null).
		RespawnPoint* respawnPoint = nullptr;
	};

	std::unordered_map<int, LevelData> m_LevelDatabase;
	int m_CurrentLevelID;
	PlayerObject* m_PlayerRef;

	ID3D11ShaderResourceView* m_BlockTexture;
	ID3D11ShaderResourceView* m_FontTexture;

	// Last respawn the player has activated. Defaults to the
	// initial spawn point on Level 1 until the player picks one up.
	int   m_LastRespawnLevel;
	float m_LastRespawnX;
	float m_LastRespawnY;

	// In-game debug tool for repositioning / resizing platforms (active while
	// the player is in debug-fly mode). Persists edits across launches.
	PlatformEditor m_PlatformEditor;

	// Counts down after a successful save so Render can flash a confirmation.
	float m_SaveFlashTimer;

	// Debug-only respawn-jump menu: one button per level that has a built-in
	// respawn point, shown while the player is in debug-fly mode. Clicking a
	// button teleports the player straight to that level's respawn point.
	ID3D11ShaderResourceView* m_UITexture; // 1x1 white pixel for button backgrounds.
	bool m_RespawnMenuMouseDown;

	// Marks each built-in respawn point's world position with bubble.png while
	// debug-fly editing is active (respawn points are otherwise invisible).
	ID3D11ShaderResourceView* m_RespawnMarkerTexture;

	// User edits, keyed by (levelID, editItemIndex). The stored box is the
	// item's whole-unit box (a collider for a platform, the sprite box for an
	// image group); reloaded and re-applied on the next launch.
	std::map<std::pair<int, int>, PlatformEditor::Box> m_Overrides;

public:
	LevelManager();
	~LevelManager();

	void Init(PlayerObject* player);
	void Uninit();

	void PreloadLevels();

	void Update(float deltaTime);
	void Render() const;

	// Runs only the debug platform editor (toggle + edit), without any
	// gameplay. Lets the editor be used while gameplay is frozen, e.g. on the
	// title / intro screen.
	void UpdatePlatformEditor(float deltaTime);

	// True while the platform editor is active (player in debug-fly mode).
	bool IsEditing() const { return m_PlayerRef && m_PlayerRef->IsDebugFly(); }

	int GetCurrentLevelID() const { return m_CurrentLevelID; }

private:
	// Builds all level definitions as data, then consumes them.
	std::vector<LevelDef> CreateLevelDefinitions() const;
	void BuildLevel(const LevelDef& def);
	void AddSurfacePlatform(LevelData& data, float x, float y);
	void AddSurfacePlatform(LevelData& data, float x, float y, float width);
	void AddDefaultSurfacePlatform(LevelData& data);

	// Applies any saved platform-editor overrides on top of the freshly
	// built (procedurally generated) level layout.
	void ApplyPlatformOverrides();
	void LoadOverrides();
	void SaveOverrides() const;

	// Editor glue: the box an edit item currently occupies, and applying a new
	// box back onto the item's geometry (a single collider, or a whole image
	// group transformed uniformly about its top-left).
	PlatformEditor::Box GetEditItemBox(const LevelData& data, const EditItem& item) const;
	void ApplyEditItemBox(LevelData& data, const EditItem& item,
		const PlatformEditor::Box& box);
	std::vector<PlatformEditor::Box> BuildEditBoxes(const LevelData& data) const;

	// Drives the platform editor for one level's geometry (shared by the
	// in-game update and the title-screen editor path).
	void RunEditor(LevelData& current, float deltaTime);

	// Draws a string from the shared ASCII font atlas in screen space.
	void DrawScreenText(const char* text, float x, float y,
		float charW, float charH) const;

	// Loads an image, grid-segments it into platform rectangles, and adds
	// them to the level. The image's ink bounding box is scaled to
	// targetWidth and positioned with its top edge at targetTopY and its
	// horizontal center at targetCenterX.
	void AddImagePlatforms(LevelData& data, const wchar_t* path,
		float targetCenterX, float targetTopY, float targetWidth);
	void AddGuideText(LevelData& data, float x, float y,
		float charW, float charH, const char* text);
	void SetLevelRespawnPoint(LevelData& data, float spawnX, float spawnY);
	void ClearLevels();
	void TriggerSeamlessTransition(Checkpoint* triggerCP);

	// If the level identified by levelID has a built-in respawn point,
	// store it as the new "last respawn" for the player.
	void AdoptLevelRespawnPoint(int levelID);

	bool IsPlayerFallenOff(const LevelData& current) const;
	void RespawnPlayer();

	// Debug respawn-jump menu: level IDs with a built-in respawn point (sorted
	// ascending), the button click test, its render, and the teleport itself.
	std::vector<int> GetRespawnLevelIDs() const;
	bool HandleRespawnMenuClick();
	void RenderRespawnMenu() const;
	void JumpToLevelRespawn(int levelID);

	// Draws the bubble.png marker at the current level's built-in respawn
	// point (no-op if the level has none).
	void RenderRespawnMarker(const LevelData& data) const;

	// Recenters the camera scroll on the player's current position. Shared by
	// RespawnPlayer() and JumpToLevelRespawn().
	void RecenterOnPlayer();
};
