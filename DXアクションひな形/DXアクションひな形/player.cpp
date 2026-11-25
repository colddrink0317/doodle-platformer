

#include "main.h"
#include "Renderer.h"
#include "Player.h"
#include "sprite.h"
#include "Input.h"
#include	"Block.h"
#include "collision.h"

// PC and Switch use different coordinate systems, so adjust before reuse.
#define	START_POSITION_X ((SCREEN_WIDTH / 4))		// Rough initial X position above the ground.
#define	START_POSITION_Y (SCREEN_HEIGHT-80.0f-40.0f)	// Rough initial Y position.
#define	PLAYER_SIZE_X	(80.0f)// Rough player width.
#define	PLAYER_SIZE_Y	(80.0f)// Rough player height.
#define	GROUND			(SCREEN_HEIGHT - 50.0f)// Rough ground Y below the initial position.
#define	GRAVITY			(9.8f/60.0f*6.0f)		// Rough gravity acceleration.

// Scroll when the player leaves this coordinate range.
#define		SCROLL_LIMIT_UP		(SCREEN_HEIGHT/4)
#define		SCROLL_LIMIT_DOWN	(SCREEN_HEIGHT - (SCREEN_HEIGHT/4))
#define		SCROLL_LIMIT_LEFT	(SCREEN_WIDTH/4)
#define		SCROLL_LIMIT_RIGHT	(SCREEN_WIDTH - (SCREEN_WIDTH/4))



//bool	CollisionBlock();


PLAYER	Player;

void	PlayerIdle();
void	PlayerWalk();
void	PlayerJump();
void	PlayerFall();


PLAYER* GetPlayer()
{
	return &Player;
}

void	InitPlayer()
{

	ZeroMemory(&Player, sizeof(PLAYER));

	TexMetadata		metadata;
	ScratchImage	image;
	LoadFromWICFile(L"Asset\\Texture\\Block.png", WIC_FLAGS_NONE, &metadata, image);
	// Pass loaded image data to DirectX and manage it as a texture.
	CreateShaderResourceView(GetDevice(), image.GetImages(),
		image.GetImageCount(), metadata, &Player.TexID);
	// Warn if texture creation failed.
	assert(Player.TexID);

	Player.Use = true;
	Player.DrawSize = XMFLOAT3(PLAYER_SIZE_X, PLAYER_SIZE_Y,0);
	Player.CollisionSize = XMFLOAT3(PLAYER_SIZE_X * 0.8f, PLAYER_SIZE_Y * 0.8f,0);
	Player.Position = XMFLOAT3(START_POSITION_X, START_POSITION_Y, 0);
	Player.ViewPosition = XMFLOAT3(START_POSITION_X, START_POSITION_Y, 0);
	Player.Mode = PLAYER_IDLE;

	// Initial physics values.
	Player.Speed = XMFLOAT3(0, 0, 0);					// Initialize movement speed.
	Player.Kasoku = XMFLOAT3(0, GRAVITY, 0);	// Set Y-only acceleration.

	Player.Mode = PLAYER_FALL;

}
void	UninitPlayer()
{
	Player.TexID->Release();
}

void	UpdatePlayer()
{

	switch (Player.Mode)
	{
		case PLAYER_IDLE:
			PlayerIdle(); break;
		case PLAYER_WALK:
			PlayerWalk(); break;
		case PLAYER_JUMP:
			PlayerJump(); break;
		case PLAYER_FALL:
			PlayerFall(); break;
	}
	{
		XMFLOAT2	Scroll = GetScrollOffset();// Current scroll offset.
		Player.ViewPosition.x = Player.Position.x - Scroll.x;// Calculate relative position.
		Player.ViewPosition.y = Player.Position.y - Scroll.y;

		float	ofsx = 0.0f;
		float	ofsy = 0.0f;
		if (Player.ViewPosition.x < SCROLL_LIMIT_LEFT)// Relative coordinate passed the scroll limit.
		{
			ofsx = Player.ViewPosition.x - SCROLL_LIMIT_LEFT;// Calculate offset.
		}
		else if (Player.ViewPosition.x > SCROLL_LIMIT_RIGHT)
		{
			ofsx = Player.ViewPosition.x - SCROLL_LIMIT_RIGHT;
		}
		if (Player.ViewPosition.y < SCROLL_LIMIT_UP)
		{
			ofsy = Player.ViewPosition.y - SCROLL_LIMIT_UP;
		}
		else if (Player.ViewPosition.y > SCROLL_LIMIT_DOWN)
		{
			ofsy = Player.ViewPosition.y - SCROLL_LIMIT_DOWN;
		}

		UpdateScrollOffset(ofsx, ofsy);// Scroll by the offset amount.

	}

	return;



}
void	DrawPlayer()
{
	// Matrix setup.
	//SetWorldViewProjection2D();

	XMFLOAT2	position = XMFLOAT2(Player.ViewPosition.x, Player.ViewPosition.y);// Draw using relative coordinates.
	XMFLOAT2	size = XMFLOAT2(Player.DrawSize.x, Player.DrawSize.y);
	XMFLOAT4	color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	DrawSprite(false, Player.TexID, position, { 80.0f, 80.0f });// Player ignores sprite scrolling.
}

