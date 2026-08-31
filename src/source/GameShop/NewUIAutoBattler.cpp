#include "stdafx.h"
#include "I18N/All.h"
#include "UI/NewUI/NewUISystem.h"
#include "GameShop/NewUIAutoBattler.h"
#include "UI/NewUI/NewUICommon.h"
#include "GameShop/MsgBoxIGSCommon.h"
#include "Engine/Object/ZzzInventory.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Object/ZzzOpenData.h"
#include "UI/Legacy/UIControls.h"
#include "Audio/DSPlaySound.h"
#include "World/MapInfra/MapManager.h"
#include "MUHelper/MuHelper.h"
#include "Dotnet/Connection.h"
#include "Core/Input/KeyState.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Models/ZzzBMD.h"
#include "Core/Math/ZzzMathLib.h"
#include "Render/Core/GlobalUBO.h"
#include "Render/Core/RenderConfig.h"
#include "Render/Core/ImmediateRenderer.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Camera/CameraProjection.h"
#include "Camera/CameraState.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <list>

using namespace SEASON3B;

extern CameraState g_Camera;
extern Connection* SocketClient;
extern float BoneScale;

namespace
{
    constexpr int kBackW = 580;
    constexpr int kBackH = 460;
    constexpr int kHeaderH = 64;   // newui_item_back01.tga native height (MsgBoxIGSCommon IMAGE_IGS_UP_HEIGHT)
    constexpr int kCatX = 14;
    constexpr int kCatY = 70;
    constexpr int kCatW = 54;   // newui_btn_empty_very_small.tga native state size (54x69, 3 states of 23)
    constexpr int kCatH = 23;
    constexpr int kCatGap = 3;
    constexpr int kViewW = 108;
    constexpr int kViewH = 29;
    constexpr int kGridCols = 3;
    constexpr int kGridCount = 9;
    constexpr int kSlotDX = 100;
    constexpr int kSlotDY = 92;
    constexpr int kNameX = 96;
    constexpr int kNameY = 150;
    constexpr int kNameW = 96;
    constexpr int kItem3DX = 96;
    constexpr int kItem3DY = 76;
    constexpr int kItem3DW = 96;
    constexpr int kItem3DH = 70;
    constexpr int kInfoX = 410;
    constexpr int kInfoW = 156;
    // Right column: status panel on top, enlarged drop list in the middle, Activate button in
    // its own strip below the list, right-aligned and inside the frame.
    constexpr int kStatusY = 68;
    constexpr int kStatusH = 66;
    constexpr int kLootY = 140;
    constexpr int kLootH = 248;         // 22 title + 8 rows x 28 + 2 padding
    constexpr int kLootTitleH = 22;
    constexpr int kLootRowH = 28;       // comfortable row height (was 20)
    constexpr int kLootIcon = 26;       // fixed icon box inside each row (was 20)
    constexpr int kActivateX = kInfoX + kInfoW - kViewW;   // 458: right-aligned in column
    constexpr int kActivateY = 392;     // below the drop list (140+248+4); 392+29=421 < 460
    constexpr float kPreviewPlace = 0.10f;
    constexpr BYTE kGroup = 0xD5;
    constexpr int kFreeQuota = 3600;
    constexpr int kVipQuota = 43200;
    constexpr int kHuntRange = 18;
    constexpr int kObtainRange = 8;
    constexpr int kLootSlots = 8;
    constexpr int kMaxRoamWps = 6;
    constexpr float kBackSrcW = 190.f;
    constexpr float kBackSrcH = 429.f;
    constexpr float kPreviewFov = 1.f;

    struct HuntMob
    {
        const wchar_t* name;
        EMonsterModelType model;
    };

    struct HuntDef
    {
        BYTE world;
        POINT roamWps[kMaxRoamWps];   // real server spawn-spot coordinates (OpenMU
                                      // map initializers: MonsterSpawnArea rects/points)
        HuntMob mobs[5];
        WORD loot[kLootSlots];
    };

    // Roam waypoints are REAL server spawn-spot coordinates taken from the OpenMU
    // map initializers (src/Persistence/Initialization/*/Maps/*.cs — MonsterSpawnArea
    // rectangles and per-monster points). They are static hunt configuration on the
    // client, not a DB coupling.
    const HuntDef kHunts[] = {
        // 178,112 is on the Lorencia city/safe-zone edge. Using it as the
        // nearest roam hub made Auto Battle walk back to town after every kill.
        { WD_0LORENCIA, { { 200, 150 }, { 215, 200 }, { 200, 235 }, { 185, 55 } }, {
            { L"Spider", MONSTER_MODEL_SPIDER },
            { L"Budge Dragon", MONSTER_MODEL_BUDGE_DRAGON },
            { L"Hound", MONSTER_MODEL_HOUND },
            { L"Bull Fighter", MONSTER_MODEL_BULL_FIGHTER },
            { L"Lich", MONSTER_MODEL_LICH },
        }, {
            ITEM_ZEN, ITEM_APPLE, ITEM_SMALL_HEALING_POTION, ITEM_KRIS,
            ITEM_SHORT_SWORD, ITEM_SMALL_AXE, ITEM_LEATHER_ARMOR, ITEM_TOWN_PORTAL_SCROLL
        } },
        { WD_1DUNGEON, { { 105, 225 }, { 90, 240 }, { 68, 210 }, { 150, 212 }, { 62, 180 }, { 110, 200 } }, {
            { L"Skeleton", MONSTER_MODEL_DARK_KNIGHT },
            { L"Hellhound", MONSTER_MODEL_HOUND },
            { L"Thunder Lich", MONSTER_MODEL_LICH },
            { L"Gorgon", MONSTER_MODEL_GORGON },
            { nullptr, MONSTER_MODEL_UNDEFINED },
        }, {
            ITEM_ZEN, ITEM_MEDIUM_HEALING_POTION, ITEM_SERPENT_SWORD, ITEM_SKULL_STAFF,
            ITEM_SCALE_ARMOR, ITEM_JEWEL_OF_BLESS, ITEM_JEWEL_OF_SOUL, ITEM_TOWN_PORTAL_SCROLL
        } },
        { WD_2DEVIAS, { { 170, 80 }, { 235, 60 }, { 225, 215 }, { 195, 165 }, { 60, 180 }, { 60, 60 } }, {
            { L"Yeti", MONSTER_MODEL_YETI },
            { L"Elite Yeti", MONSTER_MODEL_ELITE_YETI },
            { L"Ice Queen", MONSTER_MODEL_ICE_QUEEN },
            { L"Worm", MONSTER_MODEL_WORM },
            { nullptr, MONSTER_MODEL_UNDEFINED },
        }, {
            ITEM_ZEN, ITEM_LARGE_HEALING_POTION, ITEM_MORNING_STAR, ITEM_PLATE_ARMOR,
            ITEM_JEWEL_OF_SOUL, ITEM_JEWEL_OF_BLESS, ITEM_RING_OF_ICE, ITEM_TOWN_PORTAL_SCROLL
        } },
        { WD_3NORIA, { { 140, 60 }, { 210, 60 }, { 230, 110 }, { 60, 60 }, { 60, 180 }, { 200, 180 } }, {
            { L"Goblin", MONSTER_MODEL_GOBLIN },
            { L"Chain Scorpion", MONSTER_MODEL_CHAIN_SCORPION },
            { L"Beetle Monster", MONSTER_MODEL_BEETLE_MONSTER },
            { L"Hunter", MONSTER_MODEL_HUNTER },
            { nullptr, MONSTER_MODEL_UNDEFINED },
        }, {
            ITEM_ZEN, ITEM_SMALL_MANA_POTION, ITEM_SHORT_BOW, ITEM_VINE_ARMOR,
            ITEM_SILK_ARMOR, ITEM_JEWEL_OF_BLESS, ITEM_KRIS, ITEM_TOWN_PORTAL_SCROLL
        } },
        { WD_4LOSTTOWER, { { 164, 60 }, { 220, 110 }, { 195, 135 }, { 191, 120 }, { 10, 103 }, { 195, 240 } }, {
            { L"Death Cow", MONSTER_MODEL_DEATH_COW },
            { L"Devil", MONSTER_MODEL_DEVIL },
            { L"Death Knight", MONSTER_MODEL_DEATH_KNIGHT },
            { L"Shadow", MONSTER_MODEL_SHADOW },
            { nullptr, MONSTER_MODEL_UNDEFINED },
        }, {
            ITEM_ZEN, ITEM_LARGE_HEALING_POTION, ITEM_LEGENDARY_SWORD, ITEM_LEGENDARY_STAFF,
            ITEM_DRAGON_ARMOR, ITEM_JEWEL_OF_CHAOS, ITEM_JEWEL_OF_SOUL, ITEM_TOWN_PORTAL_SCROLL
        } },
        { WD_7ATLANSE, { { 40, 50 }, { 70, 40 }, { 30, 75 }, { 90, 55 }, { 147, 114 }, { 40, 225 } }, {
            { L"Bahamut", MONSTER_MODEL_BAHAMUT },
            { L"Vepar", MONSTER_MODEL_VEPAR },
            { L"Valkyrie", MONSTER_MODEL_VALKYRIE },
            { L"Great Bahamut", MONSTER_MODEL_BAHAMUT },
            { nullptr, MONSTER_MODEL_UNDEFINED },
        }, {
            ITEM_ZEN, ITEM_LARGE_MANA_POTION, ITEM_BILL_OF_BALROG, ITEM_SERPENT_SPEAR,
            ITEM_JEWEL_OF_BLESS, ITEM_JEWEL_OF_SOUL, ITEM_JEWEL_OF_CHAOS, ITEM_TOWN_PORTAL_SCROLL
        } },
        { WD_8TARKAN, { { 150, 45 }, { 135, 85 }, { 60, 70 }, { 55, 125 }, { 130, 210 }, { 160, 190 } }, {
            { L"Iron Wheel", MONSTER_MODEL_GOLDEN_WHEEL },
            { L"Tantalos", MONSTER_MODEL_TANTALLOS },
            { L"Beam Knight", MONSTER_MODEL_BEAM_KNIGHT },
            { L"Zaikan", MONSTER_MODEL_TANTALLOS },
            { nullptr, MONSTER_MODEL_UNDEFINED },
        }, {
            ITEM_ZEN, ITEM_LARGE_HEALING_POTION, ITEM_GIANT_SWORD, ITEM_STAFF_OF_DESTRUCTION,
            ITEM_JEWEL_OF_BLESS, ITEM_JEWEL_OF_SOUL, ITEM_JEWEL_OF_CHAOS, ITEM_TOWN_PORTAL_SCROLL
        } },
        { WD_10HEAVEN, { { 30, 35 }, { 75, 70 }, { 35, 100 }, { 45, 130 }, { 50, 180 }, { 40, 220 } }, {
            { L"Alquamos", MONSTER_MODEL_ALQUAMOS },
            { L"Queen Rainer", MONSTER_MODEL_QUEEN_RAINER },
            { L"Mega Crust", MONSTER_MODEL_CRUST },
            { L"Phantom Knight", MONSTER_MODEL_PHANTOM_KNIGHT },
            { nullptr, MONSTER_MODEL_UNDEFINED },
        }, {
            ITEM_ZEN, ITEM_JEWEL_OF_BLESS, ITEM_JEWEL_OF_SOUL, ITEM_JEWEL_OF_CHAOS,
            ITEM_LARGE_HEALING_POTION, ITEM_TOWN_PORTAL_SCROLL, ITEM_LARGE_MANA_POTION, ITEM_KRIS
        } },
    };

