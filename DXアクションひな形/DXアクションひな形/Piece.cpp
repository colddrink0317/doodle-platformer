#include "Main.h"
#include "Renderer.h"
#include "Manager.h"
#include "Sprite.h"
#include "Piece.h"
#include "Block.h"
#include "Input.h"
#include "Audio.h"


static ID3D11ShaderResourceView* g_Texture[4]{};

struct PIECE
{
	XMFLOAT2 Position;
	int Type[3];
};

static PIECE g_Piece{};



enum PIECE_STATE
{
	PIECE_STATE_IDLE,	// Idle.
	PIECE_STATE_MOVE,	// Falling.
	PIECE_STATE_GROUND_IDLE,	// Landing wait.
	PIECE_STATE_MISS_IDLE,		// Miss wait.
};

static PIECE_STATE g_PieceState{};
static int g_PieceStateCount{};


static int g_PieceAudio{};


void InitPiece()
{
	// Load textures.
	TexMetadata metadata;
	ScratchImage image;

	LoadFromWICFile(L"Asset\\Texture\\Spade.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &g_Texture[0]);
	assert(g_Texture[0]);

	LoadFromWICFile(L"Asset\\Texture\\Clover.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &g_Texture[1]);
	assert(g_Texture[1]);

	LoadFromWICFile(L"Asset\\Texture\\Heart.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &g_Texture[2]);
	assert(g_Texture[2]);

	LoadFromWICFile(L"Asset\\Texture\\Diamond.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &g_Texture[3]);
	assert(g_Texture[3]);



	g_PieceAudio = LoadAudio("Asset\\Audio\\wan.wav");

	CreatePiece();
}



void UninitPiece()
{
	UnloadAudio(g_PieceAudio);



	for (int i = 0; i < 4; i++)
	{
		g_Texture[i]->Release();
	}
}



void UpdatePiece()
{

	switch (g_PieceState)
	{
	case PIECE_STATE_IDLE:	// Idle.
		break;

	case PIECE_STATE_MOVE:	// Falling.
		MovePiece();
		break;

	case PIECE_STATE_GROUND_IDLE:// Landing wait.
		g_PieceStateCount++;
		if (g_PieceStateCount > 60)// Wait after landing.
		{
			g_PieceState = PIECE_STATE_IDLE;// Switch to idle.
			g_PieceStateCount = 0;

			//EraseBlock();// Erase blocks.
		}
		break;

	case PIECE_STATE_MISS_IDLE:// Miss.
		g_PieceStateCount++;
		if (g_PieceStateCount > 60)// Wait.
		{
			SetScene(SCENE_RESULT);
		}
		break;

	default:
		break;
	}

}





void CreatePiece()
{
	g_Piece.Position = { SCREEN_WIDTH * 0.5f + PIECE_WIDTH * 0.5f, 0.0f };// Spawn position.
	for (int i = 0; i < 3; i++)
	{
		g_Piece.Type[i] = rand() % 4;// Pick three piece types.
	}

	// Calculate the array position of the bottom piece.
	// (posX-Offset) / PICE_WIDTH
	// (posY-Offset + (PICE_HEIGHT*3)) / PICE_HEIGHT
	int x = (int)((g_Piece.Position.x - SCREEN_WIDTH * 0.5f + (BLOCK_COLS * 0.5f - 0.5f) * PIECE_WIDTH) / PIECE_WIDTH);
	int y = (int)((g_Piece.Position.y + PIECE_HEIGHT * 3 - SCREEN_HEIGHT * 0.5f + (BLOCK_ROWS * 0.5f - 0.5f) * PIECE_HEIGHT) / PIECE_HEIGHT);

	//BLOCK block = GetBlock(x, y);	// Get block information.

	//if (block.Enable)// Miss if the bottom piece overlaps a block on spawn.
	//{
	//	g_PieceState = PIECE_STATE_MISS_IDLE;
	//	g_PieceStateCount = 0;
	//}
	//else// Otherwise start falling.
	//{
	//	g_PieceState = PIECE_STATE_MOVE;
	//	g_PieceStateCount = 0;
	//}

}


void MovePiece()
{

	if (GetKeyTrigger(VK_UP))// Rotate pieces.
	{
		int type = g_Piece.Type[0];

		for (int i = 0; i < 2; i++)
			g_Piece.Type[i] = g_Piece.Type[i + 1];

		g_Piece.Type[2] = type;
	}




	int x, y;
	BLOCK block;


	y = (int)((g_Piece.Position.y + PIECE_HEIGHT * 3 - SCREEN_HEIGHT * 0.5f + (BLOCK_ROWS * 0.5f - 0.5f) * PIECE_HEIGHT) / PIECE_HEIGHT);

	// Move left.
	if (GetKeyTrigger(VK_LEFT))
	{	// X array position.
		x = (int)((g_Piece.Position.x - PIECE_WIDTH - SCREEN_WIDTH * 0.5f + (BLOCK_COLS * 0.5f - 0.5f) * PIECE_WIDTH) / PIECE_WIDTH);

		if (x >= 0)// Inside the array.
		{
			block = GetBlock(x, y);

			if (block.Enable == false)// Move left if the adjacent cell is empty.
				g_Piece.Position.x -= PIECE_WIDTH;
		}
	}
	// Move right.
	if (GetKeyTrigger(VK_RIGHT))
	{
		x = (int)((g_Piece.Position.x + PIECE_WIDTH - SCREEN_WIDTH * 0.5f + (BLOCK_COLS * 0.5f - 0.5f) * PIECE_WIDTH) / PIECE_WIDTH);

		if (x < BLOCK_COLS)
		{
			block = GetBlock(x, y);

			if (block.Enable == false)// Move right if the adjacent cell is empty.
				g_Piece.Position.x += PIECE_WIDTH;
		}
	}

	if (GetKeyPress(VK_DOWN))
	{
		g_Piece.Position.y += 5.0f;// Accelerated fall.
	}


	g_Piece.Position.y += 2.0f;	// Default fall.




	// Landing check.

	x = (int)((g_Piece.Position.x - SCREEN_WIDTH * 0.5f + (BLOCK_COLS * 0.5f - 0.5f) * PIECE_WIDTH) / PIECE_WIDTH);
	y = (int)((g_Piece.Position.y + PIECE_HEIGHT * 3 - SCREEN_HEIGHT * 0.5f + (BLOCK_ROWS * 0.5f - 0.5f) * PIECE_HEIGHT) / PIECE_HEIGHT);

	block = GetBlock(x, y);




	bool ground = false;

	if (block.Enable)// Landed on a block.
	{
		ground = true;
	}
	else if (g_Piece.Position.y + PIECE_HEIGHT * 2 > SCREEN_HEIGHT * 0.5f + (BLOCK_ROWS * 0.5f - 0.5f) * PIECE_HEIGHT)
	{// Landed on the bottom, y >= 13.
		ground = true;
	}


	if (ground)// Landed.
	{
		for (int i = 0; i < 3; i++)
		{
			// Place pieces into the array.
			x = (int)((g_Piece.Position.x - SCREEN_WIDTH * 0.5f + (BLOCK_COLS * 0.5f - 0.5f) * PIECE_WIDTH) / PIECE_WIDTH);
			y = (int)((g_Piece.Position.y + PIECE_HEIGHT * i - SCREEN_HEIGHT * 0.5f + (BLOCK_ROWS * 0.5f - 0.5f) * PIECE_HEIGHT) / PIECE_HEIGHT);

			SetBlock(x, y, g_Piece.Type[i]);
		}


		PlayAudio(g_PieceAudio);


		// Switch to landing wait.
		g_PieceState = PIECE_STATE_GROUND_IDLE;
		g_PieceStateCount = 0;

	}
}




void DrawPiece()
{
	// Do nothing unless moving or in miss state.
	if (g_PieceState != PIECE_STATE_MOVE && g_PieceState != PIECE_STATE_MISS_IDLE)
		return;


	for (int i = 0; i < 3; i++)
	{
		// Draw the three pieces from top to bottom.
		DrawSprite(g_Texture[g_Piece.Type[i]],
					{ g_Piece.Position.x, g_Piece.Position.y + PIECE_HEIGHT * i },
					{ PIECE_WIDTH, PIECE_HEIGHT });
	}
}
