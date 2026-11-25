// ===================================================
// collision.h collision checks
// 
// Author:                Date: 2024
// ===================================================
#ifndef _COLLISION_H_
#define _COLLISION_H_

#include	"Main.h"

// ===================================================
// Function prototypes
// ===================================================
bool CheckBoxCollider(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB);	// Bounding box collision check.
bool CheckCircleCollider(XMFLOAT3 PosA, XMFLOAT3 PosB, float rA, float rB);		// Bounding circle collision check.

bool CheckBoxColliderRightBottom(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB);
bool CheckBoxColliderRightTop(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB);
bool CheckBoxColliderLeftBottom(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB);
bool CheckBoxColliderLeftTop(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB);
bool CheckBoxColliderRight(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB);
bool CheckBoxColliderLeft(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB);




#endif

