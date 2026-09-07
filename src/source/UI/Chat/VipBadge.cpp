#include "stdafx.h"

#include "UI/Chat/VipBadge.h"

#include "Core/Globals/_TextureIndex.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Textures/ZzzOpenglUtil.h"

namespace UI::Vip::Badge
{
    namespace
    {
        // Same on-screen footprint as one gens rank badge: CNewUIGensRanking's
        // GENSMARK_WIDTH/HEIGHT (50x69) scaled by its 0.8 "boolean" nameplate
        // size, so the VIP badge reads at the same visual weight as the gens
        // mark next to a name.
        constexpr float kGensMarkWidth = 50.0f;
        constexpr float kGensMarkHeight = 69.0f;
        constexpr float kNameplateScale = 0.8f;
        constexpr float kBadgeWidth = kGensMarkWidth * kNameplateScale;
        constexpr float kBadgeHeight = kGensMarkHeight * kNameplateScale;
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
