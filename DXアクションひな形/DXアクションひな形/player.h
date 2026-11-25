#pragma once
//field.h

enum PLAYER_MODE
{
	PLAYER_IDLE = 0,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_FALL,

};



struct PLAYER
{
	bool		Use;
	XMFLOAT3	Position;		// Absolute position in game world coordinates.
	XMFLOAT3	ViewPosition;	// Relative position in screen coordinates.
	XMFLOAT3	DrawSize;		// Polygon size used for drawing.
	XMFLOAT3	CollisionSize;	// Size used for collision checks.


	XMFLOAT3	Speed;	// Movement speed.
	XMFLOAT3	Kasoku;	// Acceleration.

	PLAYER_MODE	Mode;

	ID3D11ShaderResourceView* TexID;

};


void	InitPlayer();
void	UninitPlayer();
void	UpdatePlayer();
void	DrawPlayer();


PLAYER* GetPlayer();
