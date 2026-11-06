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
