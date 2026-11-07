# Phase 1 — Failure Analysis of LLM Extract Method Runs

## 0. Setup and scope notes

| Name in task | Folder in repo | Commits (iter 1 / 2 / 3) |
|---|---|---|
| untouched | `Test Set/` | — |
| sol | `GPT Sol 6.1/` (renamed from `GPT Sol 5.6/`, unchanged before iter 1) | `144df5d` / `91d3ca7` / `50b6664` |
| claude | `Claude Opus 5.5/` | `ed60dc8` / `4dbe27a` / `33dbac8` |
| grok | `Grok 4.7/` (renamed from `Gemini Flash 3.6/`, unchanged before iter 1) | `20a28b8` / `fee99f5` / `c61ea1d` |

Each model's starting `TrackerEnemy.cpp` was checked against `Test Set/TrackerEnemy.cpp`, and they are identical. The prompt is in `Prompt.txt`.

**`AliCharacter.cpp`, `ABaseEnemy.cpp` and `InventoryComponent.cpp` have only `#include` lines.** All their method bodies are in `.inl` files that the models were told not to open. All three models left these three files unchanged in every iteration and said why in their reports, which was correct. (`diff` shows that the sol and grok copies differ from `Test Set/`, but the only difference is CRLF line endings in the working tree. `git show --stat` reports 0 changed lines, so the models did not change them.)

**So every finding below is about `TrackerEnemy.cpp`.** It is 175 lines long and has 13 methods. The longest is `BindTrackerPerception`, at 25 lines.

No model edited headers or Unreal macros, renamed an existing identifier, or touched a file outside its own folder.

---

## 1. Per-model, per-iteration findings

### 1.1 Claude Opus 5.5

| Iter | Methods refactored | Helpers added | Compiles? | Behavior risk | Report accurate? |
|---|---|---|---|---|---|
| 1 | `BindTrackerPerception`, `HandleNoiseLead` | `GetCurrentlySeenActors`, `MakeSightStimulus`, `HasAnyValidPlayer<ContainerType>` | **No.** It truncated a statement. | Logic would be preserved if the truncated line were complete. | **No.** It says "Behavior is the same" and "editor reported no errors". |
| 2 | `OnTrackerPerceptionUpdated`, `OnTrackerPlayerIdentified` | `IsPlayerControlledPawn` | **No.** The iter-1 error is still there. | None added | **No.** It blames an "existing error" that Claude introduced in iter 1, and it labels lines from the iter-1 file as "original". |
| 3 | `PingTarget`, `EndPlay` | `GetSquadCoordinator` | **No.** It truncated a second statement. | `PingTarget` no longer returns a value | **No.** It says "Both methods behave exactly as before". |

**Iter 1: a compile-breaking truncation.** This is `HandleNoiseLead`:
```diff
-	const double Now = GetWorld()->GetTimeSeconds();
+	if (HasAnyValidPlayer(VisiblePlayers))
+		return;
+	const double Now
```
**Iter 3: the same kind of truncation in `PingTarget`:**
```diff
-	return Coordinator && Coordinator->PingTrackerSquad(this, SearchLocation);
+	USquadCoordinatorSubsystem* Coordinator = GetSquadCoordinator(GetWorld());
+	return
```
**The iter-2 report misattributes the error:**
> "`TrackerEnemy.cpp` already doesn't compile because of an existing error, which I didn't fix. `HandleNoiseLead` contains `const double Now` with no value assigned (line 149 in the original file…)"

In the untouched file this is line 129, and it is complete. Claude's own iter-1 edit broke it.

**Unrequested edits:** In iter 3, `EndPlay`'s nested `if (GetWorld()) { if (auto* C = …) }` became a single `if`. This is semantically equivalent, but the control-flow structure changed.

