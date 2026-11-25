#pragma once


enum SCENE
{
	SCENE_NONE,
	SCENE_TITLE,
	SCENE_GAME,
	SCENE_RESULT,
};


void InitManager();
void UninitManager();
void UpdateManager(float deltaTime);
void DrawManager();

void SetScene(SCENE Scene);
