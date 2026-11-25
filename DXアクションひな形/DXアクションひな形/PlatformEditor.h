#pragma once

// ===================================================
// PlatformEditor.h
//   Generic in-game mouse tool for selecting, moving and resizing
//   axis-aligned boxes. It knows nothing about platforms or image groups:
//   the owner (LevelManager) supplies the current boxes each frame and
//   receives the edited box back through a callback, so a single drag can
//   drive either one collider or a whole image platform (sprite + all of
//   its collision blocks) as one unit.
//
//   Activation: live whenever the player is in debug-fly mode (toggled with
//   the 'A' key). Pan the camera with the arrow keys as usual.
//
//   Mouse:
//     - Left click on a box        -> select it (smallest box under the
//                                     cursor wins, so image blocks select
//                                     the whole image group, not a sub-block).
//     - Drag the body              -> move it.
//     - Drag the bottom-right grab -> resize (anchored at the top-left).
//     - Left click empty space     -> deselect.
//   Keyboard (while the editor is live):
//     - S  -> the owner persists all edits.
// ===================================================

#include "Main.h"

#include <vector>
#include <functional>

class PlatformEditor
{
public:
	struct Box { float x, y, width, height; };

	PlatformEditor();
	~PlatformEditor();

	void Init();
	void Uninit();

	// Screen-space mode: boxes are in fixed screen coordinates (no camera
	// scroll) and outlines draw in screen space. Used for the title-screen UI
	// images. Default is world space (level geometry).
	void SetScreenSpace(bool screenSpace) { m_ScreenSpace = screenSpace; }

	// Per-frame interaction. The owner rebuilds 'boxes' every frame from the
	// live geometry; onEdit(index, newBox) is invoked while the active item is
	// being dragged; onSave() fires when the user presses 'S'.
	void Update(int levelID,
		const std::vector<Box>& boxes,
		const std::function<void(int index, const Box& newBox)>& onEdit,
		const std::function<void()>& onSave);

	// Highlights the selected box plus its resize grab handle.
	void Render(const std::vector<Box>& boxes) const;

private:
	enum class Drag { None, Move, Resize };

	bool GetMouseWorld(float& worldX, float& worldY) const;
	void DrawOutline(const Box& b, const XMFLOAT4& color) const;

	bool  m_ScreenSpace;
	int   m_LevelID;
	int   m_SelectedIndex;
	Drag  m_Drag;
	Box   m_SnapBox;        // Item box captured at drag start.
	float m_GrabX;          // Cursor world position captured at drag start.
	float m_GrabY;
	bool  m_PrevMouseDown;

	ID3D11ShaderResourceView* m_MarkerTex;  // 1x1 white pixel for outlines.
};
