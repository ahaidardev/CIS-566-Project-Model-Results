# Extract Method Refactoring Report

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
