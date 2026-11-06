# 1st iteration

Extract Method was applied only in `Grok 4.7\TrackerEnemy.cpp`. The other three files have no method bodies to extract.

### TrackerEnemy.cpp
`BindTrackerPerception`, `OnTrackerPerceptionUpdated`, `HandleNoiseLead`, `OnTrackerPlayerIdentified`, `PingTarget`, and `EndPlay` each mixed a distinct step with the rest of the method. Those steps are now file-local helpers. Existing method, variable, and class names are unchanged, and the control flow is the same.

- `GetCurrentlySeenActors` came from original lines 60–61. It copies the actors currently perceived by sight.
- `MakeSightStimulus` came from original lines 64–65. It builds the same successful sight stimulus used to replay those actors.
- `IsPlayerControlledPawn` came from original lines 77 and 138–139. It checks that a pawn has a player controller.
- `HasAnyValidPlayer` came from original lines 124–128. It reports whether any visible-player pointer is still valid.
- `GetSquadCoordinator` came from original lines 159–160 and 167–169. It returns the squad coordinator for a world, or null when there is no world.

### AliCharacter.cpp
Left unchanged. The file only includes headers and `.inl` files, so there is no method body to extract.

### ABaseEnemy.cpp
Left unchanged. The file only includes headers and `.inl` files, so there is no method body to extract.

### InventoryComponent.cpp
Left unchanged. The file only includes headers and `.inl` files, so there is no method body to extract.

# iteration 2

The Extract Method refactoring is done. Only `TrackerEnemy.cpp` contained method bodies that could be split without changing behavior. The other three files are unchanged.

### TrackerEnemy.cpp

`UnbindTrackerPerception` both removed delegates and cleared cached perception state. The cache reset (original lines 72–76) is now `ClearTrackerPerceptionCaches`, which resets the perception, pawn-sensing, and suspicion pointers and both player lists.

`BindTrackerPerception` mixed component lookup with delegate binding. The controller component lookup (original lines 90–91) is now `FindControllerSenseComponents`, which returns the controller's perception and pawn-sensing components.

`OnTrackerPerceptionUpdated` handled sight and hearing in one function.
- `TrackSightedPlayer` (original lines 107–116) updates the visible and pinged player lists and reports whether a new sight lead should be tried.
- `IsInvestigationHearingStimulus` (original lines 118–121) accepts a sensed hearing stimulus that is not an identification shout.

`HandleNoiseLead` mixed suppression checks with the ping. The checks (original lines 147–153) are now `IsNoiseLeadSuppressed`, which rejects a noise lead from self or another enemy, while players are already visible, or while the cooldown is active.

`OnTrackerPlayerIdentified` mixed player validation with squad sharing. The validation (original lines 161–164) is now `GetIdentificationCoordinator`, which returns the squad coordinator only for a player-controlled pawn in a valid world.

`EndPlay` both unbound perception and released the squad slot. The release (original lines 182–185) is now `ReleaseTrackerFromSquad`, which releases this tracker from the squad coordinator when one exists.

### Unchanged

`AliCharacter.cpp`, `ABaseEnemy.cpp`, and `InventoryComponent.cpp` only include other translation units. They have no method bodies, so there was no Extract Method opportunity.
