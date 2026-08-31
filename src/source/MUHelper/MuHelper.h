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

namespace MUHelper
{
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
		void Work();
		int ActivatePet();
		int Buff();
		int BuffTarget(CHARACTER* pTargetChar, ActionSkillType iBuffSkill);
		int RecoverHealth();
		int Heal();
		int HealSelf(ActionSkillType iHealingSkill);
		int DrainLife();
		int ConsumePotion();
		int Attack();
		int RepairEquipments();
		int Regroup();
		void CollectNearbyMonsters();
		ActionSkillType SelectAttackSkill();
		int SimulateAttack(ActionSkillType iSkill);
		int SimulateSkill(ActionSkillType iSkill, bool bTargetRequired, int iTarget);
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
		void BlacklistTarget(int iTargetId, const char* szReason);
		void PurgeBlacklist();
		void ReleaseChaseTarget(int iTargetId, const char* szReason);
		bool ValidateChaseTarget(int iTargetId);
		void UpdateChase(int iTargetId);
		void TrackTargetProgress(int iTargetId);
		void HandleAttackStall(int iTargetId, const char* szWhy);
		bool FindRecoveryCell(int iTargetId, POINT& out);
		bool FindApproachCell(int tx, int ty, float fRange, POINT& out);
		int PlanChasePath(int iTargetId, CHARACTER* pTarget, float fRange, const char* szReason);
		void AbLog(const char* szFormat, ...);
		int ObtainItem();
		int SelectItemToObtain();
		bool ShouldObtainItem(int iItemId);
		ActionSkillType GetHealingSkill();
		ActionSkillType GetDrainLifeSkill();
		bool HasAssignedBuffSkill();
		bool IsSelfPositionSkill(ActionSkillType iSkill);

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
		// Target cell at the moment the current chase path was planned; the
		// path is only abandoned when the target drifts beyond this.
		POINT m_posChasePlanTarget = { 0, 0 };
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
		int m_iTotalCost;
	};

	extern CMuHelper g_MuHelper;
}