    float s_PreProj[16];
    float s_PreView[16];

    void FormatHms(wchar_t* buffer, int seconds)
    {
        if (seconds < 0)
            seconds = 0;
        const int hours = seconds / 3600;
        const int minutes = (seconds % 3600) / 60;
        const int rest = seconds % 60;
        mu_swprintf(buffer, L"%02d:%02d:%02d", hours, minutes, rest);
    }

    bool IsSupportSkill(int skill)
    {
        switch (skill)
        {
        case AT_SKILL_SWELL_LIFE:
        case AT_SKILL_SWELL_LIFE_STR:
        case AT_SKILL_SWELL_LIFE_PROFICIENCY:
        case AT_SKILL_INFINITY_ARROW:
        case AT_SKILL_INFINITY_ARROW_STR:
        case AT_SKILL_DEFENSE:
        case AT_SKILL_DEFENSE_STR:
        case AT_SKILL_DEFENSE_MASTERY:
        case AT_SKILL_ATTACK:
        case AT_SKILL_ATTACK_STR:
        case AT_SKILL_ATTACK_MASTERY:
        case AT_SKILL_SOUL_BARRIER:
        case AT_SKILL_SOUL_BARRIER_STR:
        case AT_SKILL_SOUL_BARRIER_PROFICIENCY:
        case AT_SKILL_EXPANSION_OF_WIZARDRY:
        case AT_SKILL_EXPANSION_OF_WIZARDRY_STR:
        case AT_SKILL_EXPANSION_OF_WIZARDRY_MASTERY:
        case AT_SKILL_ADD_CRITICAL:
        case AT_SKILL_ADD_CRITICAL_STR1:
        case AT_SKILL_ADD_CRITICAL_STR2:
        case AT_SKILL_ADD_CRITICAL_STR3:
        case AT_SKILL_ALICE_BERSERKER:
        case AT_SKILL_ALICE_BERSERKER_STR:
        case AT_SKILL_ALICE_THORNS:
        case AT_SKILL_HEALING:
        case AT_SKILL_HEALING_STR:
        case AT_SKILL_ATT_UP_OURFORCES:
        case AT_SKILL_HP_UP_OURFORCES:
        case AT_SKILL_DEF_UP_OURFORCES:
        case AT_SKILL_HP_UP_OURFORCES_STR:
        case AT_SKILL_DEF_UP_OURFORCES_MASTERY:
        case AT_SKILL_DEF_UP_OURFORCES_STR:
            return true;
        default:
            return false;
        }
    }

    bool IsCombatSkill(int skill)
    {
        if (skill <= 0 || skill >= MAX_SKILLS)
            return false;
        if (skill >= AT_SKILL_STUN && skill <= AT_SKILL_REMOVAL_BUFF)
            return false;
        if (SkillAttribute != nullptr)
        {
            const BYTE useType = SkillAttribute[skill].SkillUseType;
            if (useType == SKILL_USE_TYPE_MASTER || useType == SKILL_USE_TYPE_MASTERLEVEL)
                return false;
        }
        return !IsSupportSkill(skill);
    }

    void FillHuntSkills(MUHelper::ConfigData& hunt)
    {
        hunt.aiSkill.fill(0);
        hunt.aiSkillCondition.fill(0);
        hunt.aiSkillInterval.fill(0);
        if (CharacterAttribute == nullptr)
            return;

        int slot = 0;
        if (Hero != nullptr && Hero->CurrentSkill < MAX_MAGIC)
        {
            const int selected = CharacterAttribute->Skill[Hero->CurrentSkill];
            if (IsCombatSkill(selected))
            {
                hunt.aiSkill[slot] = static_cast<uint32_t>(selected);
                ++slot;
            }
        }
        for (int i = 0; i < MAX_MAGIC && slot < 3; ++i)
        {
            const int skill = CharacterAttribute->Skill[i];
            if (!IsCombatSkill(skill))
                continue;
            bool already = false;
            for (int s = 0; s < slot; ++s)
            {
                if (hunt.aiSkill[s] == static_cast<uint32_t>(skill))
                {
                    already = true;
                    break;
                }
            }
            if (already)
                continue;
            hunt.aiSkill[slot] = static_cast<uint32_t>(skill);
            if (slot > 0)
            {
                hunt.aiSkillCondition[slot] = static_cast<uint32_t>(MUHelper::ON_TIMER);
                hunt.aiSkillInterval[slot] = 2;
            }
            ++slot;
        }
    }

    int CountHuntMobs(const HuntDef& hunt)
    {
        int count = 0;
        for (int i = 0; i < 5; ++i)
        {
            if (hunt.mobs[i].name != nullptr && hunt.mobs[i].model >= 0)
                ++count;
        }
        return count;
    }

    int CountHuntLoot(const HuntDef& hunt)
    {
        int count = 0;
        for (int i = 0; i < kLootSlots; ++i)
        {
            if (hunt.loot[i] != 0)
                ++count;
        }
        return count;
    }

