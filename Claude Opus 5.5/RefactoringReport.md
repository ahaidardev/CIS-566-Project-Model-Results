
# Refactoring Report — Iteration 1

## Extract Method Refactoring Report

Extract Method was applied only in `TrackerEnemy.cpp`. The other three files are unchanged. The editor reported no errors in `TrackerEnemy.cpp`; a full project build was not run.

## TrackerEnemy.cpp

### 1. Methods refactored
- **`BindTrackerPerception`** did several separate things: it bound the suspicion events, bound perception, re-sent the actors it already sees, and bound pawn sensing. The part that collects the already-seen actors and the part that builds each sight event were extracted.
- **`HandleNoiseLead`** had a loop in the middle that checks whether any visible player is still valid. That loop was extracted.

The delegate binding code stays in the methods. A free helper function can't reach the class's handler methods without changes to the header, which wasn't allowed.

### 2. Extracted helpers
All helpers are in an anonymous namespace at the top of the file. Line numbers are from the original file.

| Helper | Original lines | What it does |
|---|---|---|
| `GetCurrentlySeenActors(UAIPerceptionComponent*)` | 60–61 | Returns the actors the perception component currently sees. |
| `MakeSightStimulus(const AActor*, const FVector& ReceiverLocation)` | 65 | Builds the sight event (strength 1.0, from the actor's location to the receiver's location) that `BindTrackerPerception` sends for each seen actor. |
| `HasAnyValidPlayer(const ContainerType&)` | 124–128 | Returns true if any entry in the container is still valid; `HandleNoiseLead` returns early when it does. It's a template because the header couldn't be opened to check what type `VisiblePlayers` is. |

Behavior is the same: the order of calls, the early returns and the values passed are all unchanged.

## Unchanged files
- **`AliCharacter.cpp`**, **`ABaseEnemy.cpp`** and **`InventoryComponent.cpp`**: each one only has `#include` lines that pull in `.inl` files. None of them contain method bodies, so there was nothing to extract. The actual method code is in those `.inl` files, which were off-limits.

---

# Refactoring Report — Iteration 2

I made one Extract Method change, in `TrackerEnemy.cpp`, and left the other three files unchanged. I didn't build it.

**`TrackerEnemy.cpp` already doesn't compile because of an existing error, which I didn't fix.** `HandleNoiseLead` contains `const double Now` with no value assigned (line 149 in the original file, about 154 now). It probably should read `const double Now = GetWorld()->GetTimeSeconds();`, but I didn't change it because that isn't an Extract Method refactoring.

## TrackerEnemy.cpp — refactored

1. **Methods changed:** `OnTrackerPerceptionUpdated` and `OnTrackerPlayerIdentified`. Both had the same multi-part check for "is this a pawn controlled by a player". Pulling it into one helper removes the duplicate and makes each method easier to read.
2. **New helper:** `IsPlayerControlledPawn(const APawn* Pawn)`, a static function in the existing anonymous namespace.
   - **Came from:** original line 100 (`!Player || !Player->GetController() || !Player->GetController()->IsPlayerController()`) and lines 159–160 (the same check on `PlayerPawn`, followed by `|| !GetWorld()`).
   - **What it does:** returns true only if the pawn isn't null, has a controller, and that controller belongs to a player. Each call site now uses `!IsPlayerControlledPawn(...)`, so behavior is the same. The `!GetWorld()` check stays where it was in `OnTrackerPlayerIdentified`.
3. **Other methods in this file:** I left these alone because they're already short or already use helpers in the anonymous namespace (`GetCurrentlySeenActors`, `MakeSightStimulus`, `HasAnyValidPlayer`). `BindTrackerPerception` and `UnbindTrackerPerception` mostly bind and unbind member event handlers. Moving that into free functions would mean taking member pointers to handlers that may be private, which could break the build without a header change.

## AliCharacter.cpp, ABaseEnemy.cpp, InventoryComponent.cpp — unchanged

These files contain only `#include` lines. All the method code is in `.inl` files under `AliCharacter/`, `ABaseEnemy/` and `InventoryComponent/`. The task said not to open or edit any other files, so there was nothing to refactor here. Refactoring these classes would require permission to edit those `.inl` files.

---

# Refactoring Report — Iteration 3

I made one small Extract Method change, in `TrackerEnemy.cpp`, and left the other three files unchanged. **That file still won't compile because of an error that was already there** (see the last section). I didn't run a build.

## TrackerEnemy.cpp — changed

1. **What I refactored and why:** `PingTarget` (original lines 183–184) and `EndPlay` (original lines 191–197) each had their own null-checked lookup of the squad coordinator subsystem. I moved that shared lookup into one helper. Most of the file already uses helpers in an anonymous namespace (`GetCurrentlySeenActors`, `MakeSightStimulus`, `HasAnyValidPlayer`, `IsPlayerControlledPawn`), so there was little else worth extracting.
2. **Extracted helper:** `static USquadCoordinatorSubsystem* GetSquadCoordinator(const UWorld* World)`, added to the existing anonymous namespace.
   - **Came from:** original lines 183–184 (`PingTarget`) and 191–197 (`EndPlay`).
   - **What it does:** returns `World->GetSubsystem<USquadCoordinatorSubsystem>()`, or `nullptr` if the world is null.
   - Both methods behave exactly as before. `EndPlay` now uses a single `if` instead of two nested ones.

## AliCharacter.cpp, ABaseEnemy.cpp, InventoryComponent.cpp — unchanged

These files only contain `#include`s and comments. Every method body is in `.inl` files (for example `AliCharacter/01_VisibilityAndConstruction.inl`), which I wasn't allowed to open or edit. There's nothing in these `.cpp` files to extract.

## Existing compile error in TrackerEnemy.cpp

In `HandleNoiseLead`, the line `const double Now` (around line 159 now, 154 originally) has no value and no semicolon. It was like this before my change, so the file can't compile as it stands. Fixing it isn't an Extract Method change, and I can't tell what value was intended, so I didn't touch it. It's probably meant to be `const double Now = GetWorld()->GetTimeSeconds();`, but this should be confirmed.

