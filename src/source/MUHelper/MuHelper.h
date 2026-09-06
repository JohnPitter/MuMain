#pragma once

#include <functional>
#include <array>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <atomic>
#include <vector>

#include "MuHelperData.h"
#include "MuHelperApproach.h"
#include "MuHelperManualControl.h"

namespace MUHelper
{
	// One pending self-repair request. The server answers a refused repair
	// (pet slot without an opened NPC, not enough money) with nothing but a
	// log line, so the client cannot tell success from refusal by the reply:
	// the retry has to be driven by the durability actually changing, or by a
	// backoff. Without this the helper resent the same rejected slot on every
	// 250 ms tick.
	struct RepairAttempt
	{
		int   iSentDurability = 0;
		DWORD dwRetryAt = 0;
	};

	class CMuHelper
	{
	public:
		CMuHelper() = default;
		~CMuHelper() = default;

	public:
		static void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

		ConfigData GetConfig() const;
		void Save(const ConfigData& config);
		void SaveToServer(const ConfigData& config);
		void Load(const ConfigData& config);
		void Start();
		void Stop();
		void RecalculateDistances();
		void SetIgnoreSafeZoneStop(bool ignore);
		void SetIgnoreHuntRange(bool ignore);
		void SetRoamWaypoints(const POINT* pts, int count);
		void SetAutoStopHandler(std::function<void(const char*)> handler);
		void AutoStop(const char* szReason);
		void Toggle();
		// Manual-input arbitration. NotifyManualInput() is called by the client
		// input path (MoveHero) whenever the player personally orders the hero
		// around; the helper then stops emitting anything and stops touching
		// Hero->Path / the action state until the walk that order started ends
		// plus a short grace period. IsManualOverrideActive() reports that state
		// to the per-frame hooks (FaceAttackTarget).
		void NotifyManualInput();
		bool IsManualOverrideActive() const;
		void TriggerStart();
		void TriggerStop();
		bool IsActive() { return m_bActive; }
		bool FaceAttackTarget();
		void AddCost(int iCost) { m_iTotalCost += iCost; }
		int GetTotalCost() { return m_iTotalCost; }

		void AddTarget(int iTargetId, bool bIsAttacking);
		void DeleteTarget(int iTargetId);
		void DeleteAllTargets();

		void AddItem(int iItemId, POINT posDropped);
		void DeleteItem(int iItemId);