    void BeginPreviewCamera()
    {
        glViewport2(0, 0, WindowWidth, WindowHeight);
        gluPerspective2(kPreviewFov, (float)WindowWidth / (float)WindowHeight, RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR);

        const float aspect = (float)WindowWidth / (float)WindowHeight;
        const float fovRad = kPreviewFov * 0.5f * Q_PI / 180.0f;
        const float f = 1.0f / tanf(fovRad);
        float cpuProj[16];
        BuildPerspectiveProjection(f, aspect, RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR, cpuProj);
        GlobalUBO::Instance().SetProj(cpuProj);

        // Horizontal frontal view, composed exactly like BeginOpengl() does
        // (cpuView = Ry(A1) * Rx(A0) * Rz(A2) * T(-Pos)):
        //  - A0 = -90 deg pitches the camera up from the engine's rest orientation.
        //    With this composition the view forward is -(third row of R): at A0 = 0 it is
        //    (0, 0, -1), i.e. straight down world -Z — and the models' vertical axis IS +Z
        //    (Quake-style engine). That identity view was the mathematical cause of the
        //    top-down cards; A0 = -90 gives forward = (sin A2, cos A2, 0), horizontal.
        //  - up (second row of R) = (0, 0, 1) = +Z = the model spine, for any yaw: roll 0.
        //  - A2 = 335: with the view forward = (sin A2, -cos A2, 0) (see below), a yaw-0
        //    model faces +Y, so the camera must look along -Y (A2 = 180 would be dead frontal);
        //    A2 = 335 gives forward = (-sin 25, -cos 25, 0) — the same natural 3/4 the old
        //    25-degree model yaw suggested, seen from the FRONT. A2 = 155 mirrored the
        //    horizontal forward (+Y component) and showed every model from BEHIND; adding
        //    180 degrees flips only the horizontal look direction: pitch, up (+Z) and the
        //    horizon are untouched.
        //  - Eye at the origin: each card re-centres the model along its own screen ray
        //    (ScreenToWorldRay below), so no translation is needed.
        constexpr float kPreviewPitchDeg = -90.f;   // horizontal look (0 deg would be top-down)
        constexpr float kPreviewCameraYawDeg = 335.f; // frontal 3/4 (180 = dead frontal); 155 was back-facing
        {
            // Column-major GL matrices, identical layout to the static MakeRotationX/Z helpers
            // in ZzzOpenglUtil.cpp's BeginOpengl().
            const float d = Q_PI / 180.f;
            const float sp = sinf(kPreviewPitchDeg * d), cp = cosf(kPreviewPitchDeg * d);
            const float sy = sinf(kPreviewCameraYawDeg * d), cy = cosf(kPreviewCameraYawDeg * d);
            const float ry[16] = { // Ry(0) — BeginOpengl composes Ry * Rx * Rz * T(-Pos)
                1.f, 0.f, 0.f, 0.f,
                0.f, 1.f, 0.f, 0.f,
                0.f, 0.f, 1.f, 0.f,
                0.f, 0.f, 0.f, 1.f };
            const float rx[16] = { // Rx(pitch)
                1.f, 0.f, 0.f, 0.f,
                0.f, cp, sp, 0.f,
                0.f, -sp, cp, 0.f,
                0.f, 0.f, 0.f, 1.f };
            const float rz[16] = { // Rz(yaw)
                cy, sy, 0.f, 0.f,
                -sy, cy, 0.f, 0.f,
                0.f, 0.f, 1.f, 0.f,
                0.f, 0.f, 0.f, 1.f };
            auto MatMul = [](float* out, const float* a, const float* b) {
                float r[16];
                for (int c = 0; c < 4; ++c)
                    for (int rw = 0; rw < 4; ++rw)
                    {
                        float s = 0.f;
                        for (int k = 0; k < 4; ++k)
                            s += a[k * 4 + rw] * b[c * 4 + k];
                        r[c * 4 + rw] = s;
                    }
                memcpy(out, r, sizeof(r));
            };
            float ryx[16], view[16];
            MatMul(ryx, ry, rx);   // Ry * Rx
            MatMul(view, ryx, rz); // * Rz; eye at origin so the T(-Pos) term is identity

            GlobalUBO::Instance().SetView(view);

            // g_Camera.Matrix layout matches BeginOpengl(): Matrix[i][j] = view[j*4 + i].
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 4; ++j)
                    g_Camera.Matrix[i][j] = view[j * 4 + i];
        }
    }

    void ScissorSlot(float sx, float sy, float width, float height)
    {
        IR::Flush(IR::FlushCause::Other);
        const int px = static_cast<int>(ConvertPosX(sx));
        const int py = static_cast<int>(ConvertPosY(sy));
        const int pw = static_cast<int>(ConvertX(width));
        const int ph = static_cast<int>(ConvertY(height));
        if (pw < 2 || ph < 2)
            return;

        const int inset = 2;
        const int clippedW = pw - inset * 2;
        const int clippedH = ph - inset * 2;
        const int flippedY = static_cast<int>(WindowHeight) - (py + ph - inset);
        SetScissor(px + inset, flippedY, clippedW, clippedH);
        EnableScissorTest();
    }

    void RenderMonsterPreview(float sx, float sy, float width, float height, EMonsterModelType model)
    {
        if (model < 0 || model >= MONSTER_MODEL_COUNT)
            return;

        // Models are loaded by the world (CreateMonster → OpenMonsterModel). Auto Battler
        // does not open packs itself — that path heap-corrupted on UI open (WER 0xc0000374).
        const int type = static_cast<int>(MODEL_MONSTER01) + static_cast<int>(model);
        if (type < 0 || type >= MAX_MODELS || Models == nullptr)
            return;

        BMD* b = &Models[type];
        // Same guard as the engine render paths (ZzzBMD.cpp RenderBody,
        // ZzzObject.cpp RenderObjectScreen): a half-parsed BMD (failed load
        // leaves NumMeshs/NumBones set with incomplete allocations) must never
        // be animated or measured here — that is heap corruption.
        if (!b->m_bCompletedAlloc || b->NumMeshs <= 0 || b->Meshs == nullptr
            || b->NumBones <= 0 || b->NumBones > MAX_BONES || b->Bones == nullptr)
            return;

        ScissorSlot(sx, sy, width, height);

        OBJECT o = {};
        o.Type = type;
        o.Live = true;
        o.Visible = true;
        o.Alpha = 1.f;
        o.LightEnable = false;
        o.EnableBoneMatrix = false;
        o.HiddenMesh = -1;
        o.BlendMesh = -1;
        o.CurrentAction = MONSTER01_STOP1;
        o.PriorAction = MONSTER01_STOP1;
        o.AnimationFrame = 0.f;
        o.PriorAnimationFrame = 0.f;
        o.m_fEdgeScale = 1.2f;
        Vector(1.f, 1.f, 1.f, o.Light);
        // Quake-style Euler (ZzzMathLib::AngleMatrix): angles = (roll X, pitch Y, yaw Z) and the
        // world vertical axis is Z, so yaw-only keeps the model's spine aligned with the card's
        // vertical axis. Pitch/roll stay 0 — the model stands upright; the frontal/3-4 viewing
        // angle comes from BeginPreviewCamera()'s camera yaw (335 deg), not from the model.
        Vector(0.f, 0.f, 0.f, o.Angle);
        Vector(0.f, 0.f, 0.f, o.HeadAngle);

        // Shared BMD state mutated per-render elsewhere: save and restore around the preview so
        // the world render never inherits preview-only values.
        const float prevBoneScale = BoneScale;
        const float prevBodyHeight = b->BodyHeight;
        const float prevBodyScale = b->BodyScale;
        vec3_t prevBodyOrigin;
        VectorCopy(b->BodyOrigin, prevBodyOrigin);

        BoneScale = 1.f;
        b->BodyHeight = 0.f;
        b->BodyScale = 1.f;
        b->CurrentAction = MONSTER01_STOP1;
        Vector(0.f, 0.f, 0.f, b->BodyOrigin);
        if (b->NumActions > 0)
        {
            b->Animation(
                BoneTransform,
                o.AnimationFrame,
                o.PriorAnimationFrame,
                o.PriorAction,
                o.Angle,
                o.HeadAngle,
                false,
                false,
                nullptr,
                static_cast<short>(MONSTER01_STOP1));
        }

        // Measure animation-space bounds. Engine vertical is +Z (see BeginPreviewCamera);
        // card height must follow Z or upright bipeds (Bull/Hound/Lich) get a near-zero
        // Y extent, fail the bboxH>0 gate, and never draw — only flat/sprawling models
        // (Spider) looked filled.
        vec3_t bbMin = { 1e9f, 1e9f, 1e9f };
        vec3_t bbMax = { -1e9f, -1e9f, -1e9f };
        vec3_t corner;
        int boxedBones = 0;
        for (int i = 0; i < b->NumBones; ++i)
        {
            if (!b->Bones[i].BoundingBox)
                continue;
            ++boxedBones;
            for (int v = 0; v < 8; ++v)
            {
                VectorTransform(b->Bones[i].BoundingVertices[v], BoneTransform[i], corner);
                for (int k = 0; k < 3; ++k)
                {
                    if (corner[k] < bbMin[k]) bbMin[k] = corner[k];
                    if (corner[k] > bbMax[k]) bbMax[k] = corner[k];
                }
            }
        }

        const float extentX = bbMax[0] - bbMin[0];
        const float extentY = bbMax[1] - bbMin[1];
        const float extentZ = bbMax[2] - bbMin[2];
        const float bboxW = std::max(extentX, extentY);
        const float bboxH = (extentZ > 1.f) ? extentZ : std::max(extentX, extentY);

        if (boxedBones > 0 && bboxH > 0.f && bboxW > 0.f)
        {
            // World units covered by one screen pixel at the preview distance under the
            // item-view perspective (identical horizontally and vertically because the
            // projection aspect matches the window aspect).
            const float dist = kPreviewPlace * RENDER_ITEMVIEW_FAR;
            const float fovRad = kPreviewFov * 0.5f * Q_PI / 180.0f;
            const float worldPerPx = 2.f * dist * tanf(fovRad) / static_cast<float>(WindowHeight);
            const float maxWorldH = ConvertY(height) * 0.8f * worldPerPx;
            const float maxWorldW = ConvertX(width) * 0.7f * worldPerPx;
            float scale = std::min(maxWorldH / bboxH, maxWorldW / bboxW);
            scale = std::min(scale, 0.25f);

            // Ray through the card centre (native RenderObjectScreen idiom), then recenter the
            // measured bounds on that point so the model sits centred in the card.
            vec3_t target;
            CameraProjection::ScreenToWorldRay(
                g_Camera,
                static_cast<int>(sx + width * 0.50f),
                static_cast<int>(sy + height * 0.55f),
                target,
                false);
            vec3_t direction, base;
            VectorSubtract(target, MousePosition, direction);
            VectorMA(MousePosition, kPreviewPlace, direction, base);
            o.Scale = scale;
            o.Position[0] = base[0] - (bbMin[0] + extentX * 0.5f) * scale;
            o.Position[1] = base[1] - (bbMin[1] + extentY * 0.5f) * scale;
            o.Position[2] = base[2] - (bbMin[2] + extentZ * 0.5f) * scale;

            SetActiveBoneTransform(BoneTransform);

            vec3_t light;
            Vector(1.f, 1.f, 1.f, light);
            RenderPartObject(&o, type, NULL, light, o.Alpha, 0, 0, 0, true, false, true);
        }

        IR::Flush(IR::FlushCause::Other);
        DisableScissorTest();

        BoneScale = prevBoneScale;
        b->BodyHeight = prevBodyHeight;
        b->BodyScale = prevBodyScale;
        VectorCopy(prevBodyOrigin, b->BodyOrigin);
    }

    // Loot icons render under the NATIVE item-3D camera context (CNewUI3DCamera::Render,
    // NewUI3DRenderMng.cpp): identity view, 1-degree fov, full-window viewport. The item
    // pipeline is hard-wired to that context: RenderItem3D places the model via
    // ScreenToWorldRay on g_Camera and RenderObjectScreen sets fixed WORLD-SPACE model
    // angles tuned for an identity view. Under the monster frontal preview camera those
    // same items were seen from an arbitrary angle. Each row uses a 20x20 inventory cell
    // (same as CNewUIInventoryCtrl) centred in the 26x26 icon box, with a per-row scissor.
    void RenderLootIcons(const HuntDef& hunt, int panelX, int panelY)
    {
        const float aspect = (float)WindowWidth / (float)WindowHeight;
        glViewport2(0, 0, WindowWidth, WindowHeight);
        gluPerspective2(1.f, aspect, RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR);

        constexpr float kItemFovDeg = 1.f;
        const float fovRad = kItemFovDeg * 0.5f * Q_PI / 180.0f;
        const float f = 1.0f / tanf(fovRad);
        float cpuProj[16];
        BuildPerspectiveProjection(f, aspect, RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR, cpuProj);
        GlobalUBO::Instance().SetProj(cpuProj);

        static const float kIdentityView[16] = {
            1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
        GlobalUBO::Instance().SetView(kIdentityView);
        static const float kIdentityCameraMatrix[3][4] = {
            { 1.f, 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f, 0.f }, { 0.f, 0.f, 1.f, 0.f } };
        memcpy(g_Camera.Matrix, kIdentityCameraMatrix, sizeof(kIdentityCameraMatrix));

        EnableDepthTest();
        EnableDepthMask();

        // RenderItem3D expects the TOP-LEFT of an inventory cell and then adds class-specific
        // anchors (e.g. sy += Height*0.8). Inventory uses 20x20 squares
        // (CNewUIInventoryCtrl::INVENTORY_SQUARE_*). The previous 20x27 box centered on the
        // row mid-line inflated those anchors (~35%) and dropped every model below its slot;
        // tall gear (armor) looked like a cumulative sink down the list.
        constexpr float kCell = 20.f;
        const float slotX = static_cast<float>(panelX + 6);
        float slotY = static_cast<float>(panelY + kLootTitleH + 2) + 1.f;
        const float cellInset = (static_cast<float>(kLootIcon) - kCell) * 0.5f;
        for (int i = 0; i < kLootSlots; ++i)
        {
            ClearDepthBuffer();
            ScissorSlot(slotX, slotY, static_cast<float>(kLootIcon), static_cast<float>(kLootIcon));
            RenderItem3D(slotX + cellInset, slotY + cellInset, kCell, kCell,
                hunt.loot[i], 0, 0, 0, false);
            slotY += static_cast<float>(kLootRowH);
        }

        IR::Flush(IR::FlushCause::Other);
        DisableScissorTest();
        // ScreenToWorldRay overwrites MousePosition with the camera eye (origin here);
        // refresh it before the caller's restore recomputes the real cursor position.
        UpdateMousePositionn();
    }

    void RenderLootGrid(const HuntDef& hunt, int originX, int originY, int scroll)
    {
        const float aspect = (float)WindowWidth / (float)WindowHeight;
        glViewport2(0, 0, WindowWidth, WindowHeight);
        gluPerspective2(1.f, aspect, RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR);

        constexpr float kItemFovDeg = 1.f;
        const float fovRad = kItemFovDeg * 0.5f * Q_PI / 180.0f;
        const float f = 1.0f / tanf(fovRad);
        float cpuProj[16];
        BuildPerspectiveProjection(f, aspect, RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR, cpuProj);
        GlobalUBO::Instance().SetProj(cpuProj);

        static const float kIdentityView[16] = {
            1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
        GlobalUBO::Instance().SetView(kIdentityView);
        static const float kIdentityCameraMatrix[3][4] = {
            { 1.f, 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f, 0.f }, { 0.f, 0.f, 1.f, 0.f } };
        memcpy(g_Camera.Matrix, kIdentityCameraMatrix, sizeof(kIdentityCameraMatrix));

        EnableDepthTest();
        EnableDepthMask();

        int shown = 0;
        for (int i = scroll; i < kLootSlots && shown < kGridCount; ++i)
        {
            if (hunt.loot[i] == 0)
                continue;
            const float bx = static_cast<float>(originX + ((shown % kGridCols) * kSlotDX));
            const float by = static_cast<float>(originY + ((shown / kGridCols) * kSlotDY));
            ClearDepthBuffer();
            ScissorSlot(bx, by, static_cast<float>(kItem3DW), static_cast<float>(kItem3DH));
            RenderItem3D(bx, by, static_cast<float>(kItem3DW), static_cast<float>(kItem3DH),
                hunt.loot[i], 0, 0, 0, false);
            ++shown;
        }

        IR::Flush(IR::FlushCause::Other);
        DisableScissorTest();
        UpdateMousePositionn();
    }

    void TileWindowBack(float x, float y, float w, float h)
    {
        for (float oy = 0.f; oy < h; oy += kBackSrcH)
        {
            const float th = (oy + kBackSrcH > h) ? (h - oy) : kBackSrcH;
            for (float ox = 0.f; ox < w; ox += kBackSrcW)
            {
                const float tw = (ox + kBackSrcW > w) ? (w - ox) : kBackSrcW;
                RenderImage(CNewUIAutoBattler::IMAGE_AB_BACK, x + ox, y + oy, tw, th);
            }
        }
    }
}

CNewUIAutoBattler* CNewUIAutoBattler::NewInstance()
{
    return new CNewUIAutoBattler();
}

CNewUIAutoBattler::CNewUIAutoBattler()
{
    m_pNewUIMng = nullptr;
    m_Pos.x = m_Pos.y = 0;
    m_iHunt = 0;
    m_iMapPage = 0;
    m_iRemaining = kFreeQuota;
    m_iQuota = kFreeQuota;
    m_bVip = false;
    m_bRunning = false;
    m_bSessionActive = false;
    m_bUiReady = false;
    m_bHelperOverlaid = false;
    m_dwLastTick = 0;
    m_dwLastSync = 0;
    m_bStartConfirmed = false;
    m_dwStartSentTick = 0;
    m_iDropScroll = 0;
    m_iMobScroll = 0;
}

CNewUIAutoBattler::~CNewUIAutoBattler()
{
    Release();
}

bool CNewUIAutoBattler::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == nullptr)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_AUTOBATTLER, this);
    // Death / safe-zone auto-stop: the helper detects the state centrally and
    // delegates the session teardown (stop request, analyzer hide, config
    // restore) to this window's normal stop flow.
    MUHelper::g_MuHelper.SetAutoStopHandler(
        [](const char* szReason)
        {
            if (g_pAutoBattler != nullptr)
                g_pAutoBattler->OnHelperAutoStop(szReason);
        });
    LoadImages();
    SetPos(x, y);
    Show(false);
    Enable(true);
    return true;
}

