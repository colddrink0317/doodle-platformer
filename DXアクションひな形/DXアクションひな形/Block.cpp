

/*
	When platform processing is ready, use block type IDs
	the same way the player uses modes.
	Example types:
	  1. Normal block
	  2. Moving block
	  3. Trap block
	Trap blocks may be invisible but can still push the player back.
	Add type IDs to the block placement array and store them
	inside each block structure.

	Put each behavior in its own function and dispatch from Update()
	with a switch statement. Collision results can branch the same way.

*/


#include "main.h"
#include "Renderer.h"
#include "Block.h"
#include "sprite.h"


BLOCK	Block[BLOCK_MAX];

ID3D11ShaderResourceView* g_Texture[10]; 

BLOCK_MAP	BlockMap[] =
{	// Sample placement and sizes.
	{0,XMFLOAT3(SCREEN_WIDTH/2,SCREEN_HEIGHT-40,0.0f), XMFLOAT3(80.0f * 15, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f,SCREEN_HEIGHT - 40 - 80*2,0.0f), XMFLOAT3(80.0f * 4, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f,SCREEN_HEIGHT - 40 - 80 * 4,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f,SCREEN_HEIGHT - 40 - 80 * 6,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f-80*1,SCREEN_HEIGHT - 40 - 80 * 8,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f-80*2,SCREEN_HEIGHT - 40 - 80 * 10,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f-80*3,SCREEN_HEIGHT - 40 - 80 * 12,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},

	{0,XMFLOAT3(900.0f-80*3,SCREEN_HEIGHT - 40 - 80 * 14,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 3,SCREEN_HEIGHT - 40 - 80 * 16,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 3,SCREEN_HEIGHT - 40 - 80 * 18,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 3,SCREEN_HEIGHT - 40 - 80 * 20,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 3,SCREEN_HEIGHT - 40 - 80 * 22,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},

	{0,XMFLOAT3(900.0f - 80 * 0,SCREEN_HEIGHT - 40 - 80 * 24,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 3,SCREEN_HEIGHT - 40 - 80 * 26,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 0,SCREEN_HEIGHT - 40 - 80 * 28,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 3,SCREEN_HEIGHT - 40 - 80 * 30,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 0,SCREEN_HEIGHT - 40 - 80 * 32,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},

	{0,XMFLOAT3(900.0f - 80 * 3,SCREEN_HEIGHT - 40 - 80 * 34,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 0,SCREEN_HEIGHT - 40 - 80 * 36,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 3,SCREEN_HEIGHT - 40 - 80 * 38,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 4,SCREEN_HEIGHT - 40 - 80 * 40,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 6,SCREEN_HEIGHT - 40 - 80 * 42,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 4,SCREEN_HEIGHT - 40 - 80 * 44,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},
	{0,XMFLOAT3(900.0f - 80 * 6,SCREEN_HEIGHT - 40 - 80 * 46,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},

//	{3,XMFLOAT3(900.0f - 80 * 8,SCREEN_HEIGHT - 40 - 80 * 48,0.0f), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},




//	{0,XMFLOAT3(500.0f,SCREEN_HEIGHT - 40 - 80,0), XMFLOAT3(80.0f * 2, 80.0f,0.0f)},


	{0,XMFLOAT3(-999.0f,500.0f,0.0f), XMFLOAT3(80.0f, 80.0f,0.0f)},

};

BLOCK* GetBlock()
{
	return &Block[0];
}


void	InitBlock()
{

	ZeroMemory(&g_Texture[0], sizeof(ID3D11ShaderResourceView*) * 10);


	TexMetadata		metadata;
	ScratchImage	image;
	LoadFromWICFile(L"Asset\\Texture\\Block.png", WIC_FLAGS_NONE, &metadata, image);
	// Pass loaded image data to DirectX and manage it as a texture.
	CreateShaderResourceView(GetDevice(), image.GetImages(),
		image.GetImageCount(), metadata, &g_Texture[0]);
	// Warn if texture creation failed.
	assert(g_Texture[0]);

	LoadFromWICFile(L"Asset\\Texture\\Goal.jpg", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(GetDevice(), image.GetImages(),
		image.GetImageCount(), metadata, &g_Texture[1]);
	// Warn if texture creation failed.
	assert(g_Texture[1]);


	ZeroMemory(&Block[0], sizeof(BLOCK) * BLOCK_MAX);

	for(int i = 0; i < BLOCK_MAX; i++)
	{
		if (BlockMap[i].Position.x == -999.0f) break;
		if (Block[i].Use == false)
		{
			Block[i].Position = BlockMap[i].Position;
			Block[i].DrawSize = BlockMap[i].DrawSize;
			Block[i].Use = true;
			Block[i].No = BlockMap[i].no;	// Block type.
			Block[i].TexID = g_Texture[0];
		}
	}


}
void	UninitBlock()
{
	g_Texture[0]->Release();
	g_Texture[1]->Release();
}

void	UpdateBlock()
{

	for (int i = 0; i < BLOCK_MAX; i++)
	{
		if (Block[i].Use == true)
		{
			switch (Block[i].No)// Process each block type.
			{
			case BLOCK_NORMAL:	// Normal platform.
			case BLOCK_MOVE:	// Moving platform.
			case BLOCK_TRAP_0:	// Invisible trap 0.
				break;
			}
		}

	}




}
void	DrawBlock()
{
	// Matrix setup.
	//SetWorldViewProjection2D();

	XMFLOAT2	texcoord(1.0f / 8 * 1, 1.0f / 4 * 1);
	XMFLOAT2	texsize(1.0f / 8 * 1, 1.0f / 4 * 1);

	for (int i = 0; i < BLOCK_MAX; i++)
	{
		if (Block[i].Use == true)
		{
			XMFLOAT2	scroll = GetScrollOffset();// Current scroll offset.
			Block[i].ViewPosition.x = Block[i].Position.x-scroll.x;// Calculate relative position.
			Block[i].ViewPosition.y = Block[i].Position.y-scroll.y;
	
			XMFLOAT2	position = XMFLOAT2(Block[i].Position.x, Block[i].Position.y);
			//			XMFLOAT2	size = XMFLOAT2(Block[i].Position.x, Block[i].Position.y);
			XMFLOAT4	color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

			if (Block[i].No == 0)
			{
				DrawSpriteTexCoord(true, Block[i].TexID, position, { Block[i].DrawSize.x   , Block[i].DrawSize.y }, texcoord, texsize);
			}
			else
			{
				DrawSprite(true, Block[i].TexID, position, { Block[i].DrawSize.x   , Block[i].DrawSize.y });
			}
		}
	}

}

