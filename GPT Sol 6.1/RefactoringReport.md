# 1st iteration

### TrackerEnemy.cpp
Refactored two methods to separate distinct tasks:

| Method | Extracted helper | Original lines | Purpose |
|---|---|---|---|
| `BindTrackerPerception` | `ReplayCurrentlyPerceivedActors` | 60–66 | Retrieves currently sight-perceived actors and replays their initial sight events, separating initialization from delegate binding. |
| `HandleNoiseLead` | `HasVisiblePlayer` | 124–128 | Checks whether any visible-player reference is valid, separating this condition from cooldown and ping handling. |

Both helpers are `static` functions inside an anonymous namespace. Existing iteration order, event arguments, and early-return behavior were preserved.

### AliCharacter.cpp
Unchanged. Contains includes and commented-out code, but no active method bodies eligible for extraction. Included `.inl` files were not inspected.

### aBaseEnemy.cpp
Unchanged (`ABaseEnemy.cpp` on disk). Contains only includes, with no method bodies eligible for extraction.

### InventoryComponent.cpp
Unchanged. Contains only includes, with no method bodies eligible for extraction.

### Validation
Only the four permitted files were read; only `TrackerEnemy.cpp` was edited. No headers or Unreal macros were changed.

Scoped diagnostics reported no errors. **Compilation could not be verified** because the build command `Build.BuildSolution` is unavailable in this environment.

# iteration 2

Only the four specified files in `GPT Sol 6.1` were inspected. Only `TrackerEnemy.cpp` was modified.

| File | Result |
|---|---|
| `AliCharacter.cpp` | Unchanged. Contains includes and no active method bodies; implementations reside in out-of-scope `.inl` files. |
| `aBaseEnemy.cpp` (stored as `ABaseEnemy.cpp`) | Unchanged. Contains includes only, with method implementations in out-of-scope `.inl` files. |
| `InventoryComponent.cpp` | Unchanged. Contains includes only, with method implementations in out-of-scope `.inl` files. |
| `TrackerEnemy.cpp` | Refactored `OnTrackerPerceptionUpdated` to separate sight processing from hearing processing. |

**Extracted helper**
- **Name:** `HandlePlayerSightStimulus`
- **Original range:** `TrackerEnemy.cpp`, lines **100–110**, before edits.
- **Purpose:** Validates a player-controlled pawn, removes lost players from visibility collections, or records a visible player and invokes sight-lead processing.
- **Placement:** Static helper in the existing anonymous namespace. Collections and a callback are passed as parameters, avoiding header changes.

Existing names and Unreal macros were unchanged. Checks, operation order, and hearing logic were preserved.

**Validation:** File diagnostics reported no errors. Full compilation remains **unverified** because the build could not execute: `Build.BuildSolution` is unavailable.