void CNewUIAutoBattler::Release()
{
    RestoreHelperHunt();
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void CNewUIAutoBattler::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    SetBtnInfo();
}

void CNewUIAutoBattler::LoadImages()
{
    LoadBitmap(L"Interface\\newui_msgbox_back.jpg", IMAGE_AB_BACK, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back01.tga", IMAGE_AB_TOP, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-L.tga", IMAGE_AB_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-R.tga", IMAGE_AB_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_AB_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_btn_empty.tga", IMAGE_AB_BTN, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_btn_empty_small.tga", IMAGE_AB_BTN_SMALL, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_btn_empty_very_small.tga", IMAGE_AB_BTN_MAP, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_table01(L).tga", IMAGE_AB_TABLE_TOP_LEFT);
    LoadBitmap(L"Interface\\newui_item_table01(R).tga", IMAGE_AB_TABLE_TOP_RIGHT);
    LoadBitmap(L"Interface\\newui_item_table02(L).tga", IMAGE_AB_TABLE_BOTTOM_LEFT);
    LoadBitmap(L"Interface\\newui_item_table02(R).tga", IMAGE_AB_TABLE_BOTTOM_RIGHT);
    LoadBitmap(L"Interface\\newui_item_table03(Up).tga", IMAGE_AB_TABLE_TOP_PIXEL);
    LoadBitmap(L"Interface\\newui_item_table03(Dw).tga", IMAGE_AB_TABLE_BOTTOM_PIXEL);
    LoadBitmap(L"Interface\\newui_item_table03(L).tga", IMAGE_AB_TABLE_LEFT_PIXEL);
    LoadBitmap(L"Interface\\newui_item_table03(R).tga", IMAGE_AB_TABLE_RIGHT_PIXEL);
    LoadBitmap(L"Interface\\newui_item_box.tga", IMAGE_AB_ITEMBOX);
}

int CNewUIAutoBattler::HuntCount() const
{
    return static_cast<int>(sizeof(kHunts) / sizeof(kHunts[0]));
}

int CNewUIAutoBattler::MapPageCount() const
{
    return (HuntCount() + kMapSlots - 1) / kMapSlots;
}

int CNewUIAutoBattler::SelectedSlot() const
{
    return m_iHunt - (m_iMapPage * kMapSlots);
}

const wchar_t* CNewUIAutoBattler::HuntMapName(int huntIndex) const
{
    if (huntIndex < 0 || huntIndex >= HuntCount())
        return L"-";
    const wchar_t* name = gMapManager.GetMapName(kHunts[huntIndex].world);
    return (name && name[0]) ? name : L"-";
}

void CNewUIAutoBattler::SetBtnInfo()
{
    m_StartButton.ChangeButtonImgState(true, IMAGE_AB_BTN, true);
    m_StartButton.ChangeButtonInfo(m_Pos.x + kActivateX, m_Pos.y + kActivateY, kViewW, kViewH);
    m_StartButton.MoveTextPos(0, -1);
    RefreshActivateButton();

    m_PrevButton.ChangeButtonImgState(true, IMAGE_AB_BTN_MAP, true);
    m_PrevButton.ChangeButtonInfo(m_Pos.x + kCatX, m_Pos.y + kBackH - 36, 22, 23);
    m_PrevButton.ChangeText(L"<");
    m_NextButton.ChangeButtonImgState(true, IMAGE_AB_BTN_MAP, true);
    m_NextButton.ChangeButtonInfo(m_Pos.x + kCatX + 54, m_Pos.y + kBackH - 36, 22, 23);
    m_NextButton.ChangeText(L">");

    InitMapButtons();
}

void CNewUIAutoBattler::InitMapButtons()
{
    m_MapButton.UnRegisterRadioButton();
    m_MapButton.CreateRadioGroup(kMapSlots, IMAGE_AB_BTN_MAP, true);
    m_MapButton.ChangeRadioButtonInfo(false, m_Pos.x + kCatX, m_Pos.y + kCatY, kCatW, kCatH, kCatGap);
    m_MapButton.ChangeButtonState(SEASON3B::BUTTON_STATE_DOWN, 2);
    m_MapButton.SetFont(g_hFontBold);
    RefreshMapButtons();
}

void CNewUIAutoBattler::RefreshMapButtons()
{
    std::list<std::wstring> texts;
    for (int slot = 0; slot < kMapSlots; ++slot)
    {
        const int huntIndex = (m_iMapPage * kMapSlots) + slot;
        texts.push_back(huntIndex < HuntCount() ? HuntMapName(huntIndex) : L"-");
    }
    m_MapButton.ChangeRadioText(texts);

    const int slot = SelectedSlot();
    if (slot >= 0 && slot < kMapSlots)
        m_MapButton.ChangeFrame(slot);
}

void CNewUIAutoBattler::RefreshActivateButton()
{
    m_StartButton.ChangeText(m_bSessionActive ? &I18N::Game::AutoBattlerStop : &I18N::Game::AutoBattlerActivate);
}

void CNewUIAutoBattler::SelectHunt(int huntIndex)
{
    if (huntIndex < 0 || huntIndex >= HuntCount())
        return;
    m_iHunt = huntIndex;
    m_iMapPage = m_iHunt / kMapSlots;
    m_iDropScroll = 0;
    m_iMobScroll = 0;
    RefreshMapButtons();
    // Deferred: never OpenMonsterModel sync here (WER 0xc0000374 on UI open).
    QueueHuntPreviewModels(huntIndex);
}

namespace
{
    // All 99d hunt maps together are ~25 unique models; keep headroom so browsing
    // every tab never stalls because finished slots still occupy the array.
    constexpr int kPreviewQueueCap = 40;
    EMonsterModelType s_previewQueue[kPreviewQueueCap];
    int s_previewQueueCount = 0;
    int s_previewQueueIndex = 0;
    DWORD s_previewNextTick = 0;

    bool MonsterVisualReady(EMonsterModelType model)
    {
        if (model < 0 || model >= MONSTER_MODEL_COUNT || Models == nullptr)
            return false;
        const int type = static_cast<int>(MODEL_MONSTER01) + static_cast<int>(model);
        if (type < 0 || type >= MAX_MODELS)
            return false;
        const BMD* b = &Models[type];
        return b->m_bCompletedAlloc && b->NumMeshs > 0 && b->Meshs != nullptr
            && b->NumBones > 0 && b->Bones != nullptr;
    }

    void CompactPreviewQueue()
    {
        if (s_previewQueueIndex <= 0)
            return;
        const int remaining = s_previewQueueCount - s_previewQueueIndex;
        for (int i = 0; i < remaining; ++i)
            s_previewQueue[i] = s_previewQueue[s_previewQueueIndex + i];
        s_previewQueueCount = remaining;
        s_previewQueueIndex = 0;
    }

    bool QueueContainsPending(EMonsterModelType model)
    {
        for (int i = s_previewQueueIndex; i < s_previewQueueCount; ++i)
        {
            if (s_previewQueue[i] == model)
                return true;
        }
        return false;
    }

    void EnqueueHuntModels(int huntIndex, bool prioritizeFront)
    {
        if (huntIndex < 0 || huntIndex >= static_cast<int>(sizeof(kHunts) / sizeof(kHunts[0])))
            return;

        CompactPreviewQueue();

        EMonsterModelType toAdd[5];
        int nAdd = 0;
        const HuntDef& hunt = kHunts[huntIndex];
        for (int i = 0; i < 5; ++i)
        {
            const EMonsterModelType model = hunt.mobs[i].model;
            if (model < 0 || hunt.mobs[i].name == nullptr)
                continue;
            if (MonsterVisualReady(model) || QueueContainsPending(model))
                continue;
            toAdd[nAdd++] = model;
        }
        if (nAdd == 0)
            return;

        if (s_previewQueueCount + nAdd > kPreviewQueueCap)
        {
            // Drop oldest pending to make room for the hunt the player is looking at.
            const int need = s_previewQueueCount + nAdd - kPreviewQueueCap;
            if (need >= s_previewQueueCount)
            {
                s_previewQueueCount = 0;
                s_previewQueueIndex = 0;
            }
            else
            {
                for (int i = 0; i < s_previewQueueCount - need; ++i)
                    s_previewQueue[i] = s_previewQueue[i + need];
                s_previewQueueCount -= need;
            }
        }

        if (prioritizeFront && s_previewQueueCount > 0)
        {
            // Shift pending right, insert this hunt's models at the front so the
            // visible cards fill before background leftovers from other tabs.
            for (int i = s_previewQueueCount - 1; i >= 0; --i)
                s_previewQueue[i + nAdd] = s_previewQueue[i];
            for (int i = 0; i < nAdd; ++i)
                s_previewQueue[i] = toAdd[i];
            s_previewQueueCount += nAdd;
        }
        else
        {
            for (int i = 0; i < nAdd; ++i)
                s_previewQueue[s_previewQueueCount++] = toAdd[i];
        }

        g_ErrorReport.Write(L"[AB] preview preload +%d (hunt %d), queue %d pending\r\n",
            nAdd, huntIndex, s_previewQueueCount - s_previewQueueIndex);
        const DWORD now = GetTickCount();
        if (s_previewNextTick == 0 || s_previewNextTick > now + 100)
            s_previewNextTick = now + 100;
    }

    // Isolated SEH frame: no C++ objects with destructors in this function.
    int OpenMonsterVisualSeh(int model)
    {
        __try
        {
            OpenMonsterModel(static_cast<EMonsterModelType>(model), false);
            return 1;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }
}

void CNewUIAutoBattler::QueueHuntPreviewModels(int huntIndex)
{
    EnqueueHuntModels(huntIndex, true);
}

void CNewUIAutoBattler::NotifyWorldLoaded(int world)
{
    s_previewQueueCount = 0;
    s_previewQueueIndex = 0;
    s_previewNextTick = 0;

    if (world < 0)
        return;
    if (world == WD_73NEW_LOGIN_SCENE || world == WD_74NEW_CHARACTER_SCENE)
        return;

    const int hunts = static_cast<int>(sizeof(kHunts) / sizeof(kHunts[0]));
    for (int h = 0; h < hunts; ++h)
    {
        if (kHunts[h].world != static_cast<BYTE>(world))
            continue;
        EnqueueHuntModels(h, false);
        g_ErrorReport.Write(L"[AB] preview preload queued for world %d (%d models)\r\n",
            world, s_previewQueueCount);
        s_previewNextTick = GetTickCount() + 250;
        break;
    }
}

void CNewUIAutoBattler::TickPreviewPreload()
{
    if (s_previewQueueIndex >= s_previewQueueCount)
    {
        // Fully drained — reset so the next SelectHunt starts clean.
        if (s_previewQueueCount > 0 && s_previewQueueIndex >= s_previewQueueCount)
        {
            s_previewQueueCount = 0;
            s_previewQueueIndex = 0;
        }
        return;
    }

    const DWORD now = GetTickCount();
    if (s_previewNextTick != 0 && now < s_previewNextTick)
        return;

    const EMonsterModelType model = s_previewQueue[s_previewQueueIndex++];
    if (MonsterVisualReady(model))
    {
        s_previewNextTick = now + 50;
        return;
    }

    g_ErrorReport.Write(L"[AB] preview preload OpenMonsterModel(%d) %d/%d\r\n",
        static_cast<int>(model), s_previewQueueIndex, s_previewQueueCount);
    if (!OpenMonsterVisualSeh(static_cast<int>(model)))
    {
        g_ErrorReport.Write(L"[AB] preview preload SEH abort model %d — draining queue\r\n",
            static_cast<int>(model));
        s_previewQueueIndex = s_previewQueueCount;
        return;
    }
    // Space BMD/texture allocs; stacking them in one frame was the open-UI crash.
    s_previewNextTick = now + 120;
}

void CNewUIAutoBattler::OpeningProcess()
{
    PlayBuffer(SOUND_CLICK01);
    SetBtnInfo();

    int current = 0;
    for (int i = 0; i < HuntCount(); ++i)
    {
        if (kHunts[i].world == static_cast<BYTE>(gMapManager.WorldActive))
        {
            current = i;
            break;
        }
    }
    SelectHunt(current);
    m_bUiReady = true;
    Enable(true);
    SendStatusRequest();
}

void CNewUIAutoBattler::ClosingProcess()
{
    PlayBuffer(SOUND_CLICK01);
}

void CNewUIAutoBattler::SendStatusRequest()
{
    if (SocketClient == nullptr)
        return;
    BYTE packet[4] = { 0xC1, 4, kGroup, 0x00 };
    SocketClient->Send(packet, 4);
}

void CNewUIAutoBattler::SendStart()
{
    if (SocketClient == nullptr)
        return;
    BYTE packet[5] = { 0xC1, 5, kGroup, 0x01, kHunts[m_iHunt].world };
    SocketClient->Send(packet, 5);
}

void CNewUIAutoBattler::SendStop()
{
    if (SocketClient == nullptr)
        return;
    BYTE packet[4] = { 0xC1, 4, kGroup, 0x02 };
    SocketClient->Send(packet, 4);
}

void CNewUIAutoBattler::ShowNotice(const wchar_t* text)
{
    CMsgBoxIGSCommon* pMsgBox = nullptr;
    CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
    if (pMsgBox)
        pMsgBox->Initialize(I18N::Game::AutoBattlerTitle, text);
}

void CNewUIAutoBattler::ApplyHelperHunt()
{
    MUHelper::ConfigData hunt = m_bHelperOverlaid ? m_HelperBackup : MUHelper::g_MuHelper.GetConfig();
    if (!m_bHelperOverlaid)
    {
        m_HelperBackup = hunt;
        m_bHelperOverlaid = true;
    }

    hunt.iHuntingRange = kHuntRange;
    hunt.iObtainingRange = kObtainRange;
    hunt.bPickAllItems = true;
    hunt.bReturnToOriginalPosition = false;
    hunt.bFallbackBasicAttack = true;
    hunt.bLongRangeCounterAttack = true;
    hunt.bUseCombo = false;
    hunt.bSupportParty = false;
    hunt.bUseDarkRaven = false;
    hunt.bRepairItem = false;
    hunt.bAutoHeal = false;
    hunt.bAutoHealParty = false;
    hunt.bUseHealPotion = false;
    hunt.bUseDrainLife = false;
    hunt.aiBuff.fill(0);
    FillHuntSkills(hunt);
    MUHelper::g_MuHelper.Load(hunt);
    // Safe-zone handling stays native: the helper stops as soon as the hero
    // enters a safe zone / city. The Auto Battler must never hunt there.
    MUHelper::g_MuHelper.SetIgnoreHuntRange(true);
    MUHelper::g_MuHelper.SetRoamWaypoints(kHunts[m_iHunt].roamWps, kMaxRoamWps);

    if (!MUHelper::g_MuHelper.IsActive())
        MUHelper::g_MuHelper.Start();
    else
        MUHelper::g_MuHelper.RecalculateDistances();

    ScanNearbyMonsters();
}

void CNewUIAutoBattler::RestoreHelperHunt()
{
    if (!m_bHelperOverlaid)
        return;
    MUHelper::g_MuHelper.SetIgnoreSafeZoneStop(false);
    MUHelper::g_MuHelper.SetIgnoreHuntRange(false);
    MUHelper::g_MuHelper.SetRoamWaypoints(nullptr, 0);
    MUHelper::g_MuHelper.Load(m_HelperBackup);
    MUHelper::g_MuHelper.RecalculateDistances();
    m_bHelperOverlaid = false;
}

void CNewUIAutoBattler::ScanNearbyMonsters()
{
    if (!MUHelper::g_MuHelper.IsActive() || CharactersClient == nullptr)
        return;

    for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
    {
        CHARACTER* c = &CharactersClient[i];
        if (!c->Object.Live || c->Dead > 0 || c == Hero)
            continue;
        if (!IsMonster(c))
            continue;
        MUHelper::g_MuHelper.AddTarget(c->Key, false);
    }
}

bool CNewUIAutoBattler::InventoryIsFull() const
{
    if (g_pMyInventory == nullptr)
        return false;
    return g_pMyInventory->FindEmptySlotIncludingExtensions(1, 1) == -1;
}

void CNewUIAutoBattler::TryStartHunt()
{
    if (Hero != nullptr && Hero->SafeZone)
    {
        // Never start (or keep) the Auto Battler inside a city/safe zone.
        ShowNotice(I18N::Game::AutoBattlerSafeZone);
        return;
    }

    if (m_iRemaining <= 0)
    {
        ShowNotice(I18N::Game::AutoBattlerNoTime);
        return;
    }

    if (InventoryIsFull())
    {
        ShowNotice(I18N::Game::AutoBattlerInventoryFull);
        return;
    }

    m_bSessionActive = true;
    m_bRunning = true;
    m_dwLastTick = GetTickCount();
    // Hunt Analyzer starts on the server's status reply to Start. If that
    // reply never comes (server build without the D5 handler), the session
    // is confirmed locally after a short grace period (TickSession).
    m_bStartConfirmed = false;
    m_dwStartSentTick = GetTickCount();
    SendStart();
    ApplyHelperHunt();
    RefreshActivateButton();
    g_pNewUISystem->Hide(INTERFACE_AUTOBATTLER);
}

void CNewUIAutoBattler::StopHunt(const char* szReason)
{
    const bool wasActive = m_bSessionActive;
    m_bSessionActive = false;
    m_bRunning = false;
    m_bStartConfirmed = false;
    m_dwStartSentTick = 0;
    if (g_pHuntAnalyzer)
        g_pHuntAnalyzer->NotifySession(false, szReason);
    // Single stop request: skipped when the session was never active or was
    // already stopped (auto-stop guard + wasActive check below).
    if (wasActive)
        SendStop();
    MUHelper::g_MuHelper.SetIgnoreSafeZoneStop(false);
    MUHelper::g_MuHelper.SetIgnoreHuntRange(false);
    if (MUHelper::g_MuHelper.IsActive())
        MUHelper::g_MuHelper.Stop();
    RestoreHelperHunt();
    if (wasActive)
        RefreshActivateButton();
}

// Central auto-stop entry (death / safe-zone), invoked from the helper's
// timer thread on the main loop. Runs the normal stop flow exactly once;
// a respawn in town never restarts the session — a new explicit start is
// required after leaving the safe zone.
void CNewUIAutoBattler::OnHelperAutoStop(const char* szReason)
{
    if (!m_bSessionActive)
        return; // already stopped: no duplicate stop request, no re-hide
    StopHunt(szReason);
}

void CNewUIAutoBattler::ReceiveStatus(const BYTE* buffer)
{
    if (buffer == nullptr)
        return;

    // Any status reply is an authoritative server acknowledgement.
    m_bStartConfirmed = true;
    m_dwStartSentTick = 0;

    memcpy(&m_iRemaining, buffer + 4, 4);
    memcpy(&m_iQuota, buffer + 8, 4);
    const BYTE flags = buffer[12];
    m_bVip = (flags & 0x01) != 0;
    m_bRunning = (flags & 0x02) != 0;
    const BYTE result = buffer[14];

    if (m_iQuota <= 0)
        m_iQuota = m_bVip ? kVipQuota : kFreeQuota;
    if (m_iRemaining < 0)
        m_iRemaining = 0;

    if (result == 1)
    {
        m_bSessionActive = false;
        m_bRunning = false;
        if (g_pHuntAnalyzer)
            g_pHuntAnalyzer->NotifySession(false, "start-refused");
        ShowNotice(I18N::Game::AutoBattlerNoTime);
        MUHelper::g_MuHelper.SetIgnoreSafeZoneStop(false);
        MUHelper::g_MuHelper.SetIgnoreHuntRange(false);
        if (MUHelper::g_MuHelper.IsActive())
            MUHelper::g_MuHelper.Stop();
        RestoreHelperHunt();
        RefreshActivateButton();
        return;
    }

    if (m_bSessionActive)
    {
        m_bRunning = true;
        m_dwLastTick = GetTickCount();
        // Server confirmed the session: capture analyzer baselines now.
        if (g_pHuntAnalyzer)
            g_pHuntAnalyzer->NotifySession(true, "server-status");
        ApplyHelperHunt();
        RefreshActivateButton();
        return;
    }

    m_bSessionActive = m_bRunning;
    if (g_pHuntAnalyzer)
        g_pHuntAnalyzer->NotifySession(m_bRunning, "status-sync");
    if (m_bRunning)
    {
        m_dwLastTick = GetTickCount();
        ApplyHelperHunt();
    }
    RefreshActivateButton();
}

void CNewUIAutoBattler::TickSession()
{
    if (!m_bSessionActive)
        return;

    const DWORD now = GetTickCount();
    if (m_dwLastTick == 0)
        m_dwLastTick = now;

    const DWORD elapsedMs = now - m_dwLastTick;
    if (elapsedMs >= 1000)
    {
        const int elapsed = static_cast<int>(elapsedMs / 1000);
        m_dwLastTick += elapsed * 1000;
        m_iRemaining -= elapsed;
        if (m_iRemaining < 0)
            m_iRemaining = 0;
    }

    ScanNearbyMonsters();

    if (InventoryIsFull())
    {
        StopHunt();
        ShowNotice(I18N::Game::AutoBattlerInventoryFull);
        return;
    }

    if (!MUHelper::g_MuHelper.IsActive() || m_iRemaining <= 0)
    {
        const bool noTime = m_iRemaining <= 0;
        StopHunt();
        if (noTime)
            ShowNotice(I18N::Game::AutoBattlerNoTime);
        return;
    }

    if (now - m_dwLastSync > 10000)
    {
        m_dwLastSync = now;
        SendStatusRequest();
    }

    // Fallback confirmation: the server normally answers C1 D5 01 (start)
    // with a status reply (C1 D5 10) that drives NotifySession(true). If no
    // reply at all arrives within the grace period — e.g. a server build
    // without the D5 handlers — the already-running local session is
    // confirmed so the analyzer is not hidden waiting for a reply that never
    // comes. A late refusal (result == 1) still hides it.
    if (m_bSessionActive && !m_bStartConfirmed
        && m_dwStartSentTick != 0 && now - m_dwStartSentTick > 2500)
    {
        m_bStartConfirmed = true;
        if (g_pHuntAnalyzer)
            g_pHuntAnalyzer->NotifySession(true, "start-confirmed-local");
    }
}

bool CNewUIAutoBattler::Update()
{
    TickSession();
    return true;
}

bool CNewUIAutoBattler::UpdateKeyEvent()
{
    if (!IsVisible())
        return true;

    if (IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(INTERFACE_AUTOBATTLER);
        return false;
    }
    return true;
}

bool CNewUIAutoBattler::UpdateMouseEvent()
{
    if (!IsVisible())
        return true;

    if (BtnProcess())
        return false;

    if (MouseWheel != 0 && m_iHunt >= 0 && m_iHunt < HuntCount())
    {
        const HuntDef& hunt = kHunts[m_iHunt];
        if (CheckMouseIn(m_Pos.x + kItem3DX, m_Pos.y + kItem3DY, kGridCols * kSlotDX, 3 * kSlotDY))
        {
            int hidden = CountHuntLoot(hunt) - kGridCount;
            if (hidden < 0) hidden = 0;
            m_iDropScroll -= MouseWheel;
            if (m_iDropScroll < 0) m_iDropScroll = 0;
            if (m_iDropScroll > hidden) m_iDropScroll = hidden;
            MouseWheel = 0;
            return false;
        }
        if (CheckMouseIn(m_Pos.x + kInfoX, m_Pos.y + kLootY, kInfoW, kLootH))
        {
            int hidden = CountHuntMobs(hunt) - kLootSlots;
            if (hidden < 0) hidden = 0;
            m_iMobScroll -= MouseWheel;
            if (m_iMobScroll < 0) m_iMobScroll = 0;
            if (m_iMobScroll > hidden) m_iMobScroll = hidden;
            MouseWheel = 0;
            return false;
        }
    }

    if (CheckMouseIn(m_Pos.x, m_Pos.y, kBackW, kBackH))
    {
        if (IsPress(VK_LBUTTON) || IsPress(VK_RBUTTON))
            return false;
        return false;
    }
    return true;
}

bool CNewUIAutoBattler::BtnProcess()
{
    // Original header close "X": the gold medallion baked into the right end of
    // newui_item_back01.tga. With the right cap drawn from src texels 162..190 the glyph
    // lands at (x + kBackW - 21, y + 7) — same 13x12 box every shared-frame window uses
    // (CNewUISystem::HandleFrameCornerClose, offset 169 = 190 - 21 on the 190-wide frame).
    constexpr int X_OFFSET_FROM_RIGHT = 21, Y_OFFSET = 7, X_WIDTH = 13, X_HEIGHT = 12;
    if (IsPress(VK_LBUTTON)
        && CheckMouseIn(m_Pos.x + kBackW - X_OFFSET_FROM_RIGHT, m_Pos.y + Y_OFFSET, X_WIDTH, X_HEIGHT))
    {
        g_pNewUISystem->Hide(INTERFACE_AUTOBATTLER);
        // Clear the raw button state so the click does not fall through to world movement.
        MouseLButton = false;
        MouseLButtonPop = false;
        MouseLButtonPush = false;
        return true;
    }

    if (m_StartButton.UpdateMouseEvent())
    {
        if (m_bSessionActive)
            StopHunt();
        else
            TryStartHunt();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    const int mapIndex = m_MapButton.UpdateMouseEvent();
    if (mapIndex != RADIOGROUPEVENT_NONE)
    {
        const int huntIndex = (m_iMapPage * kMapSlots) + m_MapButton.GetCurButtonIndex();
        SelectHunt(huntIndex);
        return true;
    }

    if (MapPageCount() > 1 && m_PrevButton.UpdateMouseEvent() && m_iMapPage > 0)
    {
        --m_iMapPage;
        m_iHunt = m_iMapPage * kMapSlots;
        SelectHunt(m_iHunt);
        return true;
    }

    if (MapPageCount() > 1 && m_NextButton.UpdateMouseEvent() && m_iMapPage + 1 < MapPageCount())
    {
        ++m_iMapPage;
        m_iHunt = m_iMapPage * kMapSlots;
        SelectHunt(m_iHunt);
        return true;
    }

    return false;
}

bool CNewUIAutoBattler::Render()
{
    if (!m_bUiReady || !IsVisible())
        return true;

    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);
    RenderFrame();
    RenderButtons();
    RenderTexts();
    RenderGrid();
    DisableAlphaBlend();
    return true;
}

void CNewUIAutoBattler::RenderTable(float x, float y, float width, float height, float titleHeight)
{
    // Native groupbox pattern from the inventory-family windows
    // (NewUIPetInfoWindow::RenderGroupBox, NewUIPetInfoWindow.cpp:280): darkened title strip
    // over a lighter body tint, framed by the shared newui_item_table pieces — no raw solid
    // black rectangle.
    EnableAlphaTest();
    glColor4f(0.f, 0.f, 0.f, 0.9f);
    RenderColor(x + 3.f, y + 2.f, width - 7.f, titleHeight);
    glColor4f(0.f, 0.f, 0.f, 0.6f);
    RenderColor(x + 3.f, y + 2.f + titleHeight, width - 7.f, height - titleHeight - 7.f);
    EndRenderColor();

    RenderImage(IMAGE_AB_TABLE_TOP_LEFT, x, y, 14.0f, 14.0f);
    RenderImage(IMAGE_AB_TABLE_TOP_RIGHT, (x + width) - 14.f, y, 14.0f, 14.0f);
    RenderImage(IMAGE_AB_TABLE_BOTTOM_LEFT, x, (y + height) - 14.f, 14.0f, 14.0f);
    RenderImage(IMAGE_AB_TABLE_BOTTOM_RIGHT, (x + width) - 14.f, (y + height) - 14.f, 14.0f, 14.0f);
    RenderImage(IMAGE_AB_TABLE_TOP_PIXEL, x + 6.f, y, (width - 12.f), 14.0f);
    RenderImage(IMAGE_AB_TABLE_RIGHT_PIXEL, (x + width) - 14.f, y + 6.f, 14.0f, (height - 14.f));
    RenderImage(IMAGE_AB_TABLE_BOTTOM_PIXEL, x + 6.f, (y + height) - 14.f, (width - 12.f), 14.0f);
    RenderImage(IMAGE_AB_TABLE_LEFT_PIXEL, x, (y + 6.f), 14.0f, (height - 14.f));
}

void CNewUIAutoBattler::RenderFrame()
{
    const float x = static_cast<float>(m_Pos.x);
    const float y = static_cast<float>(m_Pos.y);
    const float w = static_cast<float>(kBackW);
    const float h = static_cast<float>(kBackH);

    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);
    TileWindowBack(x, y, w, h);

    // Native frame composition (MsgBoxIGSCommon::RenderFrame), with the header built from the
    // 190x64 newui_item_back01.tga as cap/middle/cap so it spans the full window width:
    // RenderImage would sample texels 1:1 (width/height ARE the sampled extent), so stretching
    // the whole ornamented texture would both deform the end caps and drag the gold close-X
    // medallion baked into its right end (texels ~156..184) into the title bar.
    // The RIGHT cap samples the texture's real right-end texels (162..190), not a repeat of
    // the left cap — the previous (0,0,28,64) source mirrored the left flourish at the wrong
    // angle. The right end also carries the baked close-X glyph at texels 169..182, which
    // therefore lands at (x + kBackW - 21, y + 7): the original header close button position.
    constexpr float kHeaderCapW = 28.f;
    constexpr float kHeaderMidSrcX = 60.f;
    constexpr float kHeaderMidSrcW = 70.f;
    RenderImageStretch(IMAGE_AB_TOP, x, y, kHeaderCapW, static_cast<float>(kHeaderH),
        0.f, 0.f, kHeaderCapW, static_cast<float>(kHeaderH));
    RenderImageStretch(IMAGE_AB_TOP, x + kHeaderCapW, y, w - kHeaderCapW * 2.f, static_cast<float>(kHeaderH),
        kHeaderMidSrcX, 0.f, kHeaderMidSrcW, static_cast<float>(kHeaderH));
    RenderImageStretch(IMAGE_AB_TOP, x + w - kHeaderCapW, y, kHeaderCapW, static_cast<float>(kHeaderH),
        190.f - kHeaderCapW, 0.f, kHeaderCapW, static_cast<float>(kHeaderH));
    RenderImageStretch(IMAGE_AB_LEFT, x, y + static_cast<float>(kHeaderH), 21.f,
        h - static_cast<float>(kHeaderH) - 45.f, 0.f, 0.f, 21.f, 320.f);
    RenderImageStretch(IMAGE_AB_RIGHT, x + w - 21.f, y + static_cast<float>(kHeaderH), 21.f,
        h - static_cast<float>(kHeaderH) - 45.f, 0.f, 0.f, 21.f, 320.f);
    RenderImageStretch(IMAGE_AB_BOTTOM, x, y + h - 45.f, w, 45.f, 0.f, 0.f, 190.f, 45.f);

    if (m_iHunt >= 0 && m_iHunt < HuntCount())
    {
        const HuntDef& hunt = kHunts[m_iHunt];
        int shown = 0;
        for (int i = m_iDropScroll; i < kLootSlots && shown < kGridCount; ++i)
        {
            if (hunt.loot[i] == 0)
                continue;
            const float bx = static_cast<float>(m_Pos.x + kItem3DX + ((shown % kGridCols) * kSlotDX));
            const float by = static_cast<float>(m_Pos.y + kItem3DY + ((shown / kGridCols) * kSlotDY));
            RenderImageStretch(IMAGE_AB_ITEMBOX, bx, by, static_cast<float>(kItem3DW), static_cast<float>(kItem3DH),
                0.f, 0.f, 20.f, 20.f);
            ++shown;
        }
    }

    RenderTable(static_cast<float>(m_Pos.x + kInfoX), static_cast<float>(m_Pos.y + kStatusY),
        static_cast<float>(kInfoW), static_cast<float>(kStatusH), 14.f);
    RenderTable(static_cast<float>(m_Pos.x + kInfoX), static_cast<float>(m_Pos.y + kLootY),
        static_cast<float>(kInfoW), static_cast<float>(kLootH), static_cast<float>(kLootTitleH));

    if (m_iHunt >= 0 && m_iHunt < HuntCount())
    {
        glColor4f(1.f, 1.f, 1.f, 1.f);
        const float mobIconX = static_cast<float>(m_Pos.x + kInfoX + 6);
        float mobRowY = static_cast<float>(m_Pos.y + kLootY + kLootTitleH + 2);
        const HuntDef& hunt = kHunts[m_iHunt];
        int shown = 0;
        int skipped = 0;
        for (int i = 0; i < 5 && shown < kLootSlots; ++i)
        {
            if (hunt.mobs[i].name == nullptr || hunt.mobs[i].model < 0)
                continue;
            if (skipped < m_iMobScroll)
            {
                ++skipped;
                continue;
            }
            RenderImage(IMAGE_AB_ITEMBOX, mobIconX, mobRowY + 1.f,
                static_cast<float>(kLootIcon), static_cast<float>(kLootIcon));
            mobRowY += static_cast<float>(kLootRowH);
            ++shown;
        }
    }
}

void CNewUIAutoBattler::RenderButtons()
{
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->RenderText(
        m_Pos.x,
        m_Pos.y + 10,
        I18N::Game::AutoBattlerTitle,
        kBackW,
        0,
        RT3_SORT_CENTER);

    m_MapButton.Render();
    m_StartButton.Render();
    if (MapPageCount() > 1)
    {
        m_PrevButton.Render();
        m_NextButton.Render();
    }
}

void CNewUIAutoBattler::RenderTexts()
{
    wchar_t szText[128] = {};
    wchar_t remaining[16] = {};
    wchar_t quota[16] = {};
    FormatHms(remaining, m_iRemaining);
    FormatHms(quota, m_iQuota);

    const int textX = m_Pos.x + kInfoX + 8;
    const int textW = kInfoW - 16;
    const int statusY = m_Pos.y + kStatusY + 6;

    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(m_bSessionActive ? 80 : 180, m_bSessionActive ? 220 : 180, m_bSessionActive ? 120 : 180, 255);
    g_pRenderText->RenderText(
        textX,
        statusY,
        m_bSessionActive ? I18N::Game::AutoBattlerStatusOn : I18N::Game::AutoBattlerStatusOff,
        textW,
        0,
        RT3_SORT_LEFT);

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 238, 161, 255);
    mu_swprintf(szText, I18N::Game::AutoBattlerQuota, remaining, quota, L"");
    g_pRenderText->RenderText(textX, statusY + 16, szText, textW, 0, RT3_SORT_LEFT);

    g_pRenderText->SetTextColor(200, 200, 200, 255);
    mu_swprintf(szText, L"%ls: %ls", I18N::Game::AutoBattlerVipLabel,
        m_bVip ? I18N::Game::AutoBattlerVipYes : I18N::Game::AutoBattlerVipNo);
    g_pRenderText->RenderText(textX, statusY + 30, szText, textW, 0, RT3_SORT_LEFT);

    g_pRenderText->SetTextColor(255, 255, 255, 255);
    mu_swprintf(szText, I18N::Game::AutoBattlerCurrentMap, gMapManager.GetMapName(gMapManager.WorldActive));
    g_pRenderText->RenderText(textX, statusY + 44, szText, textW, 0, RT3_SORT_LEFT);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(255, 238, 161, 255);
    g_pRenderText->RenderText(m_Pos.x + kItem3DX, m_Pos.y + kItem3DY - 16, I18N::Game::AutoBattlerLootTab, kItem3DW * 2, 0, RT3_SORT_LEFT);

    const int lootX = m_Pos.x + kInfoX + 8;
    const int lootW = kInfoW - 16;
    g_pRenderText->RenderText(lootX, m_Pos.y + kLootY + 5, I18N::Game::AutoBattlerMobsTab, lootW, 0, RT3_SORT_LEFT);

    if (m_iHunt >= 0 && m_iHunt < HuntCount())
    {
        const HuntDef& hunt = kHunts[m_iHunt];
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(255, 230, 210, 255);
        const int nameX = lootX - 2 + kLootIcon + 6;
        const int nameW = lootW - kLootIcon - 8;
        int mobRowY = m_Pos.y + kLootY + kLootTitleH + 2;
        int shown = 0;
        int skipped = 0;
        for (int i = 0; i < 5 && shown < kLootSlots; ++i)
        {
            if (hunt.mobs[i].name == nullptr)
                continue;
            if (skipped < m_iMobScroll)
            {
                ++skipped;
                continue;
            }
            g_pRenderText->RenderText(nameX, mobRowY + 8, hunt.mobs[i].name, nameW, 0, RT3_SORT_LEFT);
            mobRowY += kLootRowH;
            ++shown;
        }
    }

    if (MapPageCount() > 1)
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(255, 255, 255, 255);
        mu_swprintf(szText, L"%d/%d", m_iMapPage + 1, MapPageCount());
        g_pRenderText->RenderText(m_Pos.x + kCatX + 22, m_Pos.y + kBackH - 30, szText, 32, 0, RT3_SORT_CENTER);
    }
}

