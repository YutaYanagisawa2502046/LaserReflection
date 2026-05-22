#pragma once
#include "Scene.h"
class TitleScene : public Scene
{
public:
    TitleScene() {}
    virtual ~TitleScene() {}

    virtual void Initialize() override;
    virtual SceneName Update() override;
    virtual void Draw() override;
};