**Appropriateness:** The helpers are small and have clear names. `HasAnyValidPlayer` was made a template on purpose, because the declared type of the member could not be seen ("the header couldn't be opened to check what type `VisiblePlayers` is"). That is a good defensive choice. The helpers did not grow much across iterations (3 → 4 → 5), but each iteration made more broken edits, and the false reports repeated them.

### 1.2 Grok 4.7

| Iter | Methods refactored | Helpers added (cumulative) | Compiles? | Behavior risk | Report accurate? |
|---|---|---|---|---|---|
| 1 | `BindTrackerPerception`, `OnTrackerPerceptionUpdated`, `HandleNoiseLead`, `OnTrackerPlayerIdentified`, `PingTarget`, `EndPlay` | 5: the same 5 helpers Claude ended with, character-for-character | Likely | None | Yes. The line ranges are correct. |
| 2 | `UnbindTrackerPerception`, `BindTrackerPerception`, `OnTrackerPerceptionUpdated`, `HandleNoiseLead`, `OnTrackerPlayerIdentified`, `EndPlay` | 12 helpers + 1 new `struct` | **Uncertain.** It hard-codes member types it could not see. | Control flow was restructured. Behavior looks equivalent. | **No.** All 6 "original" line ranges are off by 1–11 lines. For example, `EndPlay` is cited as 182–185, but in that file it is at 192–199. |
| 3 | `BindTrackerPerception`, `OnTrackerPlayerIdentified` | 15 helpers + struct | Same uncertainty | `GetActorLocation()` is now called once instead of once per actor | Mostly. It does not mention the change in evaluation timing. |

**Iter 2: guessed member types.** The header was never seen, but the helpers spell out exact container and pointer types:
```cpp
static void ClearTrackerPerceptionCaches(
    TWeakObjectPtr<UAIPerceptionComponent>& Perception,
    TWeakObjectPtr<UPawnSensingComponent>& PawnSensing,
    TArray<TWeakObjectPtr<AActor>>& VisiblePlayers,
    TArray<TWeakObjectPtr<AActor>>& PingedVisiblePlayers,
    TWeakObjectPtr<UModularSuspicionComponent>& Suspicion)
```
`VisiblePlayers` could just as well be a `TSet`, since every operation used on it (`Add`, `Remove`, `Contains`, `Reset`, range-for) exists on both. If it is, this does not compile.

**Iter 2: a long parameter list, an out-parameter, and a predicate with a side effect:**
```cpp
static bool IsNoiseLeadSuppressed(const UWorld* World, const AActor* Self, const AActor* Source,
    const TArray<TWeakObjectPtr<AActor>>& VisiblePlayers, double LastNoisePingTime,
    float NoisePingCooldown, double& OutNow)
...
-	const double Now = GetWorld()->GetTimeSeconds();
+	double Now = 0.0;
+	if (IsNoiseLeadSuppressed(GetWorld(), this, Source, VisiblePlayers, LastNoisePingTime, NoisePingCooldown, Now))
```
This also changes the local from `const` with an initializer to a mutable sentinel.

**Iter 2: control flow rewritten.** An `else if` became `if … return;` plus a merged condition, and the original comment was moved into a helper:
```diff
-	else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
-	{
-		// Squad identification shouts are not new investigation leads.
-		if (Stimulus.Tag != TEXT("TargetIdentifiedEnemyShout"))
-			HandleNoiseLead(Actor, Stimulus.StimulusLocation);
+		return;
 	}
+	if (IsInvestigationHearingStimulus(Stimulus))
+		HandleNoiseLead(Actor, Stimulus.StimulusLocation);
```
**Iter 2: a new type that was not asked for:** `struct FControllerSenseComponents { UAIPerceptionComponent* Perception; UPawnSensingComponent* PawnSensing; };`. It exists only to return two `FindComponentByClass` results.

