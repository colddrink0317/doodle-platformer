// ===================================================
// collision.cpp collision checks
// 
// Author:                Date: 2024
// ===================================================
#include "main.h"
#include "collision.h"

// ===================================================
// Bounding box collision check
// 
// Arguments:
//     Rectangle A center position
//     Rectangle B center position
//     Rectangle A size
//     Rectangle B size
//
// Return value
//  true: colliding
//  false: not colliding
// ===================================================
bool CheckBoxCollider(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB)
{
	float ATop =	PosA.y - SizeA.y / 2;	// A top edge.
	float ABottom =	PosA.y + SizeA.y / 2;	// A bottom edge.
	float ARight =	PosA.x + SizeA.x / 2;	// A right edge.
	float ALeft =	PosA.x - SizeA.x / 2;	// A left edge.

	float BTop =	PosB.y - SizeB.y / 2;	// B top edge.
	float BBottom =	PosB.y + SizeB.y / 2;	// B bottom edge.
	float BRight =	PosB.x + SizeB.x / 2;	// B right edge.
	float BLeft =	PosB.x - SizeB.x / 2;	// B left edge.

	if ((ARight >= BLeft) &&		// A right edge >= B left edge.
		(ALeft <= BRight) &&		// A left edge <= B right edge.
		(ABottom >= BTop) &&		// A bottom edge >= B top edge.
		(ATop <= BBottom))			// A top edge <= B bottom edge.
	{
		// Colliding.
		return true;
	}

	// Not colliding.
	return false;
}

// ===================================================
// Bounding circle collision check
// 
// Arguments:
//     Circle 1 center position
//     Circle 2 center position
//     Circle 1 radius
//     Circle 2 radius
//
// Return value
//  true: colliding
//  false: not colliding
// ===================================================
bool CheckCircleCollider(XMFLOAT3 PosA, XMFLOAT3 PosB, float rA, float rB)
{
	// Squared distance between circle A and circle B centers.
	float distance = (PosA.x - PosB.x) * (PosA.x - PosB.x) + (PosA.y - PosB.y) * (PosA.y - PosB.y);
	
	// Squared sum of both radii.
	float rSum = (rA + rB) * (rA + rB);

	// The circles collide when their distance is less than the radius sum.
	if (distance <= rSum)
	{
		// Colliding.
		return true;
	}

	// Not colliding.
	return false;
}

// Checks whether A's top-left point is inside rectangle B.
bool CheckBoxColliderLeftTop(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB)
{
	// Character.
	float ATop = PosA.y - SizeA.y / 2;	// A top edge.
	float ALeft = PosA.x - SizeA.x / 2;	// A left edge.

	// Background.
	float BTop = PosB.y - SizeB.y / 2;	// B top edge.
	float BBottom = PosB.y + SizeB.y / 2;	// B bottom edge.
	float BRight = PosB.x + SizeB.x / 2;	// B right edge.
	float BLeft = PosB.x - SizeB.x / 2;	// B left edge.

	// Is point (ALeft, ATop) inside B's horizontal range?
	if ((ALeft >= BLeft) && (ALeft <= BRight))
	{
		// Vertical range.
		if ((ATop <= BBottom) && (ATop >= BTop))
		{
			// Top-left point is inside.
			return	true;
		}
	}
	return	false;

}

