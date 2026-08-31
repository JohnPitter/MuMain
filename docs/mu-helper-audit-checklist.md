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