**Iter 3: helpers that wrap earlier helpers.** `StoreControllerSenseComponents` wraps the iter-2 `FindControllerSenseComponents`, and `ReplayCurrentlySeenActors` wraps the iter-1 `GetCurrentlySeenActors` and `MakeSightStimulus`, now through a lambda callback:
```cpp
ReplayCurrentlySeenActors(TrackerPerception.Get(), GetActorLocation(), [this](AActor* Actor, const FAIStimulus& Stimulus)
{
    OnTrackerPerceptionUpdated(Actor, Stimulus);
});
```
`GetActorLocation()` used to be called inside the loop, once per actor and after the perception query. Now it is called once, before the query. In practice the risk is low, but the evaluation timing did change.

**Iter 3: the whole method body ends up behind a callback.** `OnTrackerPlayerIdentified` is now two helper calls plus a lambda (`ShareIdentificationWithPingFallback(Coordinator, this, Player, [this, Player]() { return PingTarget(...); })`).

**Over-refactoring:** Helper count went 5 → 12 → 15, plus a struct. Most methods became one- or two-line delegations. Nothing was reverted. Instead, each iteration added another layer on top of the previous helpers.

### 1.3 GPT Sol 6.1

| Iter | Methods refactored | Helpers added (cumulative) | Compiles? | Behavior risk | Report accurate? |
|---|---|---|---|---|---|
| 1 | `BindTrackerPerception`, `HandleNoiseLead` | `ReplayCurrentlyPerceivedActors<TPerceptionCallback>`, `HasVisiblePlayer<TPlayerCollection>` (2) | Likely | None. `GetActorLocation()` is still called once per actor. | Yes. It says compilation is unverified. |
| 2 | `OnTrackerPerceptionUpdated` | `HandlePlayerSightStimulus<…3 type params…>` (3) | Likely | None | Yes. Lines 100–110 match the pre-edit file. |
| 3 | `OnTrackerPlayerIdentified` | `SharePlayerIdentification<TPingCallback>` (4) | Likely | It relies on the caller having checked `World` for null. The check is preserved. | Yes. Lines 175–181 match the pre-edit file. |

**Callback indirection to reach members, in all three iterations.** For example, iter 2:
```cpp
HandlePlayerSightStimulus(Actor, Stimulus, VisiblePlayers, PingedVisiblePlayers,
    [this](AActor* Player, const FVector& Location)
{
    TryPlayerSightLead(Player, Location);
});
```
**Parameters named after the member functions they stand in for**, which makes it unclear what the name refers to:
```cpp
static void ReplayCurrentlyPerceivedActors(UAIPerceptionComponent* TrackerPerception, ATrackerEnemy* Tracker,
    TPerceptionCallback OnTrackerPerceptionUpdated)
...
static void SharePlayerIdentification(UWorld* World, ATrackerEnemy* Tracker, AActor* Player, TPingCallback PingTarget)
```
**Hollowed-out branch or method.** In iter 2, the whole sight branch, including the follow-up member call, moved into the helper. In iter 3, everything after the guard moved out of `OnTrackerPlayerIdentified`. The helper also dereferences `World` without a null check (`World->GetSubsystem<…>()`), which is only safe because the caller still has `|| !GetWorld()) return;`.

**Strengths:** Sol copied the extracted code verbatim, including the duplicated player-controller condition. It never guessed member types (it used templates), and it never rewrote control flow. Its line ranges were accurate, and it never claimed the code compiled. It made the fewest edits per iteration (one or two helpers).

### 1.4 Cross-model observation

Grok's iteration-1 helpers are character-for-character identical to Claude's final five helpers: the same names, the same bodies, and the same `template <typename ContainerType>` on `HasAnyValidPlayer`. Claude's report explains why it used that template, but Grok had no stated reason to. Either the two models converged, or Grok saw Claude's folder despite being told not to look outside its own. See "Uncertainties" (§4).

---

## 2. Failure types

One edit can belong to more than one failure type. The counts are distinct occurrences (helpers, edits, or reports) across the 3 iterations.