// Checks whether A's top-right point is inside rectangle B.
bool CheckBoxColliderRightTop(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB)
{
	// Character.
	float ATop = PosA.y - SizeA.y / 2;	// A top edge.
//	float ABottom = PosA.y + SizeA.y / 2;	// A bottom edge.
	float ARight = PosA.x + SizeA.x / 2;	// A right edge.
//	float ALeft = PosA.x - SizeA.x / 2;	// A left edge.

	// Background.
	float BTop = PosB.y - SizeB.y / 2;	// B top edge.
	float BBottom = PosB.y + SizeB.y / 2;	// B bottom edge.
	float BRight = PosB.x + SizeB.x / 2;	// B right edge.
	float BLeft = PosB.x - SizeB.x / 2;	// B left edge.

	// Is point (ARight, ATop) inside B's horizontal range?
	if ((ARight >= BLeft) && (ARight <= BRight))
	{
		// Vertical range.
		if ((ATop <= BBottom) && (ATop >= BTop))
		{
			// Top-right point is inside.
			return	true;
		}
	}
	return	false;

}
// Checks whether A's bottom-left point is inside rectangle B.
bool CheckBoxColliderLeftBottom(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB)
{
	// Character.
	//float ATop = PosA.y - SizeA.y / 2;	// A top edge.
	float ABottom = PosA.y + SizeA.y / 2;	// A bottom edge.
	//float ARight = PosA.x + SizeA.x / 2;	// A right edge.
	float ALeft = PosA.x - SizeA.x / 2;	// A left edge.

		// Background.
	float BTop = PosB.y - SizeB.y / 2;	// B top edge.
	float BBottom = PosB.y + SizeB.y / 2;	// B bottom edge.
	float BRight = PosB.x + SizeB.x / 2;	// B right edge.
	float BLeft = PosB.x - SizeB.x / 2;	// B left edge.

	// Is point (ALeft, ABottom) inside B's horizontal range?
	if ((ALeft >= BLeft) && (ALeft <= BRight))
	{
		// Vertical range.
		if ((ABottom <= BBottom) && (ABottom >= BTop))
		{
			// Bottom-left point is inside.
			return	true;
		}
	}
	return	false;

}
// Checks whether A's bottom-right point is inside rectangle B.
bool CheckBoxColliderRightBottom(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB)
{
	// Character.
	//float ATop = PosA.y - SizeA.y / 2;	// A top edge.
	float ABottom = PosA.y + SizeA.y / 2;	// A bottom edge.
	float ARight = PosA.x + SizeA.x / 2;	// A right edge.
	//float ALeft = PosA.x - SizeA.x / 2;	// A left edge.

	// Background.
	float BTop = PosB.y - SizeB.y / 2;	// B top edge.
	float BBottom = PosB.y + SizeB.y / 2;	// B bottom edge.
	float BRight = PosB.x + SizeB.x / 2;	// B right edge.
	float BLeft = PosB.x - SizeB.x / 2;	// B left edge.

	// Is point (ARight, ABottom) inside B's horizontal range?
	if ((ARight >= BLeft) && (ARight <= BRight))
	{
		// Vertical range.
		if ((ABottom <= BBottom) && (ABottom >= BTop))
		{
			// Bottom-right point is inside.
			return	true;
		}
	}
	return	false;
}

bool CheckBoxColliderRight(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB)
{
	// Character.
	//float ATop = PosA.y - SizeA.y / 2;	// A top edge.
	float ABottom = PosA.y;	// A lower-middle Y.
	float ARight = PosA.x + SizeA.x / 2;	// A right edge.
	//float ALeft = PosA.x - SizeA.x / 2;	// A left edge.

	// Background.
	float BTop = PosB.y - SizeB.y / 2;	// B top edge.
	float BBottom = PosB.y + SizeB.y / 2;	// B bottom edge.
	float BRight = PosB.x + SizeB.x / 2;	// B right edge.
	float BLeft = PosB.x - SizeB.x / 2;	// B left edge.

	// Is point (ARight, ABottom) inside B's horizontal range?
	if ((ARight >= BLeft) && (ARight <= BRight))
	{
		// Vertical range.
		if ((ABottom <= BBottom) && (ABottom >= BTop))
		{
			// Right-side point is inside.
			return	true;
		}
	}
	return	false;
}
bool CheckBoxColliderLeft(XMFLOAT3 PosA, XMFLOAT3 PosB, XMFLOAT3 SizeA, XMFLOAT3 SizeB)
{
	// Character.
	//float ATop = PosA.y - SizeA.y / 2;	// A top edge.
	float ABottom = PosA.y;	// A lower-middle Y.
	//float ARight = PosA.x + SizeA.x / 2;	// A right edge.
	float ALeft = PosA.x - SizeA.x / 2;	// A left edge.

	// Background.
	float BTop = PosB.y - SizeB.y / 2;	// B top edge.
	float BBottom = PosB.y + SizeB.y / 2;	// B bottom edge.
	float BRight = PosB.x + SizeB.x / 2;	// B right edge.
	float BLeft = PosB.x - SizeB.x / 2;	// B left edge.

	// Is point (ALeft, ABottom) inside B's horizontal range?
	if ((ALeft >= BLeft) && (ALeft <= BRight))
	{
		// Vertical range.
		if ((ABottom <= BBottom) && (ABottom >= BTop))
		{
			// Left-side point is inside.
			return	true;
		}
	}
	return	false;
}

