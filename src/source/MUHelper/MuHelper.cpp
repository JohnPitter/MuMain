#include "stdafx.h"
#include "GameLogic/Combat/SkillExecution.h"

#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>

#include "Engine/AI/ZzzAI.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/PlayerActionState.h"
#include "UI/NewUI/NewUISystem.h"
#include "Core/Utilities/Log/muConsoleDebug.h"
#include "Core/Utilities/Log/ErrorReport.h"
#include "Character/CharacterManager.h"
#include "GameLogic/Skills/SkillManager.h"
#include "GameLogic/Social/PartyManager.h"
#include "World/MapInfra/MapManager.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Network/Server/WSclient.h"

#include "MuHelper.h"

constexpr int MAX_ACTIONABLE_DISTANCE = 10;
constexpr int DEFAULT_DURABILITY_THRESHOLD = 50;

SpinLock _targetsLock;
SpinLock _itemsLock;

// Movement/target globals are defined in ZzzInterface.cpp.
extern MovementSkill g_MovementSkill;
extern int SelectedCharacter;
extern int TargetX;
extern int TargetY;

namespace MUHelper
{
	MovementSkill& g_MovementSkill = ::g_MovementSkill;
	int& SelectedCharacter = ::SelectedCharacter;
	int& TargetX = ::TargetX;
	int& TargetY = ::TargetY;

    CMuHelper g_MuHelper;

    namespace
    {
        constexpr int kRoamArrivalDistance = 12;
        constexpr DWORD kRoamObserveMs = 1500;
        constexpr DWORD kRoamVisitedCooldownMs = 15000;
        constexpr DWORD kRoamUnreachableCooldownMs = 60000;
        constexpr DWORD kRoamIdleLogIntervalMs = 5000;
        // Attack attempts may continue for ~2.5 s without target/hit feedback
        // before the target is classified attack-stalled and a recovery step
        // is issued (one SendMove, consumed entirely by MoveHero).
        constexpr DWORD kAttackStallMs = 2500;
        constexpr int kMaxRecoveryAttempts = 3;
        // Once a swing was issued inside range the attack check keeps this
        // tolerance, so a mob nudge at the range boundary does not flip the
        // hero between walking and stopping every helper tick (250 ms).
        constexpr float kRangeHysteresis = 0.4f;
    }

    void CALLBACK CMuHelper::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
    {
        g_MuHelper.WorkLoop(hwnd, uMsg, idEvent, dwTime);
    }

    void CMuHelper::SaveToServer(const ConfigData& config)
    {
        if (SocketClient == nullptr || SocketClient->ToGameServer() == nullptr)
            return;

        PRECEIVE_MUHELPER_DATA netData;
        ConfigDataSerDe::Serialize(config, netData);
        SocketClient->ToGameServer()->SendMuHelperSaveDataRequest(reinterpret_cast<BYTE*>(&netData), sizeof(netData));
    }

    void CMuHelper::Save(const ConfigData& config)
    {
        m_config = config;
        SaveToServer(m_config);
    }

    void CMuHelper::Load(const ConfigData& config)
    {
        m_config = config;
    }

    ConfigData CMuHelper::GetConfig() const {
        return m_config;
    }

    void CMuHelper::Toggle()
    {
        if (m_bActive)
        {
            TriggerStop();

            // Stop the client-driven bot immediately instead of waiting for the
            // server's status reply. After an auto-reconnect the server's new
            // session doesn't have the helper marked active, so it never replies
            // and the bot would otherwise keep running with no way to stop it.
            Stop();
        }
        else
        {
            TriggerStart();
        }
    }

    void CMuHelper::TriggerStart()
    {
        if (!Hero->SafeZone)
            SocketClient->ToGameServer()->SendMuHelperStatusChangeRequest(0);
    }

    void CMuHelper::TriggerStop()
    {
        SocketClient->ToGameServer()->SendMuHelperStatusChangeRequest(1);
    }

    void CMuHelper::Start()
    {
        if (m_bActive)
        {
            return;
        }

        m_iTotalCost = 0;
        m_iComboState = 0;
        m_iCurrentBuffIndex = 0;
        m_iCurrentBuffPartyIndex = 0;
        m_iCurrentHealPartyIndex = 0;
        m_iCurrentTarget = -1;
        m_iCurrentSkill = (ActionSkillType)m_config.aiSkill[0];
        m_iCurrentItem = MAX_ITEMS;
        m_posOriginal = { Hero->PositionX, Hero->PositionY };
        m_posLastStuck = m_posOriginal;
        m_iStuckTicks = 0;
        m_iObtainFails = 0;
        m_mapBlacklist.clear();
        m_iChaseTarget = -1;
        m_posChaseLast = { 0, 0 };
        m_dwChaseLastProgress = 0;
        m_bChaseRepathed = false;
        m_bPrevMovement = false;
        m_dwAttackLastProgress = 0;
        m_posAttackHeroLast = { 0, 0 };
        m_posAttackTargetLast = { 0, 0 };
        m_iAttackTargetActionLast = -1;
        m_iRecoveryAttempts = 0;
        m_bRecoveryActive = false;
        m_posChasePlanTarget = { 0, 0 };
        m_bAttackEngaged = false;
        m_dwRoamReachedTick = 0;
        m_dwRoamIdleLogTick = 0;

        RecalculateDistances();

        m_iSecondsElapsed = 0;
        m_iSecondsAway = 0;

        m_bTimerActivatedBuffOngoing = false;
        m_bPetActivated = false;

        m_iLoopCounter = 0;

        m_bActive = true;
        AbLog("session active roam=%d", m_bRoamEnabled ? 1 : 0);
        g_ConsoleDebug->Write(MCD_NORMAL, L"[MU Helper] Started");
    }

    void CMuHelper::Stop()
    {
        AbLog("session stopped");
        m_bActive = false;
        m_bIgnoreSafeZoneStop = false;
        m_bIgnoreHuntRange = false;
        m_bRoamEnabled = false;
        m_vecRoamWps.clear();
        m_iRoamWpIndex = -1;
        m_iRoamLastWpIndex = -1;
        m_mapWpCooldown.clear();
        m_dwRoamReachedTick = 0;
        m_dwRoamIdleLogTick = 0;
        m_iStuckTicks = 0;
        m_iObtainFails = 0;
        // Full chase/target/recovery teardown: nothing survives a stop.
        DeleteAllTargets();
        m_iCurrentTarget = -1;
        m_iChaseTarget = -1;
        m_posChaseLast = { 0, 0 };
        m_dwChaseLastProgress = 0;
        m_bChaseRepathed = false;
        m_dwAttackLastProgress = 0;
        m_posAttackHeroLast = { 0, 0 };
        m_posAttackTargetLast = { 0, 0 };
        m_iAttackTargetActionLast = -1;
        m_iRecoveryAttempts = 0;
        m_bRecoveryActive = false;
        m_posChasePlanTarget = { 0, 0 };
        m_bAttackEngaged = false;
        m_iComboState = 0;
        m_iCurrentItem = MAX_ITEMS;
        // Cancel any walk in progress so the hero stops on the spot.
        if (Hero != nullptr)
        {
            Hero->Movement = false;
            Hero->Path.PathNum = 0;
        }
        g_ConsoleDebug->Write(MCD_NORMAL, L"[MU Helper] Stopped");
    }

    void CMuHelper::RecalculateDistances()
    {
        m_iHuntingDistance = ComputeDistanceByRange(m_config.iHuntingRange);
        m_iObtainingDistance = ComputeDistanceByRange(m_config.iObtainingRange);
    }

    void CMuHelper::SetIgnoreSafeZoneStop(bool ignore)
    {
        m_bIgnoreSafeZoneStop = ignore;
    }

    void CMuHelper::SetIgnoreHuntRange(bool ignore)
    {
        m_bIgnoreHuntRange = ignore;
    }

    void CMuHelper::SetAutoStopHandler(std::function<void(const char*)> handler)
    {
        m_AutoStopHandler = std::move(handler);
    }

    // Full auto-deactivation on death / safe-zone entry. Runs once (the
    // m_bActive guard below makes every later tick a no-op): cancels chase,
    // path, recovery, targets and roam via Stop(), asks the Auto Battler to
    // stop the server session and restore the helper config (never sends a
    // second stop request for an already-stopped session), and hides the
    // Hunt Analyzer. Respawn in town does NOT restart the session.
    void CMuHelper::AutoStop(const char* szReason)
    {
        if (!m_bActive)
            return;

        AbLog("auto-stop reason=%s", szReason != nullptr ? szReason : "unknown");
        g_ConsoleDebug->Write(MCD_NORMAL, L"[MU Helper] Auto-stop (%S).", szReason);

        // Local teardown first: chase/path/recovery/target/roam cleared and
        // m_bActive dropped, so later ticks cannot repeat the flow.
        Stop();

        if (m_AutoStopHandler)
        {
            m_AutoStopHandler(szReason);
            return;
        }

        // Fallback (native Mu Helper without the Auto Battler overlay).
        TriggerStop();
    }

    void CMuHelper::SetRoamWaypoints(const POINT* pts, int count)
    {
        m_vecRoamWps.clear();
        m_iRoamWpIndex = -1;
        m_iRoamLastWpIndex = -1;
        m_mapWpCooldown.clear();
        m_dwRoamReachedTick = 0;
        m_dwRoamIdleLogTick = 0;
        if (pts != nullptr && count > 0)
        {
            for (int i = 0; i < count; ++i)
            {
                if (pts[i].x > 0 && pts[i].y > 0)
                    m_vecRoamWps.push_back(pts[i]);
            }
        }
        m_bRoamEnabled = !m_vecRoamWps.empty();
        m_iStuckTicks = 0;
        if (Hero != nullptr)
            m_posLastStuck = { Hero->PositionX, Hero->PositionY };
    }