	private:
		void WorkLoop(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
		bool YieldToManualControl(DWORD now);
		void Work();
		int ActivatePet();
		int Buff();
		int BuffTarget(CHARACTER* pTargetChar, ActionSkillType iBuffSkill);
		int RecoverHealth();
		DWORD ComputeAttackIntervalMs() const;
		bool IsAttackCadenceReady() const;
		void NoteAttackRequestSent();
		bool IsAttackBackoffActive() const;
		void EnterStallBackoff(int iTargetId);
		void ResetStallState();
		void RepairEquipmentSlot(int iSlot, DWORD now);
		int Heal();
		int HealSelf(ActionSkillType iHealingSkill);
		int DrainLife();
		int ConsumePotion();
		bool TryUseHealthPotion(DWORD now);
		bool TryUseManaPotion(DWORD now);
		int Attack();
		int RepairEquipments();
		int Regroup();
		void CollectNearbyMonsters();
		ActionSkillType SelectAttackSkill();
		int SimulateAttack(ActionSkillType iSkill);
		// bAttackRequest marks the casts the server counts as attacks (damage
		// skills, Drain Life). Only those obey the attack-cadence timer; buffs
		// and heals keep the plain swing gate, exactly as before, because the
		// server excludes Buff/Regeneration skills from its own rate check.
		int SimulateSkill(ActionSkillType iSkill, bool bTargetRequired, int iTarget, bool bAttackRequest = false);
		int SimulateBasicAttack(int iTarget);
		int SimulateComboAttack();
		int GetNearestTarget();
		int GetFarthestAttackingTarget();
		void CleanupTargets();
		int ComputeDistanceByRange(int iRange);
		int ComputeDistanceFromTarget(CHARACTER* pTarget);
		int ComputeDistanceBetween(POINT posA, POINT posB);
		int SimulateMove(POINT posMove);
		void RoamForHunt();
		int SelectNextRoamWaypoint(POINT here, DWORD now);
		bool TryStartRoamSegment(POINT destination);
		bool IsRoamPathAllowed() const;
		bool IsWalkingPath() const;
		void TrackHuntMotion();
		bool IsBlacklisted(int iTargetId);
		void BlacklistTarget(int iTargetId, const char* szReason, DWORD dwCooldownMs = 0);
		void PurgeBlacklist();
		void ReleaseChaseTarget(int iTargetId, const char* szReason);
		bool ValidateChaseTarget(int iTargetId);
		void UpdateChase(int iTargetId);
		void TrackTargetProgress(int iTargetId);
		void HandleAttackStall(int iTargetId, const char* szWhy);
		// Repositioning around an obstacle. ProbeApproach walks the ordered
		// candidate list from MuHelperApproach.h and returns the first cell the
		// client pathfinder genuinely reaches; TryReposition turns that into a
		// single walk request. IsRepositioning guards every caller so an
		// in-flight detour is never cancelled by the next 250 ms tick.
		bool ProbeApproach(const Approach::Request& req, POINT& out, PATH_t& outPath);
		bool CommitPathAndMove(PATH_t& path, POINT dest, const char* szReason);
		bool TryReposition(int iTargetId, float fRange, const char* szWhy);
		bool IsRepositioning() const;
		void ResetRepositionState();
		void NoteRepositionTried(POINT cell);
		int PlanChasePath(int iTargetId, CHARACTER* pTarget, float fRange, const char* szReason);
		void AbLog(const char* szFormat, ...);
		int ObtainItem();
		int SelectItemToObtain();
		bool ShouldObtainItem(int iItemId);
		ActionSkillType GetHealingSkill();
		ActionSkillType GetDrainLifeSkill();
		bool HasAssignedBuffSkill();
		bool IsSelfPositionSkill(ActionSkillType iSkill);
		bool IsLegalLockedTarget(CHARACTER* pTarget) const;
		bool AllowsPlayerTargets() const;

	private:
		ConfigData m_config;
		POINT m_posOriginal;
		std::thread m_timerThread;
		std::atomic<bool> m_bActive;
		std::set<int> m_setTargets;
		std::set<int> m_setTargetsAttacking;
		std::set<int> m_setItems;
		int m_iCurrentItem;
		int m_iCurrentTarget;
		int m_iCurrentBuffIndex;
		int m_iCurrentBuffPartyIndex;
		int m_iCurrentHealPartyIndex;
		int m_iComboState;
		ActionSkillType m_iCurrentSkill;
		int m_iHuntingDistance;
		int m_iObtainingDistance;
		int m_iLoopCounter;
		int m_iSecondsElapsed;
		int m_iSecondsAway;
		bool m_bTimerActivatedBuffOngoing;
		bool m_bPetActivated;
		bool m_bIgnoreSafeZoneStop = false;
		bool m_bIgnoreHuntRange = false;
		bool m_bRoamEnabled = false;
		// Real server spawn spots supplied by the Auto Battler overlay. The first
		// target is nearest; empty spots then advance cyclically after observation.
		std::vector<POINT> m_vecRoamWps;
		int m_iRoamWpIndex = -1;
		int m_iRoamLastWpIndex = -1;
		std::map<int, DWORD> m_mapWpCooldown;
		DWORD m_dwRoamReachedTick = 0;
		DWORD m_dwRoamIdleLogTick = 0;
		int m_iStuckTicks = 0;
		int m_iObtainFails = 0;
		POINT m_posLastStuck = { 0, 0 };
		// Target lock / chase progress state (Auto Battle hardening).
		std::map<int, DWORD> m_mapBlacklist;
		int m_iChaseTarget = -1;
		POINT m_posChaseLast = { 0, 0 };
		DWORD m_dwChaseLastProgress = 0;
		bool m_bChaseRepathed = false;
		// Attack-stall detector / recovery step state. Progress is a target
		// position or hit-animation change, hero movement, or target death;
		// ~2.5 s of attack attempts without any of it triggers a recovery
		// step (one SendMove to a validated cell 2-4 tiles off the target).
		DWORD m_dwAttackLastProgress = 0;
		POINT m_posAttackHeroLast = { 0, 0 };
		POINT m_posAttackTargetLast = { 0, 0 };
		int m_iAttackTargetActionLast = -1;
		int m_iRecoveryAttempts = 0;
		bool m_bRecoveryActive = false;
		// Reposition bookkeeping. A recovery step now walks a real detour
		// instead of a straight-line sidestep, so it needs a grace window (the
		// walk must be allowed to finish before the next stall evaluation), the
		// cells already tried on this stall (so consecutive attempts pick a
		// different side instead of the same blocked one) and the best
		// hero->target distance seen since the lock, which is what separates
		// "the hero moved" from "the hero got closer".
		DWORD m_dwRepositionUntil = 0;
		Approach::Cell m_aTriedCells[Approach::kMaxRepositionAttempts] = {};
		int m_iTriedCells = 0;
		int m_iChaseBestDistance = -1;
		// Attack range of the action currently being attempted, remembered so
		// the stall handler (which runs from the tick watchdog too) knows how
		// far from the target an approach cell may sit.
		float m_fEngageRange = 1.8f;
		// Target cell at the moment the current chase path was planned; the
		// path is only abandoned when the target drifts beyond this.
		POINT m_posChasePlanTarget = { 0, 0 };
		// Attack cadence. The swing-animation gate alone is not a rate limit:
		// a hit, a stun or any server-driven action change pulls the hero out
		// of the swing enum early, and every attack request that never starts
		// a swing (blocked cast, refused hit) leaves no animation at all. Both
		// cases let the fixed 250 ms helper tick become the only pacing, which
		// is faster than the character's real attack speed. m_dwLastAttackSent
		// is the wall clock of the last attack/skill request actually issued;
		// m_iLastSwingAction is the last swing animation observed, whose
		// PlaySpeed carries AttackSpeed/MagicSpeed (see MuHelperPacing.h).
		DWORD m_dwLastAttackSent = 0;
		int m_iLastSwingAction = -1;
		// Unreachable-target backoff. The target lock never releases on its
		// own (3feaaad6), so a mob that cannot be reached at all -- behind a
		// wall the server disagrees about, inside a closed cage -- used to
		// loop through recovery and chase forever. Each exhausted recovery
		// cycle now costs an exponential pause, and after kMaxStallCycles or
		// kUnreachableGiveUpMs the lock is finally dropped.
		int m_iStallCycles = 0;
		DWORD m_dwStallSince = 0;
		DWORD m_dwAttackBackoffUntil = 0;
		// Auto-repair pacing, keyed by equipment slot.
		std::map<int, RepairAttempt> m_mapRepairAttempts;
		DWORD m_dwNextRepairSweep = 0;
		// Healing-potion pacing: one request per potion cooldown, not one per
		// helper tick while the life bar stays under the threshold.
		DWORD m_dwNextPotionRequest = 0;
		// Range hysteresis: once a swing was issued inside range, the attack
		// condition keeps a small tolerance so the hero does not oscillate
		// between walking and stopping on the range boundary.
		bool m_bAttackEngaged = false;
		// Auto-stop on death / safe-zone entry. The handler (owned by the
		// Auto Battler UI) runs the full stop flow once: cancel chase/roam,
		// stop the server session, hide the analyzer, restore the config.
		std::function<void(const char*)> m_AutoStopHandler;
		// Previous-tick Movement flag for the "path completed" transition log.
		bool m_bPrevMovement = false;
		// Who owns the hero: the player or the bot. See MuHelperManualControl.h.
		Manual::State m_manual;
		int m_iTotalCost;
	};

	extern CMuHelper g_MuHelper;
}