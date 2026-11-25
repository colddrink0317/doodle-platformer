#include "Main.h"
#include "Renderer.h"
#include "Sprite.h"


ID3D11Buffer* g_VertexBuffer{};
ID3D11VertexShader* g_VertexShader{};
ID3D11PixelShader* g_PixelShader{};
ID3D11InputLayout* g_VertexLayout{};

static	XMFLOAT2	ScrollOffset;

void	UpdateScrollOffset(float x, float y)
{
	ScrollOffset.x += x;
	ScrollOffset.y += y;
}

void	ResetScrollOffset()
{
	ScrollOffset.x = 0.0f;
	ScrollOffset.y = 0.0f;
}

void InitSprite()
{
	VERTEX_3D vertex[4];

	vertex[0].Position = XMFLOAT3(-0.5f, -0.5f, 0.0f);
	vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

	vertex[1].Position = XMFLOAT3(0.5f, -0.5f, 0.0f);
	vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

	vertex[2].Position = XMFLOAT3(-0.5f, 0.5f, 0.0f);
	vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

	vertex[3].Position = XMFLOAT3(0.5f, 0.5f, 0.0f);
	vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);


	// Create vertex buffer.
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex;

	GetDevice()->CreateBuffer(&bd, &sd, &g_VertexBuffer);


	CreateVertexShader(&g_VertexShader, &g_VertexLayout, "Shader\\UnlitTextureVS.cso");
	CreatePixelShader(&g_PixelShader, "Shader\\UnlitTexturePS.cso");

	ScrollOffset.x = 0.0f;
	ScrollOffset.y = 0.0f;


}

void UninitSprite()
{
	g_VertexBuffer->Release();
	g_VertexLayout->Release();
	g_VertexShader->Release();
	g_PixelShader->Release();


}


XMFLOAT2	GetScrollOffset()
{
	return ScrollOffset;
}
void DrawSprite(bool scr, ID3D11ShaderResourceView* Texture, XMFLOAT2 Position, XMFLOAT2 Scale, float Rotation, XMFLOAT4 Color)
{
	if (scr)
	{
		Position.x -= ScrollOffset.x;
		Position.y -= ScrollOffset.y;
	}
	DrawSpriteTexCoord(false, Texture, Position, Scale, { 0.0f, 0.0f }, { 1.0f, 1.0f }, Rotation, Color);

}


void DrawSpriteTexCoord(bool scr,ID3D11ShaderResourceView* Texture, XMFLOAT2 Position, XMFLOAT2 Scale, XMFLOAT2 TexPosition, XMFLOAT2 TexScale, float Rotation, XMFLOAT4 Color)
{
	if (scr)
	{
		Position.x -= ScrollOffset.x;
		Position.y -= ScrollOffset.y;
	}

	// Set input layout.
	GetDeviceContext()->IASetInputLayout(g_VertexLayout);

	// Set shaders.
	GetDeviceContext()->VSSetShader(g_VertexShader, NULL, 0);
	GetDeviceContext()->PSSetShader(g_PixelShader, NULL, 0);


	// Set matrices.
	SetWorldViewProjection2D();

	// Set world matrix.
	XMMATRIX world, scale, rot, trans;
	scale = XMMatrixScaling(Scale.x, Scale.y, 1.0f);
	rot = XMMatrixRotationZ(Rotation);
	trans = XMMatrixTranslation(Position.x, Position.y, 0.0f);
	world = scale * rot * trans;
	SetWorldMatrix(world);


	// Set vertex buffer.
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	GetDeviceContext()->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);

	// Set texture.
	GetDeviceContext()->PSSetShaderResources(0, 1, &Texture);



	// Set material.
	MATERIAL material;
	ZeroMemory(&material, sizeof(material));
	material.Diffuse = Color;
	material.TextureEnable = true;
	SetMaterial(material);


	// Set texture coordinates.
	TEXCOORD texcoord{};
	texcoord.Position = TexPosition;
	texcoord.Scale = TexScale;
	SetTexCoord(texcoord);


	// Set primitive topology.
	GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Draw polygon.
	GetDeviceContext()->Draw(4, 0);


}