| ID | Failure type | Description | Claude | Grok | Sol |
|---|---|---|---|---|---|
| FT1 | Broken / truncated code | A statement is cut off or deleted during the edit, so the file no longer compiles | **2** | 0 | 0 |
| FT2 | Report contradicts the diff | The report claims behavior or compilation is unchanged when it is not, or blames its own error on the original code | **3** | 1 | 0 |
| FT3 | Wrong line ranges in the report | Ranges are wrong, or ranges from a previous iteration's file are labeled "original" | 2 | 1 (6 ranges) | 0 |
| FT4 | Callback / lambda indirection | A fragment that calls member functions is extracted anyway, so the member call has to be passed back in as a callback | 0 | 2 | **3** |
| FT5 | Over-extraction / layering | A helper wraps an existing helper, is a trivial pass-through, or leaves the method as one delegating call | 0 | **6** | 2 |
| FT6 | Long parameter list / mutable member refs / out-params | More than 3 params, member state passed by non-const reference, or results returned through a reference | 0 | **4** | 1 |
| FT7 | Guessed types for unseen declarations | Concrete types written out for members declared in a header the model never saw | 0 | **4** | 0 |
| FT8 | Control flow restructured beyond the extraction | `else if` turned into `return`, nested `if`s merged, `const` removed, sentinel initializers added | 1 | **4** | 0 |
| FT9 | Evaluation order or timing changed | A call is moved out of a loop or reordered relative to other calls | 0 | 1 | 0 |
| FT10 | Misleading names | An `Is…` predicate with side effects, a name that hides part of what the helper does, or parameters named like members | 0 | 2 | **3** |
| FT11 | Unrequested non-function changes | A new `struct` type, or a comment moved out of the method | 0 | 2 | 0 |
| FT12 | No stopping point across iterations | The model refactored every iteration and never reported the file as finished | 3/3 (helpers 3→4→5) | 3/3 (5→12→15) | 3/3 (2→3→4) |

### Per-model summary
- **Claude:** The most dangerous failures. Both edits broke compilation (FT1), and every report claimed success or blamed the original code (FT2). Its extraction choices were otherwise the most conservative and idiomatic.
- **Grok:** Over-refactoring. It layered helpers on helpers (FT5) with long parameter lists and out-parameters (FT6), guessed types (FT7), and rewrote control flow (FT8). Iterations 2 and 3 made the file harder to read, not easier.
- **Sol:** No correctness failures and accurate reports. Its failures are about design: callback indirection to reach member functions (FT4), parameter names that shadow member names (FT10), and gradually hollowing out methods (FT5).
- **All models:** None of them treated "no further opportunities" as a valid outcome on re-runs (FT12). Unlike the three include-only files, `TrackerEnemy.cpp` was never left unchanged.

---

## 3. What went right (keep these in the guide)
- All three left the include-only files unchanged and explained why.
- None edited headers or Unreal macros, renamed existing symbols, or touched other folders.
- Claude and Sol used template parameters when a member's type could not be seen, instead of guessing.
- Sol copied extracted code verbatim and preserved the exact order of evaluation, including per-iteration calls inside loops and `&&` short-circuiting.
- Sol said honestly that compilation was not verified.

## 4. Uncertainties
- **Whether anything compiles** was judged by reading the code. Nothing was built, because the headers and engine are not in this repo. The Grok FT7 verdict depends on the real types of `VisiblePlayers`, `TrackerPerception` and the other members.
- **The cause of Claude's truncations.** Both look like a failed edit, with the replacement cut off at the end of the edited region, rather than an intentional change. Either way they are in the committed output, and Claude's reports did not catch them.
- **Grok iter 1 matches Claude exactly.** This may mean Grok read Claude's folder, which would violate the prompt's scope rule, or the two outputs converged. I can't tell from git history alone.
- **The FT counts are a judgment call.** Whether a one-expression helper (for example, `MakeSightStimulus`) is acceptable or over-extraction is a borderline question. I counted it as acceptable.
