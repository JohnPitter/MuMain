#include "stdafx.h"

#include "UI/Chat/VipBadge.h"

#include "Core/Globals/_TextureIndex.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Textures/ZzzOpenglUtil.h"

namespace UI::Vip::Badge
{
    namespace
    {
        // The owner's screenshot showed the badge rendering much larger than
        // the gens rank mark it sits next to. Cause: the v4 art fills the whole
        // render box, while the gens mark's art carries its own padding inside
        // the 50x69 atlas cell (the painted mark is only ~70% of the cell), so
        // a badge drawn at the full 50x69*0.8 = 40x55 box reads ~1.4x wider and
        // taller than the neighbouring gens seal. Render at 30x42 -- a bit
        // smaller than the gens mark's visual -- still wide enough to read as
        // a badge at nameplate size.
        constexpr float kBadgeWidth = 30.0f;
        constexpr float kBadgeHeight = 42.0f;
    }

    void Render(float rightEdgeX, float topY, float bottomY)
    {
        // The badge texture loads late (OpenBasicData, during the load scene).
        // A nameplate can render before that in the LoadWorld window -- skip
        // the badge instead of drawing with an unloaded bitmap.
        if (Bitmaps.FindTexture(BITMAP_VIP_MARK) == nullptr)
        {
            return;
        }

        const float renderY = (bottomY - topY - kBadgeHeight) / 2.0f + topY;
        RenderBitmap(BITMAP_VIP_MARK, rightEdgeX, renderY, kBadgeWidth, kBadgeHeight);
    }
}
