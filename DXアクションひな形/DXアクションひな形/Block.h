#pragma once
//field.h

#include "Renderer.h"
#include "main.h"


#define		BLOCK_MAX	(100)


enum BLOCK_NO
{
	BLOCK_NORMAL = 0,	// Normal block.
	BLOCK_MOVE,			// Moving block.
	BLOCK_TRAP_0,		// Trap block.

};

struct BLOCK
{
	bool		Use;
	int			No;
	XMFLOAT3	Position;		// Absolute position in game world coordinates.
	XMFLOAT3	ViewPosition;	// Relative position in screen coordinates.
	XMFLOAT3	DrawSize;		// Polygon size used for drawing.
	XMFLOAT3	CollisionSize;	// Size used for collision checks.
	ID3D11ShaderResourceView* TexID;

};


struct BLOCK_MAP
{
	int			no;
	XMFLOAT3	Position;
	XMFLOAT3	DrawSize;

};


void	InitBlock();
void	UninitBlock();
void	UpdateBlock();
void	DrawBlock();

BLOCK* GetBlock();


