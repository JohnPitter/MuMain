# MU Helper functional audit checklist

Reproduce with a character in a non-safe-zone map and inspect the saved helper packet/configuration after each case:

- Set hunt range from minimum to maximum with both +/- controls; verify it clamps at 0 and 6.
- Set obtaining range with both +/- controls; verify it clamps at 1 and 8.
- Enter empty, negative, non-numeric, `999`, and valid values in distance and skill-delay fields. Save and verify distance is 0–15 and delays remain 0–999.
- Toggle original position and long-distance counterattack; move away and verify regrouping and attacking-target selection.
- Assign three attack skills, enable combo, remove/reassign a slot, and verify combo disables when incomplete.
- Toggle skill 2/3 timer versus condition and verify only one mode remains active; exercise each precondition and mob-count subcondition.
- Toggle basic attack fallback with no usable skill and verify attack fallback changes accordingly.
- Open potion/auto-heal/drain-life pages and exercise thresholds 0%, 10%, and 100%; verify potion and healing flags persist.
- Toggle buff duration, party support, party heal, and buff interval; save and reload to verify persistence.
- Add/remove extra item names, including empty input; save/reload and verify the list and pickup flags.
- Enable "Pick all near items" while "Pick selected items" is checked; verify the other box clears, save, and confirm only one pickup mode is serialized. Repeat starting from the opposite mode.
- Uncheck the active pickup mode without touching the other one; verify the other mode's state is left untouched (unchecking must not clear the opposite box).
- Press Initialization and verify basic attack fallback returns to enabled, and PVP self-defense plus friend/guild auto-accept return to disabled — including after a session where those were toggled on for another character.
- Toggle PVP self-defense, friend/guild auto-accept, pet attack modes, repair, and pickup modes; save/reload and verify state.
- Close with Save, Close, and Escape; verify text focus is released and reopening restores the last saved state.

## Auto Battle roaming and Hunt Analyzer

- With no visible monster, the first roam destination is the nearest real spawn spot. At arrival the helper observes for 1.5 seconds, marks the spot visited for 15 seconds, then advances cyclically; an unreachable spot cools down for 60 seconds. Every generated path must avoid `TW_NOMOVE` and `TW_SAFEZONE` cells.
- The Hunt Analyzer starts only after the server confirms Auto Battle as active. Hiding the panel does not pause its monotonic session clock or its 1 Hz metric sample.
- Normal next-level progress is `(Experience - levelBase) / (NextExperience - levelBase)`, where `levelBase` is cumulative EXP at the prior level and is zero at level 1. Master progress uses the same interval rule with the native master lower bound. Both formulas clamp to `[0, 1]` and return zero for an empty interval.
- Total EXP accumulates absolute-counter deltas and credits the old-level remainder plus new-level progress when the normal/master channel changes. Level resets, reconnects and character changes rebase counters instead of creating fake gains.
- Profit is the signed delta of the real Zen wallet, so pickups add and repairs/purchases subtract. Hourly rates are `total * 3600 / elapsedSeconds` and use pt-BR thousands grouping.

## Chasing a target around an obstacle

These are the rules the bot follows once it has locked a monster; they matter
most when a wall, a fence, a cage or a hole sits between the hero and the mob.

- An **approach cell** is a walkable cell inside the real attack range of the
  target with a clear line to it. The line matters for ranged classes too: the
  client refuses an attack through a wall, so a bow standing behind a fence
  would fire nothing. Walkability is judged exactly as the client pathfinder
  judges it, so `TW_NOMOVE`, `TW_NOGROUND`, water, no-attack zones and cells
  another character occupies are all rejected; `TW_SAFEZONE` is rejected too,
  because entering a safe zone auto-stops the helper.
- A **staging cell** is a walkable cell one to three rings beyond attack range.
  It is not an attack position: it exists so the hero can leave a blocked
  corridor and re-plan from the other side of the obstacle.
- Candidates are ordered by distance from the hero, with staging cells and
  cells without a clear line penalised. Up to six of them are confirmed with
  the pathfinder before anything is sent, and a path is only accepted when it
  really ends on the cell asked for -- the client pathfinder otherwise answers
  a blocked destination with a partial path that walks into the obstacle and
  stops there.
- On a stall (about 2.5 s with no progress, or an attack refused because of a
  wall) the bot performs at most **four reposition steps** per stall cycle. Each
  step is a single walk request to a different validated cell, alternating
  sides around the target and widening the ring, and the walk is allowed to
  finish (up to 2.5 s) before the next evaluation. A reposition is never an
  attack, so the attack-speed cadence is untouched.
- Only movement that actually **shortens** the distance to the target counts as
  progress. Sidestepping around a mob that can never be reached does not reset
  the budget, which is what lets the give-up rule below fire at all.
- When the reposition steps are spent, the helper pauses its attacks with an
  exponential backoff (1 s, 2 s, 4 s, then 8 s). After three exhausted cycles,
  or 20 s of uninterrupted stalling, the target is dropped and blacklisted for
  30 s and the bot picks another one.

## Manual control and stopping

The player always outranks the bot. These rules apply to the native MU Helper
and to the Auto Battler alike -- both drive the same hunting loop.

- A real click on the world (walk, attack, pick up, talk, follow) **claims the
  hero**. While the claim holds the helper sends nothing, does not touch the
  hero's path, and does not steer the body: mouse-look behaves exactly as it
  does with the helper off.
- The claim lasts for the whole walk the click started, plus a grace period of
  **1.5 s** after it ends, so a chain of short clicks is not chopped up by the
  bot between them. Every new click restarts the claim. A claim never lasts
  longer than **20 s**, so a movement flag left behind by a desync cannot
  disable the helper for the rest of the session.
- Nothing is counted against the bot while the player is driving: the stall
  detector, the reposition budget and the roam stuck counter are all rearmed,
  so a long manual walk never makes the helper give a target up.
- Before this rule existed the bot cancelled the player's walk within 250 ms
  and replaced it with a chase of its own, or stopped it mid-step to attack.
  The client then restarted the walk from a different cell than the one the
  server was walking the character to, which showed up in game as the hero
  gliding or rubberbanding ("floating").
- A **map change stops the helper**: `/move`, the warp window, a gate, the town
  return after death. The stop is local and immediate -- it does not wait for
  the server to confirm the session is over -- and it also ends the Auto Battler
  session and clears the HUD toggle. Restarting on the new map requires an
  explicit start, exactly like the safe-zone and death stops.
- The hero can never be left walking with no path. If the movement flag is set
  and the path is empty or already consumed, the client stops the hero on the
  spot instead of sliding it forward frame after frame with nothing to walk.