void CNewUIAutoBattler::RenderGrid()
{
    if (m_iHunt < 0 || m_iHunt >= HuntCount())
        return;

    EndBitmap();
    memcpy(s_PreProj, GlobalUBO::Instance().GetProj(), sizeof(s_PreProj));
    memcpy(s_PreView, GlobalUBO::Instance().GetView(), sizeof(s_PreView));
    SaveCameraPerspective();
    BeginPreviewCamera();
    EnableDepthTest();
    EnableDepthMask();
    ClearDepthBuffer();

    const HuntDef& hunt = kHunts[m_iHunt];

    int shown = 0;
    int skipped = 0;
    for (int i = 0; i < 5 && shown < kLootSlots; ++i)
    {
        if (hunt.mobs[i].name == nullptr || hunt.mobs[i].model < 0)
            continue;
        if (skipped < m_iMobScroll)
        {
            ++skipped;
            continue;
        }
        const float iconX = static_cast<float>(m_Pos.x + kInfoX + 6);
        const float iconY = static_cast<float>(m_Pos.y + kLootY + kLootTitleH + 2) + (shown * kLootRowH) + 1.f;
        ClearDepthBuffer();
        RenderMonsterPreview(iconX, iconY, static_cast<float>(kLootIcon), static_cast<float>(kLootIcon), hunt.mobs[i].model);
        ++shown;
    }

    RenderLootGrid(hunt, m_Pos.x + kItem3DX, m_Pos.y + kItem3DY, m_iDropScroll);

    glViewport2(0, 0, WindowWidth, WindowHeight);
    UpdateMousePositionn();
    RestoreCameraPerspective();
    GlobalUBO::Instance().SetProj(s_PreProj);
    GlobalUBO::Instance().SetView(s_PreView);
    BeginBitmap();

    EnableAlphaTest();
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    shown = 0;
    for (int i = m_iDropScroll; i < kLootSlots && shown < kGridCount; ++i)
    {
        if (hunt.loot[i] == 0)
            continue;
        wchar_t lootName[MAX_TEXT_LENGTH] = {};
        GetItemName(hunt.loot[i], 0, lootName);
        if (lootName[0] != 0)
        {
            g_pRenderText->RenderText(
                m_Pos.x + kNameX + ((shown % kGridCols) * kSlotDX),
                m_Pos.y + kNameY + ((shown / kGridCols) * kSlotDY),
                lootName,
                kNameW,
                0,
                RT3_SORT_CENTER);
        }
        ++shown;
    }
}