//bool	CollisionBlock()
//{
//	BLOCK* Block = GetBlock();
//
//	for (int i = 0; i < BLOCK_MAX; i++)
//	{
//		if (Player.Use && Block[i].Use)
//		{
//
//			//			bool hit = CheckBoxCollider(Player.Position, Block[i].Position, Player.DrawSize, Block[i].DrawSize);
//			//			bool hit = CheckBoxColliderLeftBottom(Player.Position, Block[i].Position, Player.DrawSize, Block[i].DrawSize);
//			bool hit = CheckBoxColliderRightTop(Player.Position, Block[i].Position, Player.DrawSize, Block[i].DrawSize);
//			if (hit == true) return true;
//		}
//
//
//
//
//	}
//
//
//
//
//
//}



void	PlayerIdle()// Idle state.
{

	Player.Speed.x = 0.0f;	// Stop.
	Player.Speed.y = 0.0f;	// Stop.

	if (GetKeyPress(VK_LEFT))		// Move left.
	{
		Player.Mode = PLAYER_WALK;
	}
	else if (GetKeyPress(VK_RIGHT))// Move right.
	{
		Player.Mode = PLAYER_WALK;
	}
	if (GetKeyTrigger('Z'))			// Jump handling.
	{
		Player.Speed.y = -20.0f;	// Set movement speed.
		Player.Kasoku.y = GRAVITY;	// Set acceleration.
		Player.Mode = PLAYER_JUMP;	// Switch to jump.
	}


}

