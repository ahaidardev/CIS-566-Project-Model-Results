# Traceability — refactor-guide-v1.md

Failure types (FT1–FT12) are defined in `phase1-failure-analysis.md` §2. Code examples come from the commits listed there.

Abbreviations:
- **Fowler** = M. Fowler, *Refactoring: Improving the Design of Existing Code*, 2nd ed. (2018). Chapter 1 defines refactoring and the "two hats", chapter 3 covers smells, and chapter 6 covers *Extract Function* and *Inline Function*. *Separate Query from Modifier* is in chapter 11.
- **T&C** = N. Tsantalis & A. Chatzigeorgiou, "Identification of Extract Method Refactoring Opportunities," CSMR 2009, and "Identification of extract method refactoring opportunities for the decomposition of methods," *JSS* 84(10), 2011. These papers cover slice-based extraction of complete computations and the behavior-preservation preconditions.
- **Project** = a constraint given by the project or prompt (`Prompt.txt`).

Model columns: **C** = Claude Opus 5.5, **G** = Grok 4.7, **S** = GPT Sol 6.1. A number is how many times that model showed the failure. "—" means the model never showed it.

| Rule | Rule summary | Source | Evidence / example | C | G | S |
|---|---|---|---|---|---|---|
| G1 | Goal is readability; behavior stays exactly the same | Fowler ch.1 (definition of refactoring); FT2, FT9 | Grok iter 3 moved `GetActorLocation()` out of the per-actor loop | — | 1 | — |
| G2 | Edit only the target `.cpp` files; no headers, no outside files | Project | No violations seen. Possible cross-folder read by Grok (iter 1 matches Claude's helpers exactly; unconfirmed) | — | ? | — |
| G3 | Helpers are `static` functions in one anonymous namespace, declared before use | Project | All models complied | — | — | — |
| G4 | Don't change Unreal macros | Project | All models complied | — | — | — |
| G5 | Make only Extract Method changes; no renames, reformatting, comment moves, or bug fixes | Project; Fowler ch.2 "two hats"; FT8, FT11 | Grok moved the `// Squad identification shouts…` comment into a helper | — | 2 | — |
| G6 | Add only functions; no new types | FT11 | Grok iter 2 added `struct FControllerSenseComponents` | — | 1 | — |
| G7 | "Unchanged" is a valid result | Project; FT12 | No model ever left `TrackerEnemy.cpp` unchanged on a re-run (helpers grew G 5→12→15, S 2→3→4, C 3→4→5) | 3 | 3 | 3 |
| W1 | Name the fragment by intent; extract it if the name replaces a comment | Fowler ch.6 *Extract Function* ("intention vs. implementation"); Fowler ch.3 *Comments* smell | Good example: `IsPlayerControlledPawn` (C, G) | — | — | — |
| W2 | One complete computation: one result or one self-contained effect | T&C (complete computation slice of a single variable or object state) | Grok's `IsNoiseLeadSuppressed` computes a flag and the current time together | — | 1 | — |
| W3 | Single exit; a guard is extracted as a `bool` predicate with the `return` left at the call site | T&C (behavior-preservation precondition on jumps out of the slice); FT8 | Good: `HasAnyValidPlayer` (C, G) / `HasVisiblePlayer` (S) kept `return` at the call site | — | — | — |
| W4 | At most 3 read-only parameters, at most 1 return value | Fowler ch.3 *Long Parameter List*; FT6 | Grok's `IsNoiseLeadSuppressed(...7 params..., double& OutNow)`; Sol's `HandlePlayerSightStimulus` has 5 params | — | 4 | 1 |
| W5 | All dependencies can be passed as plain parameters; no non-public member calls or delegate binding | FT4; Project (static helper, no header edits) | Sol's `[this](AActor* Player, const FVector& Location) { TryPlayerSightLead(Player, Location); }` | — | 2 | 3 |
| W6 | Extract duplicated logic once and use it at every identical site | Fowler ch.3 *Duplicated Code* | Good: `IsPlayerControlledPawn` and `GetSquadCoordinator` each replaced 2 copies (C, G) | — | — | — |
| N1 | No lambdas, callbacks, or member pointers in helpers | FT4 | Grok's `ShareIdentificationWithPingFallback(..., [this, Player]() { return PingTarget(...); })` | — | 2 | 3 |
| N2 | No more than 3 params, no mutable member refs, no out-params | FT6; Fowler ch.3 *Long Parameter List* | Grok's `ClearTrackerPerceptionCaches` takes 5 member fields by non-`const` reference | — | 4 | 1 |
| N3 | No helper-wrapping-helper, no pass-throughs, no single-call method bodies | FT5; Fowler ch.3 *Middle Man*, ch.6 *Inline Function* | Grok's `StoreControllerSenseComponents` wraps `FindControllerSenseComponents`, and `ReleaseTrackerFromSquad` wraps `GetSquadCoordinator` | — | 6 | 2 |
| N4 | Don't guess types; use a template type parameter or don't extract | FT7 | Grok's `TArray<TWeakObjectPtr<AActor>>& VisiblePlayers` for a member it never saw. Good: Claude's `template <typename ContainerType>` | — | 4 | — |
| N5 | Leave short, single-purpose methods alone | Fowler ch.6 (extract when it aids understanding); FT5 | Grok's 3-line `EndPlay` was reduced to a call to `ReleaseTrackerFromSquad` | — | 1 | — |
| N6 | On re-runs, don't restructure existing helpers; report "no further opportunities" when there are none | FT12, FT5 | Grok iter 3 added a layer on top of iter-2 helpers; all models changed the file in every iteration | 3 | 3 | 3 |
| H1 | Name by intent; `Is`/`Has` must have no side effects; the name covers everything the helper does; no names shadowing members | Fowler ch.6 (naming), ch.11 *Separate Query from Modifier*; FT10 | Grok's `IsNoiseLeadSuppressed` writes `OutNow`; Grok's `GetIdentificationCoordinator` hides a validation step; Sol's parameter `TPingCallback PingTarget` shadows a member | — | 2 | 3 |
| H2 | Pass only what the fragment reads; use the original types; `const` for read-only | Fowler ch.6 *Extract Function* mechanics; FT7 | See N4 | — | 4 | — |
| H3 | Return at most one assigned local; otherwise don't extract | Fowler ch.6 mechanics (modified locals become the return value); T&C (one variable per slice) | Grok returns two results through `struct FControllerSenseComponents` and through `double& OutNow` | — | 2 | — |
| H4 | Copy the fragment verbatim | Fowler ch.6 mechanics; FT1, FT8 | Grok rewrote `const double Now = …` as `double Now = 0.0;`. Good: Sol's helpers are verbatim copies | — | 1 | — |
| H5 | Preserve evaluation order, call count, and short-circuit order | Fowler ch.1 (observable behavior); FT9 | Grok iter 3: `ReplayCurrentlySeenActors(TrackerPerception.Get(), GetActorLocation(), …)` calls `GetActorLocation()` once instead of once per actor | — | 1 | — |
| H6 | Leave call-site control flow and guards unchanged | FT8; T&C (preconditions) | Grok turned `else if (Hearing && Sensed) { if (Tag != …) … }` into `return;` + `if (IsInvestigationHearingStimulus(…))`. Claude merged the nested `if` in `EndPlay` | 1 | 4 | — |
| H7 | Replace whole statements; re-read every edited method for truncation | FT1 | Claude: `const double Now` (iter 1), bare `return` in `PingTarget` (iter 3) | 2 | — | — |
| §5.1 | Never change behavior | Fowler ch.1; FT1, FT8, FT9 | See H5, H6, H7 | 3 | 5 | — |
| §5.5 | Never leave a statement truncated | FT1 | See H7 | 2 | — | — |
| §5.6 | Never guess unseen types | FT7 | See N4 | — | 4 | — |
| §5.7 | Don't claim compilation or behavior preservation without verifying; don't blame the original without checking it | FT2 | Claude iter 2: "already doesn't compile because of an existing error", which Claude itself introduced in iter 1. Claude iter 3: "Both methods behave exactly as before" right after truncating `PingTarget` | 3 | 1 | — |
| §6.1–2 | Self-check: every removed line reappears exactly once; statements complete; braces balanced | FT1 | Catches both Claude truncations | 2 | — | — |
| §6.3–9 | Self-check: placement, parameters, types, order, control flow, scope, usefulness | Same sources as G3, W4, N1–N4, H4–H6 | — | — | — | — |
| §6.10 + §7 line column | Report line numbers from the file as it was at the start of the run | FT3 | Grok iter 2 cited `EndPlay` at "original lines 182–185"; in that file it is at 192–199. Claude iter 2 cited "line 149 in the original file" (original is 129) | 2 | 1 | — |
| §7 Compilation field | Report "Built" or "Not built" explicitly | FT2 | Sol stated "Compilation could not be verified" (good). Claude iter 1 said "no errors" while the code was broken | 1 | — | — |
| §7 "Considered but not extracted" | Record rejected candidates and the rule that blocked each | FT12; Project (report files and methods left unchanged) | Makes "no further opportunities" a reported outcome rather than a silent one | — | — | — |
