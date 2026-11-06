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