void	PlayerWalk()// Walking.
{
	if (GetKeyPress(VK_LEFT))		// Move left.
	{
		Player.Speed.x = -5.0f;
	}
	else if (GetKeyPress(VK_RIGHT))// Move right.
	{
		Player.Speed.x = 5.0f;
	}
	else
	{
		Player.Speed.x *= 0.8f;		// Decelerate when left/right are not pressed.
		if (fabs(Player.Speed.x) < 0.05f)
		{
			Player.Speed.x = 0.0f;	// Switch to idle after stopping.
			Player.Mode = PLAYER_IDLE;
		}
	}

	// Check all platforms against six points.
	BLOCK* Block = GetBlock();
	bool	hit1 = false;
	bool	hit2 = false;
	bool	hit3 = false;
	bool	hit4 = false;
	bool	hit5 = false;
	bool	hit6 = false;
	for (int i = 0; i < BLOCK_MAX; i++)
	{
		if (Player.Use && Block[i].Use)
		{
			hit1 |= CheckBoxColliderLeftBottom(Player.Position, Block[i].Position, Player.DrawSize, Block[i].DrawSize);
			hit2 |= CheckBoxColliderRightBottom(Player.Position, Block[i].Position, Player.DrawSize, Block[i].DrawSize);
			hit3 |= CheckBoxColliderLeftTop(Player.Position, Block[i].Position, Player.DrawSize, Block[i].DrawSize);
			hit4 |= CheckBoxColliderRightTop(Player.Position, Block[i].Position, Player.DrawSize, Block[i].DrawSize);
			hit5 |= CheckBoxColliderLeft(Player.Position, Block[i].Position, Player.DrawSize, Block[i].DrawSize);
			hit6 |= CheckBoxColliderRight(Player.Position, Block[i].Position, Player.DrawSize, Block[i].DrawSize);
		}
	}
	if ((hit1 == false) && (hit2 == false))// Not fully standing on a platform.
	{
		Player.Speed.y = 0.0f;			// Reset movement speed.
		Player.Kasoku.y = GRAVITY;		// Reset acceleration.
		Player.Mode = PLAYER_FALL;		// Switch to falling.
	}

	if ((hit5 == true) && (Player.Speed.x < 0.0f))	// Colliding on the left.
	{
		Player.Speed.x = 0.0f;		// Reset movement speed.
	}
	else if ((hit6 == true) && (Player.Speed.x > 0.0f))	// Colliding on the right.
	{
		Player.Speed.x = 0.0f;		// Reset movement speed.
	}


	Player.Position.x += Player.Speed.x;	// Update coordinates.
	Player.Position.y += Player.Speed.y;
	Player.Position.z += Player.Speed.z;


	if (GetKeyTrigger('Z'))			// Jump handling.
	{
		Player.Speed.y = -20.0f;	// Set movement speed.
		Player.Kasoku.y = GRAVITY;	// Set acceleration.
		Player.Mode = PLAYER_JUMP;	// Switch to jump.
	}

}
void	PlayerJump()// Jumping.
{
	if (GetKeyPress(VK_LEFT))		// Move left.
	{
		Player.Speed.x = -5.0f;
	}
	else if (GetKeyPress(VK_RIGHT))// Move right.
	{
		Player.Speed.x = 5.0f;
	}
	else
	{
		Player.Speed.x *= 0.8f;		// Decelerate when left/right are not pressed.
		if (fabs(Player.Speed.x) < 0.05f)
		{
			Player.Speed.x = 0.0f;
		}
	}

	Player.Speed.y += Player.Kasoku.y;		// Add acceleration to speed.

	if (Player.Speed.y > 0.0f)				// Movement speed is pointing downward.
	{
		Player.Mode = PLAYER_FALL;			// Switch to falling.
	}

	Player.Position.x += Player.Speed.x;	// Update coordinates.
	Player.Position.y += Player.Speed.y;
	Player.Position.z += Player.Speed.z;

}
void	PlayerFall()// Falling.
{

	if (GetKeyPress(VK_LEFT))		// Move left.
	{
		Player.Speed.x = -5.0f;
	}
	else if (GetKeyPress(VK_RIGHT))// Move right.
	{
		Player.Speed.x = 5.0f;
	}
	else
	{
		Player.Speed.x *= 0.8f;		// Decelerate when left/right are not pressed.
		if (fabs(Player.Speed.x) < 0.05f)
		{
			Player.Speed.x = 0.0f;
		}
	}

	Player.Speed.y += Player.Kasoku.y;		// Add acceleration to speed.
	if (Player.Speed.y > 78.0f)
	{
		Player.Speed.y = 78.0f;	// Acceleration limit.
	}
	Player.Position.x += Player.Speed.x;	// Update coordinates.
	Player.Position.y += Player.Speed.y;
	Player.Position.z += Player.Speed.z;


	// Platform check.
	BLOCK* Block = GetBlock();
	for (int i = 0; i < BLOCK_MAX; i++)
	{
		if (Player.Use && Block[i].Use)
		{
			bool hit1 = CheckBoxColliderLeftBottom(Player.Position, Block[i].Position, Player.DrawSize, Block[i].DrawSize);
			bool hit2 = CheckBoxColliderRightBottom(Player.Position, Block[i].Position, Player.DrawSize, Block[i].DrawSize);
			if ( (hit1 == true)||(hit2==true))
			{	// Correct the display position onto the platform.
				float	pos = -Player.DrawSize.y /2;
				pos += Block[i].Position.y - (Block[i].DrawSize.y / 2);
				Player.Position.y = pos;

				Player.Speed.y = 0.0f;		// Reset movement speed.
				Player.Kasoku.y = 0.0f;		// Reset acceleration.

				Player.Mode = PLAYER_WALK;	// Switch to walking.
				break;
			}
		}
	}

	return;

}
