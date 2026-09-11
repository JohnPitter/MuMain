#pragma once

class BMD;
class OBJECT;

namespace Render::Items::Celestial
{
    void RenderWingSurface(BMD* model, OBJECT* object);
    void RenderHaloGlow(BMD* model, OBJECT* object);
    void RenderMaterialAccents(BMD* model, OBJECT* object, int level, float alpha);
}
