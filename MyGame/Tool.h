#pragma once

class Tool {

	int holdSpriteAtlasIndex;

	virtual void OnClick() = 0;
	virtual void OnRelease() {};
};