    bool CMuHelper::FaceAttackTarget()
    {
        if (!m_bActive || m_iCurrentTarget == -1 || Hero == nullptr || CharactersClient == nullptr)
            return false;

        const int iCharIndex = FindCharacterIndex(m_iCurrentTarget);
        if (iCharIndex == MAX_CHARACTERS_CLIENT)
            return false;

        CHARACTER* pTarget = &CharactersClient[iCharIndex];
        if (!pTarget->Object.Live || pTarget->Dead > 0 || !IsMonster(pTarget))
            return false;

        // While the hero walks a path, MovePath() (ZzzAI.cpp) owns the body angle.
        // Writing the attack facing here used to fight the path steering every frame
        // and made the chase stutter. Still return true so MoveHero keeps the
        // mouse-look suppressed for the whole locked-target walk.
        if (Hero->Movement)
            return true;

        VectorCopy(pTarget->Object.Position, Hero->TargetPosition);
        Hero->Object.Angle[2] = CreateAngle2D(Hero->Object.Position, Hero->TargetPosition);
        return true;
    }

    void CMuHelper::WorkLoop(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
    {
        if (!m_bActive)
        {
            return;
        }

        if (Hero == nullptr)
        {
            return;
        }

        // Central auto-stop: death (incl. revival state) or real safe-zone /
        // city entry fully deactivates the Auto Battle — the full stop flow
        // runs once; nothing reactivates it without a new explicit start.
        // Ordinary teleports / map changes are not safe zones and keep hunting.
        const WORD heroWall =
            TerrainWall[TERRAIN_INDEX_REPEAT(Hero->PositionX, Hero->PositionY)];
        const bool bDead = Hero->Dead > 0 || !Hero->Object.Live;
        const bool bSafeZone =
            (Hero->SafeZone && !m_bIgnoreSafeZoneStop)
            || (heroWall & TW_SAFEZONE) == TW_SAFEZONE;

        if (bDead)
        {
            AutoStop("dead");
            return;
        }
        if (bSafeZone)
        {
            AutoStop("safe-zone");
            return;
        }

        Work();

        if (m_iLoopCounter++ == 4)
        {
            m_iSecondsElapsed++;

            if (ComputeDistanceBetween({ Hero->PositionX, Hero->PositionY }, m_posOriginal) > 1)
            {
                m_iSecondsAway++;
            }
            else
            {
                m_iSecondsAway = 0;
            }

            m_iLoopCounter = 0;
        }
    }

    void CMuHelper::Work()
    {
        try
        {
            if (!ActivatePet())
            {
                return;
            }

            // Path lifecycle diagnostics are transition-level only, never per tick.
            if (m_bPrevMovement && !Hero->Movement)
            {
                if (m_iCurrentTarget != -1)
                {
                    AbLog("path completed");
                }
                else if (m_bRoamEnabled && m_iRoamWpIndex >= 0
                    && m_iRoamWpIndex < static_cast<int>(m_vecRoamWps.size()))
                {
                    const POINT destination = m_vecRoamWps[m_iRoamWpIndex];
                    AbLog("roam segment completed wp=%d dest=%d,%d",
                        m_iRoamWpIndex, destination.x, destination.y);
                }
            }
            m_bPrevMovement = Hero->Movement;

            TrackHuntMotion();
            // The roam watchdog owns only targetless travel. It must never
            // clear a committed Auto Battle target that is still being chased.
            if (m_bRoamEnabled && m_iCurrentTarget == -1 && m_iStuckTicks >= 8)
            {
                AbLog("roam stuck, repathing pos=%d,%d", Hero->PositionX, Hero->PositionY);
                if (m_iCurrentItem != MAX_ITEMS)
                    DeleteItem(m_iCurrentItem);
                if (Hero != nullptr)
                {
                    Hero->Movement = false;
                    Hero->Path.PathNum = 0;
                }
                m_iStuckTicks = 0;
                RoamForHunt();
                return;
            }

            if (!Buff())
            {
                return;
            }

            if (!RecoverHealth())
            {
                return;
            }

            if (!ObtainItem())
            {
                return;
            }

            if (!Regroup())
            {
                return;
            }

            CollectNearbyMonsters();
            if (Attack() == 0 && m_iCurrentTarget == -1)
            {
                RoamForHunt();
            }

            RepairEquipments();
        }
        catch (...)
        {
            g_ConsoleDebug->Write(MCD_NORMAL, L"[MU Helper] Exception occurred. Ignoring...");
        }
    }

    void CMuHelper::AddTarget(int iTargetId, bool bIsAttacking)
    {
        if (!m_bActive)
        {
            return;
        }

        CHARACTER* pTarget = FindCharacterByKey(iTargetId);
        if (!pTarget || pTarget == Hero)
        {
            return;
        }

        int iDistance = ComputeDistanceFromTarget(pTarget);

        if ((iDistance <= m_iHuntingDistance)
            || m_bIgnoreHuntRange
            || (bIsAttacking && m_config.bLongRangeCounterAttack))
        {
            _targetsLock.lock();

            m_setTargets.insert(iTargetId);

            if (bIsAttacking)
            {
                m_setTargetsAttacking.insert(iTargetId);
            }

            _targetsLock.unlock();
        }

        if (m_config.bUseSelfDefense && IsMonster(pTarget))
        {
            m_iCurrentTarget = iTargetId;
        }
    }

    void CMuHelper::DeleteTarget(int iTargetId)
    {
        _targetsLock.lock();

        m_setTargets.erase(iTargetId);
        m_setTargetsAttacking.erase(iTargetId);

        _targetsLock.unlock();

        if (iTargetId == m_iCurrentTarget)
        {
            m_iCurrentTarget = -1;
        }
    }

    void CMuHelper::DeleteAllTargets()
    {
        _targetsLock.lock();

        m_setTargets.clear();
        m_setTargetsAttacking.clear();

        _targetsLock.unlock();
    }

    int CMuHelper::ComputeDistanceByRange(int iRange)
    {
        return ComputeDistanceBetween({ 0, 0 }, { iRange, iRange });
    }

    int CMuHelper::ComputeDistanceFromTarget(CHARACTER* pTarget)
    {
        const POINT posHero = { Hero->PositionX, Hero->PositionY };

        const POINT posCurrent = { pTarget->PositionX, pTarget->PositionY };
        const POINT posNext    = { pTarget->TargetX,   pTarget->TargetY };

        return std::min(
            ComputeDistanceBetween(posHero, posCurrent),
            ComputeDistanceBetween(posHero, posNext)
        );
    }

    int CMuHelper::ComputeDistanceBetween(POINT posA, POINT posB)
    {
        int iDx = posA.x - posB.x;
        int iDy = posA.y - posB.y;

        return static_cast<int>(std::ceil(std::sqrt(iDx * iDx + iDy * iDy)));
    }

    int CMuHelper::GetNearestTarget()
    {
        int iClosestMonsterId = -1;
        int iMinDistance = m_bIgnoreHuntRange ? 255 : m_iHuntingDistance;
        std::set<int> setTargets;
        {
            _targetsLock.lock();
            setTargets = m_setTargets;
            _targetsLock.unlock();
        }

        for (const int& iMonsterId : setTargets)
        {
            int iIndex = FindCharacterIndex(iMonsterId);
            if (iIndex == MAX_CHARACTERS_CLIENT)
            {
                continue;
            }

            CHARACTER* pTarget = &CharactersClient[iIndex];

            if (!IsMonster(pTarget) || IsBlacklisted(iMonsterId))
            {
                continue;
            }

            int iDistance = ComputeDistanceFromTarget(pTarget);
            if (iDistance <= iMinDistance)
            {
                iMinDistance = iDistance;
                iClosestMonsterId = iMonsterId;
            }
        }

        return iClosestMonsterId;
    }

    int CMuHelper::GetFarthestAttackingTarget()
    {
        int iFarthestMonsterId = -1;
        int iMaxDistance = -1;

        std::set<int> setTargets;
        {
            _targetsLock.lock();
            setTargets = m_setTargetsAttacking;
            _targetsLock.unlock();
        }

        for (const int& iMonsterId : setTargets)
        {
            int iIndex = FindCharacterIndex(iMonsterId);
            if (iIndex == MAX_CHARACTERS_CLIENT)
            {
                continue;
            }

            CHARACTER* pTarget = &CharactersClient[iIndex];

            if (!IsMonster(pTarget) || IsBlacklisted(iMonsterId))
            {
                continue;
            }

            int iDistance = ComputeDistanceFromTarget(pTarget);
            if (iDistance > iMaxDistance)
            {
                iMaxDistance = iDistance;
                iFarthestMonsterId = iMonsterId;
            }
        }

        return iFarthestMonsterId;
    }

    void CMuHelper::CleanupTargets()
    {
        std::set<int> setTargets;
        {
            _targetsLock.lock();
            setTargets = m_setTargets;
            _targetsLock.unlock();
        }

        for (const int& iMonsterId : setTargets)
        {
            int iIndex = FindCharacterIndex(iMonsterId);
            if (iIndex == MAX_CHARACTERS_CLIENT)
            {
                DeleteTarget(iMonsterId);
                continue;
            }

            CHARACTER* pTarget = &CharactersClient[iIndex];
            if (pTarget->Dead > 0 || !pTarget->Object.Live)
            {
                DeleteTarget(iMonsterId);
            }
        }
    }

    int CMuHelper::ActivatePet()
    {
        if (!m_config.bUseDarkRaven)
        {
            return 1;
        }

        if (m_bPetActivated)
        {
            return 1;
        }

        if (m_config.iDarkRavenMode == PET_ATTACK_CEASE)
        {
            SocketClient->ToGameServer()->SendPetCommandRequest(PetType::DarkRaven, PetCommandMode::Normal, 0xFFFF);
        }
        else if (m_config.iDarkRavenMode == PET_ATTACK_AUTO)
        {
            SocketClient->ToGameServer()->SendPetCommandRequest(PetType::DarkRaven, PetCommandMode::AttackRandom, 0xFFFF);
        }
        else if (m_config.iDarkRavenMode == PET_ATTACK_TOGETHER)
        {
            SocketClient->ToGameServer()->SendPetCommandRequest(PetType::DarkRaven, PetCommandMode::AttackWithOwner, 0xFFFF);
        }

        m_bPetActivated = true;
        return 1;
    }

    int CMuHelper::Buff()
    {
        if (!HasAssignedBuffSkill())
        {
            return 1;
        }

        if (m_config.bSupportParty && g_pPartyManager->IsPartyActive())
        {
            m_iCurrentBuffPartyIndex %= PartyNumber;

            PARTY_t* pMember = &Party[m_iCurrentBuffPartyIndex];
            CHARACTER* pChar = g_pPartyManager->GetPartyMemberChar(pMember);
            int iBuffResult = 1;

            if (pChar != NULL
                && ComputeDistanceFromTarget(pChar) <= MAX_ACTIONABLE_DISTANCE)
            {
                if (!m_config.bBuffDurationParty
                    && m_config.iBuffCastInterval != 0
                    && m_iSecondsElapsed % m_config.iBuffCastInterval == 0)
                {
                    m_bTimerActivatedBuffOngoing = true;
                }

                iBuffResult = BuffTarget(pChar, (ActionSkillType)m_config.aiBuff[m_iCurrentBuffIndex]);
            }

            m_iCurrentBuffPartyIndex = (m_iCurrentBuffPartyIndex + 1) % PartyNumber;

            if (m_iCurrentBuffPartyIndex == 0)
            {
                m_iCurrentBuffIndex = (m_iCurrentBuffIndex + 1) % m_config.aiBuff.size();

                // Reaching this branch means everyone's been buffed, 
                // so we're resetting the timer activated buff flag
                if (m_iCurrentBuffIndex == 0)
                {
                    m_bTimerActivatedBuffOngoing = false;
                }
            }

            return iBuffResult;
        }
        else
        {
            m_iCurrentBuffPartyIndex = 0;

            if (!m_config.bBuffDuration
                && m_config.iBuffCastInterval != 0
                && m_iSecondsElapsed % m_config.iBuffCastInterval == 0)
            {
                m_bTimerActivatedBuffOngoing = true;
            }

            if (!BuffTarget(Hero, (ActionSkillType)m_config.aiBuff[m_iCurrentBuffIndex]))
            {
                return 0;
            }
        }

        if (m_iCurrentBuffPartyIndex == 0)
        {
            m_iCurrentBuffIndex = (m_iCurrentBuffIndex + 1) % m_config.aiBuff.size();

            // Reaching this branch means everyone's been buffed, 
            // so we're resetting the timer activated buff flag
            if (m_iCurrentBuffIndex == 0)
            {
                m_bTimerActivatedBuffOngoing = false;
            }
        }

        return 1;
    }

    int CMuHelper::BuffTarget(CHARACTER* pTargetChar, ActionSkillType iBuffSkill)
    {
        OBJECT* obj = &pTargetChar->Object;

        auto CastIfMissing = [&](bool bBuffActive, bool bTimerRespected, bool bNeedsTarget) -> int
        {
            if (!bBuffActive || (bTimerRespected && m_bTimerActivatedBuffOngoing))
                return SimulateSkill(iBuffSkill, bNeedsTarget, pTargetChar->Key);
            return 1;
        };

        switch (iBuffSkill)
        {
        case AT_SKILL_ATTACK:
        case AT_SKILL_ATTACK_STR:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Attack), true, true);

        case AT_SKILL_DEFENSE:
        case AT_SKILL_DEFENSE_STR:
        case AT_SKILL_DEFENSE_MASTERY:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Defense), true, true);

        case AT_SKILL_INFINITY_ARROW:
        case AT_SKILL_INFINITY_ARROW_STR:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_InfinityArrow), false, false);

        case AT_SKILL_SOUL_BARRIER:
        case AT_SKILL_SOUL_BARRIER_STR:
        case AT_SKILL_SOUL_BARRIER_PROFICIENCY:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_WizDefense), true, true);

        case AT_SKILL_SWELL_LIFE:
        case AT_SKILL_SWELL_LIFE_STR:
        case AT_SKILL_SWELL_LIFE_PROFICIENCY:
            if (m_iComboState == 2)
            {
                return 1;
            }
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Life), true, false);

        case AT_SKILL_EXPANSION_OF_WIZARDRY:
        case AT_SKILL_EXPANSION_OF_WIZARDRY_STR:
        case AT_SKILL_EXPANSION_OF_WIZARDRY_MASTERY:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_SwellOfMagicPower), false, false);

        case AT_SKILL_ADD_CRITICAL:
        case AT_SKILL_ADD_CRITICAL_STR1:
        case AT_SKILL_ADD_CRITICAL_STR2:
        case AT_SKILL_ADD_CRITICAL_STR3:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_AddCriticalDamage), false, false);

        case AT_SKILL_ALICE_BERSERKER:
        case AT_SKILL_ALICE_BERSERKER_STR:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Berserker), false, false);

        case AT_SKILL_ALICE_THORNS:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Thorns), false, false);

        // Rage Fighter party buffs — self/party AoE, no explicit target needed.
        case AT_SKILL_ATT_UP_OURFORCES:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Att_up_Ourforces), true, false);

        case AT_SKILL_HP_UP_OURFORCES:
        case AT_SKILL_HP_UP_OURFORCES_STR:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Hp_up_Ourforces), true, false);

        case AT_SKILL_DEF_UP_OURFORCES:
        case AT_SKILL_DEF_UP_OURFORCES_STR:
        case AT_SKILL_DEF_UP_OURFORCES_MASTERY:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Def_up_Ourforces), true, false);

        default:
            return 1;
        }
    }


    int CMuHelper::ConsumePotion()
    {
        int64_t iLife = CharacterAttribute->Life;
        int64_t iLifeMax = CharacterAttribute->LifeMax;

        if (m_config.bUseHealPotion && iLifeMax > 0 && iLife > 0)
        {
            int64_t iRemaining = (iLife * 100 + iLifeMax - 1) / iLifeMax;
            if (iRemaining <= m_config.iPotionThreshold)
            {
                int iPotionIndex = g_pMyInventory->FindHealingItemIndex();
                if (iPotionIndex != -1)
                {
                    SendRequestUse(iPotionIndex, 0);
                }
            }
        }

        return 1;
    }

    int CMuHelper::RecoverHealth()
    {
        if (!Heal())
        {
            return 0;
        }
        
        if (!DrainLife())
        {
            return 0;
        }

        if (!ConsumePotion())
        {
            return 0;
        }

        return 1;
    }

    int CMuHelper::Heal()
    {
        if (!m_config.bAutoHeal)
        {
            return 1;
        }

        auto iHealingSkill = GetHealingSkill();
        if (iHealingSkill == AT_SKILL_UNDEFINED)
        {
            return 1;
        }

        if (m_config.bAutoHealParty && g_pPartyManager->IsPartyActive())
        {
            m_iCurrentHealPartyIndex %= PartyNumber;

            PARTY_t* pMember = &Party[m_iCurrentHealPartyIndex];
            CHARACTER* pChar = g_pPartyManager->GetPartyMemberChar(pMember);
            int iHealResult = 1;

            if (pChar != NULL)
            {
                if (pChar == Hero)
                {
                    iHealResult = HealSelf(iHealingSkill);
                }
                else if (pMember->stepHP * 10 <= m_config.iHealPartyThreshold
                    && ComputeDistanceFromTarget(pChar) <= MAX_ACTIONABLE_DISTANCE)
                {
                    iHealResult = SimulateSkill(iHealingSkill, true, pChar->Key);
                }
            }

            m_iCurrentHealPartyIndex = (m_iCurrentHealPartyIndex + 1) % PartyNumber;

            return iHealResult;
        }
        else
        {
            m_iCurrentHealPartyIndex = 0;
            return HealSelf(iHealingSkill);
        }

        return 1;
    }

    int CMuHelper::HealSelf(ActionSkillType iHealingSkill)
    {
        int64_t iLife = CharacterAttribute->Life;
        int64_t iLifeMax = CharacterAttribute->LifeMax;
        int64_t iRemaining = (iLife * 100 + iLifeMax - 1) / iLifeMax;

        if (iRemaining <= m_config.iHealThreshold)
        {
            return SimulateSkill(iHealingSkill, true, HeroKey);
        }

        return 1;
    }

    int CMuHelper::DrainLife()
    {
        if (!m_config.bUseDrainLife)
        {
            return 1;
        }

        auto iDrainLife = GetDrainLifeSkill();
        if (iDrainLife == AT_SKILL_UNDEFINED)
        {
            return 1;
        }

        int64_t iLife = CharacterAttribute->Life;
        int64_t iLifeMax = CharacterAttribute->LifeMax;
        int64_t iRemaining = (iLife * 100 + iLifeMax - 1) / iLifeMax;

        if (iRemaining <= m_config.iHealThreshold)
        {
            m_iCurrentTarget = GetNearestTarget();
            if (m_iCurrentTarget != -1)
            {
                return SimulateSkill(iDrainLife, true, m_iCurrentTarget);
            }
        }

        return 1;
    }

    int CMuHelper::RepairEquipments()
    {
        if (m_config.bRepairItem)
        {
            for (int i = 0; i < MAX_EQUIPMENT; i++)
            {
                ITEM* pItem = &CharacterMachine->Equipment[i];
                if (!pItem || pItem->Type == -1)
                {
                    continue;
                }

                ITEM_ATTRIBUTE* pAttr = &ItemAttribute[pItem->Type];
                if (!pAttr)
                {
                    continue;
                }

                int iLevel = pItem->Level;
                int iDurability = pItem->Durability;
                int iMaxDurability = CalcMaxDurability(pItem, pAttr, iLevel);

                int64_t iHealth = (iDurability * 100 + iMaxDurability - 1) / iMaxDurability;

                if (iHealth <= DEFAULT_DURABILITY_THRESHOLD)
                {
                    int64_t iGoldCost = CalcSelfRepairCost(ItemValue(pItem, 2), iDurability, iMaxDurability, pItem->Type);
                    if (iGoldCost <= CharacterMachine->Gold)
                    {
                        SocketClient->ToGameServer()->SendRepairItemRequest(i, 1);
                    }
                }
            }
        }

        return 1;
    }

    int CMuHelper::Attack()
    {
        // Target lock: while a target is locked it is validated first and kept
        // until it dies, is removed, leaves the leash, or the chase times out.
        if (m_iCurrentTarget != -1 && !ValidateChaseTarget(m_iCurrentTarget))
        {
            m_iCurrentTarget = -1;
        }

        if (m_iCurrentTarget == -1)
        {
            if (!m_setTargets.empty())
            {
                CleanupTargets();
                PurgeBlacklist();

                if (m_config.bLongRangeCounterAttack)
                {
                    m_iCurrentTarget = GetFarthestAttackingTarget();
                }

                if (m_iCurrentTarget == -1)
                {
                    m_iCurrentTarget = GetNearestTarget();
                }
            }

            if (m_iCurrentTarget == -1)
            {
                m_iComboState = 0;
                return 0;
            }
        }

        // Attack-stall watchdog: run once per helper tick while a target is
        // locked. A stall (no target/hit/hero progress for ~2.5 s) issues a
        // recovery step or, after the attempt limit, releases the target.
        if (m_iCurrentTarget != -1)
        {
            TrackTargetProgress(m_iCurrentTarget);
            if (m_iCurrentTarget == -1)
            {
                m_iComboState = 0;
                return 0;
            }
        }

        if (m_config.bUseCombo)
        {
            return SimulateComboAttack();
        }

        m_iCurrentSkill = SelectAttackSkill();
        if (m_iCurrentSkill > AT_SKILL_UNDEFINED)
        {
            const float fSkillDistance = gSkillManager.GetSkillDistance(m_iCurrentSkill, Hero);
            if (m_bIgnoreHuntRange
                || GameLogic::Combat::CanExecuteSkill(Hero, m_iCurrentSkill, fSkillDistance))
            {
                const int skillResult = SimulateAttack(m_iCurrentSkill);
                if (skillResult)
                    return skillResult;
            }
        }

        if (m_config.bFallbackBasicAttack)
        {
            return SimulateBasicAttack(m_iCurrentTarget);
        }

        return 1;
    }

    ActionSkillType CMuHelper::SelectAttackSkill()
    {
        const size_t safeSize = std::min({m_config.aiSkill.size(), m_config.aiSkillCondition.size(), m_config.aiSkillInterval.size()});
        for (int i = 1; i < (int)safeSize; i++)
        {
            const int iSkillId = m_config.aiSkill[i];
            if (iSkillId <= 0 || iSkillId >= MAX_SKILLS)
            {
                continue;
            }

            if ((m_config.aiSkillCondition[i] & ON_TIMER)
                && m_config.aiSkillInterval[i] != 0
                && m_iSecondsElapsed > 0
                && m_iSecondsElapsed % m_config.aiSkillInterval[i] == 0)
            {
                return (ActionSkillType)iSkillId;
            }

            if (m_config.aiSkillCondition[i] & ON_CONDITION)
            {
                int iCount = 0;
                if (m_config.aiSkillCondition[i] & ON_MOBS_NEARBY)
                {
                    iCount = (int)m_setTargets.size();
                }
                else if (m_config.aiSkillCondition[i] & ON_MOBS_ATTACKING)
                {
                    iCount = (int)m_setTargetsAttacking.size();
                }
                else
                {
                    continue;
                }

                if (((m_config.aiSkillCondition[i] & ON_MORE_THAN_TWO_MOBS)   && iCount >= 2)
                    || ((m_config.aiSkillCondition[i] & ON_MORE_THAN_THREE_MOBS) && iCount >= 3)
                    || ((m_config.aiSkillCondition[i] & ON_MORE_THAN_FOUR_MOBS)  && iCount >= 4)
                    || ((m_config.aiSkillCondition[i] & ON_MORE_THAN_FIVE_MOBS)  && iCount >= 5))
                {
                    return (ActionSkillType)iSkillId;
                }
            }
        }

        if (m_config.aiSkill[0] > 0)
        {
            return (ActionSkillType)m_config.aiSkill[0];
        }

        return AT_SKILL_UNDEFINED;
    }

    int CMuHelper::SimulateComboAttack()
    {
        for (int i = 0; i < m_config.aiSkill.size(); i++)
        {
            if (m_config.aiSkill[i] == 0)
            {
                return 0;
            }
        }

        if (SimulateAttack((ActionSkillType)m_config.aiSkill[m_iComboState]))
        {
            m_iComboState = (m_iComboState + 1) % 3;
        }

        return 1;
    }

    // True while the hero is mid swing; gating helper actions on it makes the
    // bot's cadence follow AttackSpeed instead of the fixed helper timer, the
    // same way the manual click path gates in MoveHero (ZzzInterface.cpp).
    static bool IsHeroSwingInProgress()
    {
        const int iAction = Hero->Object.CurrentAction;

        // Outside the swing enum range entirely -> not a swing.
        if (!Engine::Object::IsAttackAction(iAction))
            return false;

        // Several non-swing *stance* animations (mounted idle/walk/run, two-hand-
        // sword stance, ride-horse, rage-fenrir) share the [PLAYER_ATTACK_FIST ..
        // PLAYER_RIDE_SKILL] enum range that IsAttackAction() spans. MoveHero
        // (ZzzInterface.cpp) OR-excludes exactly these four ranges when deciding
        // whether the hero may move; mirror that here. Otherwise a Fenrir-mounted
        // idle character (CurrentAction == PLAYER_FENRIR_STAND, inside the range)
        // reads as a perpetual swing, IsHeroSwingInProgress() never clears, and
        // SimulateSkill()/SimulateAttack() never fire -- the auto-helper is dead
        // for the whole session while Horn of Fenrir (or any mount) is equipped.
        if ((iAction >= PLAYER_STOP_TWO_HAND_SWORD_TWO && iAction <= PLAYER_RUN_TWO_HAND_SWORD_TWO)
            || (iAction >= PLAYER_DARKLORD_STAND && iAction <= PLAYER_RUN_RIDE_HORSE)
            || (iAction >= PLAYER_FENRIR_RUN && iAction <= PLAYER_FENRIR_WALK_ONE_LEFT)
            || (iAction >= PLAYER_RAGE_FENRIR_WALK && iAction <= PLAYER_RAGE_FENRIR_STAND_ONE_LEFT))
            return false;

        // Genuine attack/skill swing -> Fenrir attack/skill actions sit below
        // PLAYER_FENRIR_RUN, so they stay gated and cadence still tracks
        // AttackSpeed when mounted.
        return true;
    }

    int CMuHelper::SimulateAttack(ActionSkillType iSkill)
    {
        return SimulateSkill(iSkill, true, m_iCurrentTarget);
    }

    int CMuHelper::SimulateSkill(ActionSkillType iSkill, bool bTargetRequired, int iTarget)
    {
        // Let the current swing finish before issuing another action, so the
        // cadence tracks AttackSpeed instead of the fixed helper timer.
        if (IsHeroSwingInProgress())
        {
            return 0;
        }

        g_MovementSkill.m_iSkill = iSkill;
        g_MovementSkill.m_bMagic = true;

        const float fSkillDistance = gSkillManager.GetSkillDistance(iSkill, Hero);
        const bool bSelfPositionSkill = IsSelfPositionSkill(iSkill);

        if (bTargetRequired)
        {
            if (bSelfPositionSkill)
            {
                TargetX = Hero->PositionX;
                TargetY = Hero->PositionY;

                g_MovementSkill.m_iTarget = -1;

                // Check if current target is still valid (exists and alive)
                if (iTarget != -1)
                {
                    const int iCharIndex = FindCharacterIndex(iTarget);
                    if (iCharIndex != MAX_CHARACTERS_CLIENT)
                    {
                        CHARACTER* pCurrentTarget = &CharactersClient[iCharIndex];
                        if (pCurrentTarget->Dead > 0 || !IsMonster(pCurrentTarget))
                        {
                            DeleteTarget(iTarget);
                            return 0;
                        }
                    }
                    else
                    {
                        DeleteTarget(iTarget);
                        return 0;
                    }
                }
            }
            else
            {
                if (iTarget == -1)
                {
                    return 0;
                }

                const int iCharIndex = FindCharacterIndex(iTarget);
                if (iCharIndex == MAX_CHARACTERS_CLIENT)
                {
                    DeleteTarget(iTarget);
                    return 0;
                }

                SelectedCharacter = iCharIndex;

                CHARACTER* pTarget = &CharactersClient[iCharIndex];
                if (pTarget->Dead > 0)
                {
                    DeleteTarget(iTarget);
                    return 0;
                }

                g_MovementSkill.m_iTarget = iCharIndex;

                // Match the manual attack path: lock the hero's facing to the
                // selected target before sending a skill or attack request.
                VectorCopy(pTarget->Object.Position, Hero->TargetPosition);
                Hero->Object.Angle[2] = CreateAngle2D(Hero->Object.Position, Hero->TargetPosition);

                TargetX = (int)(pTarget->Object.Position[0] / TERRAIN_SCALE);
                TargetY = (int)(pTarget->Object.Position[1] / TERRAIN_SCALE);

                const bool bTargetNear = CheckTile(Hero, &Hero->Object,
                    fSkillDistance + (m_bAttackEngaged ? kRangeHysteresis : 0.0f));
                if (bTargetNear)
                {
                    // In skill range: attack. A wall between hero and target
                    // means the skill cannot land — instead of stalling (or
                    // blacklisting outright), step aside and re-approach.
                    if (!CheckWall(Hero->PositionX, Hero->PositionY, TargetX, TargetY))
                    {
                        HandleAttackStall(iTarget, "wall-between");
                        return 0;
                    }
                    m_bAttackEngaged = true;
                }
                else
                {
                    m_bAttackEngaged = false;

                    // Chase watchdog: tracks progress and gives up after a
                    // stall (one replan, then abandon + blacklist).
                    UpdateChase(iTarget);
                    if (m_iCurrentTarget != iTarget)
                        return 0;

                    if (PlanChasePath(iTarget, pTarget, fSkillDistance, "chase-skill") == -1
                        && !m_bRoamEnabled)
                    {
                        BlacklistTarget(iTarget, "no-path");
                        ReleaseChaseTarget(iTarget, "no-path");
                    }
                    return 0;
                }
            }
        }
        else
        {
            TargetX = Hero->PositionX;
            TargetY = Hero->PositionY;
        }

        int iSkillResult = GameLogic::Combat::ExecuteSkill(Hero, iSkill, fSkillDistance);
        if (iSkillResult == -1 && iTarget != -1)
        {
            DeleteTarget(iTarget);
        }

        return (int)(iSkillResult == 1);
    }

    int CMuHelper::SimulateBasicAttack(int iTarget)
    {
        if (iTarget == -1)
        {
            return 0;
        }

        // Let the current swing finish before attacking again, so the cadence
        // tracks AttackSpeed instead of the fixed helper timer.
        if (IsHeroSwingInProgress())
        {
            return 0;
        }

        const int iCharIndex = FindCharacterIndex(iTarget);
        if (iCharIndex == MAX_CHARACTERS_CLIENT)
        {
            DeleteTarget(iTarget);
            return 0;
        }

        CHARACTER* pTarget = &CharactersClient[iCharIndex];
        if (pTarget->Dead > 0 || !IsMonster(pTarget))
        {
            DeleteTarget(iTarget);
            return 0;
        }

        constexpr float BASIC_RANGE_DEFAULT = 1.8f;
        constexpr float BASIC_RANGE_SPEAR = 2.2f;
        constexpr float BASIC_RANGE_BOW = 6.0f;

        float fRange = BASIC_RANGE_DEFAULT;
        const int iWeaponRight = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;
        if (iWeaponRight >= ITEM_SPEAR && iWeaponRight < ITEM_SPEAR + MAX_ITEM_INDEX)
        {
            fRange = BASIC_RANGE_SPEAR;
        }
        if (gCharacterManager.GetEquipedBowType() != BOWTYPE_NONE)
        {
            fRange = BASIC_RANGE_BOW;
        }

        SelectedCharacter = iCharIndex;
        VectorCopy(pTarget->Object.Position, Hero->TargetPosition);
        Hero->Object.Angle[2] = CreateAngle2D(Hero->Object.Position, Hero->TargetPosition);
        TargetX = (int)(pTarget->Object.Position[0] / TERRAIN_SCALE);
        TargetY = (int)(pTarget->Object.Position[1] / TERRAIN_SCALE);

        const bool bTargetNear = CheckTile(Hero, &Hero->Object,
            fRange + (m_bAttackEngaged ? kRangeHysteresis : 0.0f));
        if (bTargetNear)
        {
            // In melee/ranged-basic range: attack. A wall between hero and
            // target means the attack cannot land — step aside and re-approach
            // instead of retrying the blocked swing forever.
            if (!CheckWall(Hero->PositionX, Hero->PositionY, TargetX, TargetY))
            {
                HandleAttackStall(iTarget, "wall-between");
                return 0;
            }
            m_bAttackEngaged = true;
        }
        else
        {
            m_bAttackEngaged = false;

            // Chase watchdog: tracks progress and gives up after a stall
            // (one replan, then abandon + blacklist).
            UpdateChase(iTarget);
            if (m_iCurrentTarget != iTarget)
                return 0;

            if (PlanChasePath(iTarget, pTarget, fRange, "chase-basic") == -1
                && !m_bRoamEnabled)
            {
                BlacklistTarget(iTarget, "no-path");
                ReleaseChaseTarget(iTarget, "no-path");
            }
            return 0;
        }

        if (gCharacterManager.GetEquipedBowType() != BOWTYPE_NONE && !CheckArrow())
        {
            return 0;
        }

        Hero->MovementType = MOVEMENT_ATTACK;
        ActionTarget = iCharIndex;
        Attacking = 1;
        Action(Hero, &Hero->Object, true);
        return 1;
    }

    void CMuHelper::CollectNearbyMonsters()
    {
        if (!m_bIgnoreHuntRange || CharactersClient == nullptr || Hero == nullptr)
            return;

        for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
        {
            CHARACTER* c = &CharactersClient[i];
            if (!c->Object.Live || c->Dead > 0 || c == Hero)
                continue;
            if (!IsMonster(c))
                continue;
            AddTarget(c->Key, false);
        }
    }

    int CMuHelper::Regroup()
    {
        if (m_config.bReturnToOriginalPosition && m_iSecondsAway > m_config.iMaxSecondsAway)
        {
            if (!SimulateMove(m_posOriginal))
            {
                return 0;
            }

            m_iSecondsAway = 0;
            m_iComboState = 0;
            m_iCurrentTarget = -1;
        }

        return 1;
    }

    bool CMuHelper::IsWalkingPath() const
    {
        if (Hero == nullptr)
            return false;
        return Hero->Movement && Hero->Path.PathNum > 1 && Hero->Path.CurrentPath < Hero->Path.PathNum - 1;
    }

    void CMuHelper::TrackHuntMotion()
    {
        if (!m_bRoamEnabled || Hero == nullptr)
            return;

        if (Hero->PositionX != m_posLastStuck.x || Hero->PositionY != m_posLastStuck.y)
        {
            m_posLastStuck = { Hero->PositionX, Hero->PositionY };
            m_iStuckTicks = 0;
            return;
        }

        if (Hero->Movement && !IsHeroSwingInProgress())
            ++m_iStuckTicks;
    }

    // Sparse diagnostics for the Auto Battle state machine, written to
    // MuError.log with an "AUTOBATTLE:" prefix. Transition-level only — never
    // called per frame.
    void CMuHelper::AbLog(const char* szFormat, ...)
    {
        char szAnsi[256];
        va_list args;
        va_start(args, szFormat);
        _vsnprintf_s(szAnsi, sizeof(szAnsi), _TRUNCATE, szFormat != nullptr ? szFormat : "", args);
        va_end(args);

        wchar_t szText[256];
        const int nLen = MultiByteToWideChar(CP_ACP, 0, szAnsi, -1, szText, std::size(szText) - 1);
        if (nLen <= 0)
            return;
        szText[nLen] = L'\0';
        g_ErrorReport.Write(L"AUTOBATTLE: %s\r\n", szText);
    }

    void CMuHelper::PurgeBlacklist()
    {
        const DWORD now = GetTickCount();
        for (auto it = m_mapBlacklist.begin(); it != m_mapBlacklist.end();)
        {
            if (static_cast<int>(it->second - now) <= 0)
                it = m_mapBlacklist.erase(it);
            else
                ++it;
        }
    }

    bool CMuHelper::IsBlacklisted(int iTargetId)
    {
        PurgeBlacklist();
        return m_mapBlacklist.find(iTargetId) != m_mapBlacklist.end();
    }

    void CMuHelper::BlacklistTarget(int iTargetId, const char* szReason)
    {
        // Short cooldown so a temporarily unreachable target is not retried
        // every tick, but becomes eligible again after the cooldown elapses.
        constexpr DWORD kBlacklistCooldownMs = 5000;
        m_mapBlacklist[iTargetId] = GetTickCount() + kBlacklistCooldownMs;
        AbLog("target blacklisted id=%d reason=%s", iTargetId, szReason != nullptr ? szReason : "unknown");
    }

    // Releases the target lock; only death/removal, invalidity, leash exit,
    // confirmed unreachable path or stuck-timeout may call this.
    void CMuHelper::ReleaseChaseTarget(int iTargetId, const char* szReason)
    {
        AbLog("target released id=%d reason=%s", iTargetId, szReason != nullptr ? szReason : "unknown");
        DeleteTarget(iTargetId);
        if (m_iChaseTarget != -1)
        {
            m_iChaseTarget = -1;
            m_posChaseLast = { 0, 0 };
            m_dwChaseLastProgress = 0;
            m_bChaseRepathed = false;
        }
        m_dwAttackLastProgress = 0;
        m_posAttackHeroLast = { 0, 0 };
        m_posAttackTargetLast = { 0, 0 };
        m_iAttackTargetActionLast = -1;
        m_iRecoveryAttempts = 0;
        m_bRecoveryActive = false;
        m_posChasePlanTarget = { 0, 0 };
        m_bAttackEngaged = false;
    }

    // Validates the locked target before any action: live monster, same view,
    // not blacklisted and inside the leash. Returns false when the lock must
    // be dropped.
    bool CMuHelper::ValidateChaseTarget(int iTargetId)
    {
        if (iTargetId == -1)
            return false;

        const int iCharIndex = FindCharacterIndex(iTargetId);
        if (iCharIndex == MAX_CHARACTERS_CLIENT)
        {
            ReleaseChaseTarget(iTargetId, "removed");
            return false;
        }

        CHARACTER* pTarget = &CharactersClient[iCharIndex];
        if (!pTarget->Object.Live || pTarget->Dead > 0 || !IsMonster(pTarget))
        {
            ReleaseChaseTarget(iTargetId, "dead-or-invalid");
            return false;
        }

        // Normal Mu Helper retains its blacklist/leash behavior. Auto Battle
        // commits to the chosen live target and may only select another after
        // this one dies or disappears from the client object list.
        if (!m_bRoamEnabled && IsBlacklisted(iTargetId))
        {
            ReleaseChaseTarget(iTargetId, "blacklisted");
            return false;
        }

        if (!m_bRoamEnabled)
        {
            const int iLeash = m_iHuntingDistance + 10;
            if (ComputeDistanceFromTarget(pTarget) > iLeash)
            {
                BlacklistTarget(iTargetId, "out-of-leash");
                ReleaseChaseTarget(iTargetId, "out-of-leash");
                return false;
            }
        }

        return true;
    }

    // Chase progress watchdog. While the hero is walking an existing path,
    // the path is left untouched (MoveHero keeps sending the normal walk
    // packets, exactly like a long manual click). Only when the hero stopped
    // moving is a fresh path computed. No movement progress for a while
    // triggers one replan; a second stall abandons and blacklists the target.
    void CMuHelper::UpdateChase(int iTargetId)
    {
        constexpr DWORD kStuckTimeoutMs = 3000;

        if (Hero == nullptr)
            return;

        if (iTargetId != m_iChaseTarget)
        {
            m_iChaseTarget = iTargetId;
            m_posChaseLast = { Hero->PositionX, Hero->PositionY };
            m_dwChaseLastProgress = GetTickCount();
            m_bChaseRepathed = false;
            m_dwAttackLastProgress = m_dwChaseLastProgress;
            m_posAttackHeroLast = { Hero->PositionX, Hero->PositionY };
            m_iAttackTargetActionLast = -1;
            m_iRecoveryAttempts = 0;
            m_bRecoveryActive = false;
            m_bAttackEngaged = false;
            AbLog("target acquired id=%d pos=%d,%d", iTargetId, Hero->PositionX, Hero->PositionY);
        }

        const DWORD now = GetTickCount();
        const POINT pos = { Hero->PositionX, Hero->PositionY };
        if (pos.x != m_posChaseLast.x || pos.y != m_posChaseLast.y)
        {
            m_posChaseLast = pos;
            m_dwChaseLastProgress = now;
            m_bChaseRepathed = false;
            return;
        }

        if (now - m_dwChaseLastProgress < kStuckTimeoutMs)
            return;

        if (!m_bChaseRepathed)
        {
            // First stall: cancel the current path so the movement branch
            // replans from the committed cell. This clears the walk state,
            // it never writes the hero position directly.
            m_bChaseRepathed = true;
            m_dwChaseLastProgress = now;
            Hero->Movement = false;
            Hero->Path.PathNum = 0;
            AbLog("stuck, repathing target id=%d pos=%d,%d", iTargetId, pos.x, pos.y);
            return;
        }

        if (m_bRoamEnabled)
        {
            // Auto Battle keeps the live target. Retry from the committed cell
            // instead of selecting a nearer replacement after a temporary stall.
            m_bChaseRepathed = false;
            m_dwChaseLastProgress = now;
            Hero->Movement = false;
            Hero->Path.PathNum = 0;
            AbLog("stuck, retrying locked target id=%d pos=%d,%d", iTargetId, pos.x, pos.y);
            return;
        }

        AbLog("stuck, giving up target id=%d pos=%d,%d", iTargetId, pos.x, pos.y);
        BlacklistTarget(iTargetId, "stuck");
        ReleaseChaseTarget(iTargetId, "stuck-timeout");
    }

    // Per-tick progress tracker for the locked target. Progress is any of:
    // hero moved, target moved, target hit/act animation changed, or target
    // died. ~2.5 s of attack attempts without progress classifies as
    // attack-stalled and triggers a recovery step (never an instant release
    // of a live target).
    void CMuHelper::TrackTargetProgress(int iTargetId)
    {
        if (iTargetId == -1 || Hero == nullptr || CharactersClient == nullptr)
            return;

        const int iCharIndex = FindCharacterIndex(iTargetId);
        if (iCharIndex == MAX_CHARACTERS_CLIENT)
            return;

        CHARACTER* pTarget = &CharactersClient[iCharIndex];
        const DWORD now = GetTickCount();
        const POINT posHero = { Hero->PositionX, Hero->PositionY };
        const POINT posTarget = { pTarget->PositionX, pTarget->PositionY };
        const int iAction = pTarget->Object.CurrentAction;

        const bool heroMoved =
            posHero.x != m_posAttackHeroLast.x || posHero.y != m_posAttackHeroLast.y;
        const bool targetMoved =
            posTarget.x != m_posAttackTargetLast.x || posTarget.y != m_posAttackTargetLast.y;
        const bool actChanged = iAction != m_iAttackTargetActionLast;

        if (heroMoved || targetMoved || actChanged || pTarget->Dead > 0)
        {
            if (m_bRecoveryActive && heroMoved)
            {
                // The recovery SendMove was consumed by MoveHero: back on a
                // valid cell, the normal chase replans from here.
                m_bRecoveryActive = false;
                AbLog("recovery complete target=%d pos=%d,%d", iTargetId, posHero.x, posHero.y);
            }
            m_dwAttackLastProgress = now;
        }

        m_posAttackHeroLast = posHero;
        m_posAttackTargetLast = posTarget;
        m_iAttackTargetActionLast = iAction;

        if (now - m_dwAttackLastProgress < kAttackStallMs)
            return;

        HandleAttackStall(iTargetId, "no-progress");
    }

    // Attack-stalled recovery: one SendMove to a validated cell 2-4 tiles off
    // the target (never a Position write). After kMaxRecoveryAttempts the
    // target is blacklisted and released — even in roam sessions — so the
    // bot can never loop forever on an unhittable target.
    void CMuHelper::HandleAttackStall(int iTargetId, const char* szWhy)
    {
        AbLog("attack stalled target=%d reason=%s", iTargetId, szWhy ? szWhy : "unknown");

        POINT dest;
        if (m_iRecoveryAttempts >= kMaxRecoveryAttempts
            || !FindRecoveryCell(iTargetId, dest))
        {
            BlacklistTarget(iTargetId, "attack-stalled");
            ReleaseChaseTarget(iTargetId, "recovery failed/release");
            return;
        }

        ++m_iRecoveryAttempts;
        Hero->Movement = false;
        Hero->Path.PathNum = 0;
        Hero->MovementType = MOVEMENT_MOVE;
        TargetX = dest.x;
        TargetY = dest.y;

        if (!PathFinding2(Hero->PositionX, Hero->PositionY, dest.x, dest.y, &Hero->Path, 0.0f))
        {
            BlacklistTarget(iTargetId, "attack-stalled");
            ReleaseChaseTarget(iTargetId, "recovery failed/release");
            return;
        }

        SendMove(Hero, &Hero->Object);
        m_bRecoveryActive = true;
        m_dwAttackLastProgress = GetTickCount();
        AbLog("recovery step target=%d dest=%d,%d attempt=%d",
            iTargetId, dest.x, dest.y, m_iRecoveryAttempts);
    }

    // Walkable recovery cell 2-4 tiles away from the target (chebyshev ring),
    // outside occupied/walled/safe-zone cells, with a wall-free straight line
    // from the hero so the short hop cannot cross a wall, and inside the hunt
    // leash so a recovery never drags the hero out of the spot.
    bool CMuHelper::FindRecoveryCell(int iTargetId, POINT& out)
    {
        if (Hero == nullptr || CharactersClient == nullptr)
            return false;

        const int iCharIndex = FindCharacterIndex(iTargetId);
        if (iCharIndex == MAX_CHARACTERS_CLIENT)
            return false;

        CHARACTER* pTarget = &CharactersClient[iCharIndex];
        const int tx = (int)(pTarget->Object.Position[0] / TERRAIN_SCALE);
        const int ty = (int)(pTarget->Object.Position[1] / TERRAIN_SCALE);
        const POINT heroPos = { Hero->PositionX, Hero->PositionY };
        const int iLeash = m_iHuntingDistance + 10;

        bool found = false;
        int bestTravel = 0;
        for (int ring = 2; ring <= 4; ++ring)
        {
            for (int dy = -ring; dy <= ring; ++dy)
            {
                for (int dx = -ring; dx <= ring; ++dx)
                {
                    if (abs(dx) != ring && abs(dy) != ring)
                        continue; // ring cells only

                    const int x = tx + dx;
                    const int y = ty + dy;
                    if (x < 1 || y < 1 || x > 254 || y > 254)
                        continue;

                    const WORD wall = TerrainWall[TERRAIN_INDEX_REPEAT(x, y)];
                    if ((wall & TW_NOMOVE) == TW_NOMOVE
                        || (wall & TW_SAFEZONE) == TW_SAFEZONE)
                        continue;

                    if (!CheckWall(heroPos.x, heroPos.y, x, y))
                        continue;

                    const int travel = ComputeDistanceBetween(heroPos, { x, y });
                    if (travel > iLeash)
                        continue;

                    // Outer rings first (more separation), then short hops.
                    const int score = travel + (ring - 2) * 8;
                    if (!found || score < bestTravel)
                    {
                        found = true;
                        bestTravel = score;
                        out = { x, y };
                    }
                }
            }
        }
        return found;
    }

    // Walkable cell within real attack range of the target cell with a clear
    // line to it — the approach path ends where the first swing is valid
    // instead of anywhere inside a wide pathfinding disk.
    bool CMuHelper::FindApproachCell(int tx, int ty, float fRange, POINT& out)
    {
        if (Hero == nullptr)
            return false;

        const int searchR = (int)ceil(fRange);
        if (searchR < 1)
            return false;

        const POINT heroPos = { Hero->PositionX, Hero->PositionY };
        bool found = false;
        int bestTravel = 0;
        for (int dy = -searchR; dy <= searchR; ++dy)
        {
            for (int dx = -searchR; dx <= searchR; ++dx)
            {
                if (dx == 0 && dy == 0)
                    continue; // the target cell itself is never walkable

                const float fdx = (float)dx;
                const float fdy = (float)dy;
                if (sqrtf(fdx * fdx + fdy * fdy) > fRange - 0.35f)
                    continue; // must end inside real attack range

                const int x = tx + dx;
                const int y = ty + dy;
                if (x < 1 || y < 1 || x > 254 || y > 254)
                    continue;

                const WORD wall = TerrainWall[TERRAIN_INDEX_REPEAT(x, y)];
                if ((wall & TW_NOMOVE) == TW_NOMOVE
                    || (wall & TW_SAFEZONE) == TW_SAFEZONE)
                    continue;

                if (!CheckWall(x, y, tx, ty))
                    continue; // LOS from the approach cell to the target

                const int travel = ComputeDistanceBetween(heroPos, { x, y });
                if (!found || travel < bestTravel)
                {
                    found = true;
                    bestTravel = travel;
                    out = { x, y };
                }
            }
        }
        return found;
    }

    // Chase movement for the locked target. While the hero walks a committed
    // path it is consumed untouched (native run momentum, like a long manual
    // click) — a replan only happens when the target drifts more than a tile
    // from the position the path was planned for. New paths aim at a
    // validated approach cell inside attack range with LOS; the path is long
    // enough that the hero runs smoothly instead of walk-stop micro segments.
    int CMuHelper::PlanChasePath(int iTargetId, CHARACTER* pTarget, float fRange, const char* szReason)
    {
        if (Hero == nullptr || pTarget == nullptr)
            return -1;

        const int tx = (int)(pTarget->Object.Position[0] / TERRAIN_SCALE);
        const int ty = (int)(pTarget->Object.Position[1] / TERRAIN_SCALE);

        if (Hero->Movement)
        {
            const int driftX = tx - m_posChasePlanTarget.x;
            const int driftY = ty - m_posChasePlanTarget.y;
            if (abs(driftX) <= 1 && abs(driftY) <= 1)
                return 0; // progressing toward a still-valid path: keep running

            Hero->Movement = false;
            Hero->Path.PathNum = 0;
        }

        POINT approach = { tx, ty };
        if (FindApproachCell(tx, ty, fRange, approach))
        {
            PATH_t tempPath;
            if (PathFinding2(Hero->PositionX, Hero->PositionY, approach.x, approach.y, &tempPath, 0.0f))
            {
                Hero->Path.Lock.lock();
                const int pathNum = std::min<int>(tempPath.PathNum, MAX_PATH_FIND - 1);
                for (int i = 0; i < pathNum; i++)
                {
                    Hero->Path.PathX[i] = tempPath.PathX[i];
                    Hero->Path.PathY[i] = tempPath.PathY[i];
                }
                Hero->Path.PathNum = pathNum;
                Hero->Path.CurrentPath = 0;
                Hero->Path.CurrentPathFloat = 0;
                Hero->Path.Lock.unlock();

                SendMove(Hero, &Hero->Object);
                m_posChasePlanTarget = { tx, ty };
                AbLog("path issued steps=%d reason=%s dest=%d,%d",
                    pathNum, szReason ? szReason : "chase", approach.x, approach.y);
                return 0;
            }
        }

        // Fallback: native disk pathing around the target cell (old behavior)
        // when no exact approach cell/path is available.
        PATH_t tempPath;
        const float pathLimit = m_bIgnoreHuntRange ? 64.f : (m_iHuntingDistance + fRange);
        if (!PathFinding2(Hero->PositionX, Hero->PositionY, tx, ty, &tempPath, pathLimit))
            return -1;

        Hero->Path.Lock.lock();
        const int pathNum = std::min<int>(tempPath.PathNum, MAX_PATH_FIND - 1);
        for (int i = 0; i < pathNum; i++)
        {
            Hero->Path.PathX[i] = tempPath.PathX[i];
            Hero->Path.PathY[i] = tempPath.PathY[i];
        }
        Hero->Path.PathNum = pathNum;
        Hero->Path.CurrentPath = 0;
        Hero->Path.CurrentPathFloat = 0;
        Hero->Path.Lock.unlock();

        SendMove(Hero, &Hero->Object);
        m_posChasePlanTarget = { tx, ty };
        AbLog("path issued steps=%d reason=%s dest=%d,%d",
            pathNum, szReason ? szReason : "chase", tx, ty);
        return 0;
    }

    int CMuHelper::SelectNextRoamWaypoint(POINT here, DWORD now)
    {
        const int waypointCount = static_cast<int>(m_vecRoamWps.size());
        if (waypointCount == 0)
            return -1;

        const auto isCooling = [&](const int index)
        {
            const auto cooldown = m_mapWpCooldown.find(index);
            return cooldown != m_mapWpCooldown.end() && now < cooldown->second;
        };

        // After a completed/unreachable spot, preserve server-list order and
        // advance cyclically. Only the first selection uses nearest distance.
        if (m_iRoamLastWpIndex >= 0 && m_iRoamLastWpIndex < waypointCount)
        {
            for (int offset = 1; offset <= waypointCount; ++offset)
            {
                const int index = (m_iRoamLastWpIndex + offset) % waypointCount;
                if (!isCooling(index))
                    return index;
            }
            return -1;
        }

        int nearest = -1;
        int nearestDistance = 0;
        for (int index = 0; index < waypointCount; ++index)
        {
            if (isCooling(index))
                continue;

            const int distance = ComputeDistanceBetween(here, m_vecRoamWps[index]);
            if (nearest == -1 || distance < nearestDistance)
            {
                nearest = index;
                nearestDistance = distance;
            }
        }
        return nearest;
    }

    bool CMuHelper::IsRoamPathAllowed() const
    {
        if (Hero == nullptr || Hero->Path.PathNum <= 1)
            return false;

        for (int index = 0; index < Hero->Path.PathNum; ++index)
        {
            const int x = Hero->Path.PathX[index];
            const int y = Hero->Path.PathY[index];
            const WORD wall = TerrainWall[TERRAIN_INDEX_REPEAT(x, y)];
            if ((wall & TW_NOMOVE) == TW_NOMOVE
                || (wall & TW_SAFEZONE) == TW_SAFEZONE)
            {
                return false;
            }
        }
        return true;
    }

    bool CMuHelper::TryStartRoamSegment(POINT destination)
    {
        if (Hero == nullptr || destination.x < 1 || destination.y < 1
            || destination.x > 254 || destination.y > 254)
        {
            return false;
        }

        const WORD destinationWall =
            TerrainWall[TERRAIN_INDEX_REPEAT(destination.x, destination.y)];
        if ((destinationWall & TW_NOMOVE) == TW_NOMOVE
            || (destinationWall & TW_SAFEZONE) == TW_SAFEZONE)
        {
            return false;
        }

        Hero->MovementType = MOVEMENT_MOVE;
        TargetX = destination.x;
        TargetY = destination.y;
        if (!PathFinding2(Hero->PositionX, Hero->PositionY,
            TargetX, TargetY, &Hero->Path))
        {
            return false;
        }

        if (!IsRoamPathAllowed())
        {
            Hero->Path.Lock.lock();
            Hero->Path.PathNum = 0;
            Hero->Path.Lock.unlock();
            return false;
        }

        SendMove(Hero, &Hero->Object);
        return true;
    }

    // With no visible target, tour real server spawn spots continuously. A
    // reached spot gets 1.5 s to expose/respawn a monster, then a short visited
    // cooldown and cyclic advance; unreachable spots receive a longer cooldown.
    void CMuHelper::RoamForHunt()
    {
        if (!m_bRoamEnabled || Hero == nullptr || m_vecRoamWps.empty())
            return;
        if (Hero->Movement)
            return; // MoveHero owns the current segment; never resend per tick.

        const POINT here = { Hero->PositionX, Hero->PositionY };
        const DWORD now = GetTickCount();
        int completedWaypoint = -1;

        if (m_iRoamWpIndex >= 0
            && m_iRoamWpIndex < static_cast<int>(m_vecRoamWps.size()))
        {
            const POINT waypoint = m_vecRoamWps[m_iRoamWpIndex];
            const int distance = ComputeDistanceBetween(here, waypoint);
            if (distance <= kRoamArrivalDistance)
            {
                if (m_dwRoamReachedTick == 0)
                {
                    m_dwRoamReachedTick = now;
                    AbLog("spot reached wp=%d pos=%d,%d", m_iRoamWpIndex,
                        waypoint.x, waypoint.y);
                    return;
                }
                if (now - m_dwRoamReachedTick < kRoamObserveMs)
                    return; // Active-session idle: observe before advancing.

                completedWaypoint = m_iRoamWpIndex;
                m_mapWpCooldown[completedWaypoint] = now + kRoamVisitedCooldownMs;
                m_iRoamLastWpIndex = completedWaypoint;
                m_iRoamWpIndex = -1;
                m_dwRoamReachedTick = 0;
            }
            else
            {
                m_dwRoamReachedTick = 0;
            }
        }

        if (m_iRoamWpIndex == -1)
        {
            const int nextWaypoint = SelectNextRoamWaypoint(here, now);
            if (nextWaypoint == -1)
            {
                if (m_dwRoamIdleLogTick == 0
                    || now - m_dwRoamIdleLogTick >= kRoamIdleLogIntervalMs)
                {
                    AbLog("roam idle active=1 reason=spots-cooling");
                    m_dwRoamIdleLogTick = now;
                }
                return;
            }

            m_dwRoamIdleLogTick = 0;
            m_iRoamWpIndex = nextWaypoint;
            const POINT waypoint = m_vecRoamWps[nextWaypoint];
            const int distance = ComputeDistanceBetween(here, waypoint);
            if (completedWaypoint != -1)
            {
                AbLog("spot advance from=%d to=%d pos=%d,%d",
                    completedWaypoint, nextWaypoint, waypoint.x, waypoint.y);
            }
            else
            {
                AbLog("spot select wp=%d pos=%d,%d dist=%d",
                    nextWaypoint, waypoint.x, waypoint.y, distance);
            }
        }

        const POINT destination = m_vecRoamWps[m_iRoamWpIndex];
        const int distance = ComputeDistanceBetween(here, destination);
        if (distance <= kRoamArrivalDistance)
        {
            m_dwRoamReachedTick = now;
            AbLog("spot reached wp=%d pos=%d,%d", m_iRoamWpIndex,
                destination.x, destination.y);
            return;
        }

        if (TryStartRoamSegment(destination))
        {
            AbLog("roam path steps=%d dest=%d,%d", Hero->Path.PathNum,
                destination.x, destination.y);
            return;
        }

        const int unreachableWaypoint = m_iRoamWpIndex;
        m_mapWpCooldown[unreachableWaypoint] = now + kRoamUnreachableCooldownMs;
        m_iRoamLastWpIndex = unreachableWaypoint;
        m_iRoamWpIndex = -1;
        m_dwRoamReachedTick = 0;
        AbLog("roam unreachable wp=%d dest=%d,%d cooldown=%u",
            unreachableWaypoint, destination.x, destination.y,
            kRoamUnreachableCooldownMs);
    }

    int CMuHelper::SimulateMove(POINT posMove)
    {
        Hero->MovementType = MOVEMENT_MOVE;
        TargetX = (int)posMove.x;
        TargetY = (int)posMove.y;

        if (CheckTile(Hero, &Hero->Object, 1.5f))
            return 1;

        if (PathFinding2((Hero->PositionX), (Hero->PositionY), TargetX, TargetY, &Hero->Path))
        {
            SendMove(Hero, &Hero->Object);
            return 0;
        }

        return 1;
    }

    bool CMuHelper::HasAssignedBuffSkill()
    {
        for (int i = 0; i < m_config.aiBuff.size(); i++)
        {
            if (m_config.aiBuff[i] != 0)
            {
                return true;
            }
        }

        return false;
    }

    ActionSkillType CMuHelper::GetHealingSkill()
    {
        std::vector<ActionSkillType> aiHealingSkills =
        {
            AT_SKILL_HEALING,
            AT_SKILL_HEALING_STR,
        };

        for (int i = 0; i < aiHealingSkills.size(); i++)
        {
            int iSkillIndex = g_pSkillList->GetSkillIndex(aiHealingSkills[i]);
            if (iSkillIndex != -1)
            {
                return aiHealingSkills[i];
            }
        }

        return AT_SKILL_UNDEFINED;
    }

    // Matches AttackWizard() behavior in ZzzInterface.cpp for these skill IDs.
    bool CMuHelper::IsSelfPositionSkill(ActionSkillType iSkill)
    {
        return (
            iSkill == AT_SKILL_NOVA_BEGIN ||
            iSkill == AT_SKILL_NOVA ||
            iSkill == AT_SKILL_HELL_FIRE ||
            iSkill == AT_SKILL_HELL_FIRE_STR ||
            iSkill == AT_SKILL_INFERNO ||
            iSkill == AT_SKILL_INFERNO_STR ||
            iSkill == AT_SKILL_INFERNO_STR_MG
        );
    }

    ActionSkillType CMuHelper::GetDrainLifeSkill()
    {
        std::vector<ActionSkillType> aiDrainLifeSkills =
        {
            AT_SKILL_ALICE_DRAINLIFE,
            AT_SKILL_ALICE_DRAINLIFE_STR
        };

        for (int i = 0; i < aiDrainLifeSkills.size(); i++)
        {
            int iSkillIndex = g_pSkillList->GetSkillIndex(aiDrainLifeSkills[i]);
            if (iSkillIndex != -1)
            {
                return aiDrainLifeSkills[i];
            }
        }

        return AT_SKILL_UNDEFINED;
    }

    int CMuHelper::ObtainItem()
    {
        if (m_iCurrentItem == MAX_ITEMS)
        {
            m_iCurrentItem = SelectItemToObtain();
            if (m_iCurrentItem == MAX_ITEMS)
            {
                return 1;
            }
        }

        ITEM_t* pDrop = &Items[m_iCurrentItem];

        if (!pDrop->Object.Live)
        {
            DeleteItem(m_iCurrentItem);
            return 1;
        }

        TargetX = (int)(Items[m_iCurrentItem].Object.Position[0] / TERRAIN_SCALE);
        TargetY = (int)(Items[m_iCurrentItem].Object.Position[1] / TERRAIN_SCALE);

        int iDistance = ComputeDistanceBetween({ Hero->PositionX, Hero->PositionY }, { TargetX, TargetY });
        if (iDistance <= m_iObtainingDistance)
        {
            if (!CheckTile(Hero, &Hero->Object, 2.0f))
            {
                // Never overwrite a live walk with a pickup detour: PathFinding2
                // writes straight into Hero->Path and the mid-walk SendMove resets
                // CurrentPath (rubberband + walk restart). Retry once the current
                // segment ends, like the native long-click path consumer.
                if (Hero->Movement)
                    return 1;

                if (PathFinding2((Hero->PositionX), (Hero->PositionY), TargetX, TargetY, &Hero->Path))
                {
                    SendMove(Hero, &Hero->Object);
                    AbLog("path issued steps=%d reason=obtain", Hero->Path.PathNum);
                    m_iObtainFails = 0;
                    return 0;
                }

                ++m_iObtainFails;
                if (m_iObtainFails >= 3)
                {
                    DeleteItem(m_iCurrentItem);
                    m_iObtainFails = 0;
                }
                return 1;
            }
            else
            {
                if (SendGetItem == -1)
                {
                    SendGetItem = m_iCurrentItem;
                    SocketClient->ToGameServer()->SendPickupItemRequest(m_iCurrentItem);
                    DeleteItem(m_iCurrentItem);
                    m_iObtainFails = 0;
                }
            }
        }

        return 1;
    }

    bool CMuHelper::ShouldObtainItem(int iItemId)
    {
        ITEM_t* pDrop = &Items[iItemId];
        ITEM* pItem = &pDrop->Item;

        if ((m_config.bPickZen && IsMoneyItem(pItem))
            || (m_config.bPickJewel && IsJewelItem(pItem))
            || (m_config.bPickAncient && IsAncientItem(pItem))
            || (m_config.bPickExcellent && IsExcellentItem(pItem)))
        {
            return true;
        }

        if (m_config.bPickExtraItems)
        {
            std::wstring strDisplayName = GetItemDisplayName(pItem);

            for (const auto& str : m_config.aExtraItems)
            {
                // Check if the search keyword is in the item's display name
                if (strDisplayName.find(str) != std::wstring::npos)
                {
                    return true;
                }
            }
        }

        return m_config.bPickAllItems;
    }

    void CMuHelper::AddItem(int iItemId, POINT posWhere)
    {
        _itemsLock.lock();
        m_setItems.insert(iItemId);
        _itemsLock.unlock();
    }

    void CMuHelper::DeleteItem(int iItemId)
    {
        _itemsLock.lock();
        m_setItems.erase(iItemId);
        _itemsLock.unlock();

        if (iItemId == m_iCurrentItem)
        {
            m_iCurrentItem = MAX_ITEMS;
        }
    }

    int CMuHelper::SelectItemToObtain()
    {
        int iClosestItemId = MAX_ITEMS;
        int iMinDistance = m_config.iObtainingRange;

        std::set<int> setItems;
        {
            _itemsLock.lock();
            setItems = m_setItems;
            _itemsLock.unlock();
        }

        for (const int& iItemId : setItems)
        {
            if (!ShouldObtainItem(iItemId))
            {
                continue;
            }

            int iItemX = (int)(Items[iItemId].Object.Position[0] / TERRAIN_SCALE);
            int iItemY = (int)(Items[iItemId].Object.Position[1] / TERRAIN_SCALE);

            int iDistance = ComputeDistanceBetween({ Hero->PositionX, Hero->PositionY }, { iItemX, iItemY });
            if (iDistance <= iMinDistance)
            {
                iMinDistance = iDistance;
                iClosestItemId = iItemId;
            }
        }

        return iClosestItemId;
    }
}
