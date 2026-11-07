# Extract Method Refactoring Guide (v1) — C++ / Unreal Engine

Follow every rule in this guide. If a rule conflicts with your idea of a "cleaner" result, the rule wins. If you're unsure whether an extraction is allowed, don't do it.

## 1. Goal and scope

- **G1.** Your goal is to make long or multi-purpose methods easier to read by moving coherent fragments into named helpers, while keeping the program's behavior exactly the same. A refactoring that changes behavior, even slightly, has failed.
- **G2.** Edit only the target `.cpp` files. Do not edit headers (`.h`, `.inl`) or any other file, and do not read files outside the target set.
- **G3.** Put every helper in the same `.cpp`, as a `static` free function inside a single anonymous namespace (`namespace { ... }`) placed after the `#include` lines. Declare each helper before the code that uses it.
- **G4.** Do not add, remove, or change any `UCLASS`, `UPROPERTY`, `UFUNCTION`, `GENERATED_BODY`, or any other macro.
- **G5.** Make only Extract Method changes. Do not rename anything, reformat, reorder methods, move or rewrite comments, or "fix" code you think is wrong. If you notice an existing bug, mention it in the report and leave it alone.
- **G6.** Add only functions. Do not introduce new structs, classes, enums, type aliases, globals, or macros.
- **G7.** Leaving a file unchanged is a valid result. If a file has no method bodies (for example, it only has `#include`s) or nothing meets §2, leave it unchanged and say why.

## 2. When to extract

Extract a fragment only if **all** of the following are true:

- **W1.** You can name it by **what** it does, not how it does it, and that name would replace a comment or a moment of puzzling over the code.
- **W2.** It is one complete computation: it produces one result (a return value) or performs one self-contained effect, and the statements in it belong together. Don't split a computation across the fragment boundary.
- **W3.** It has a single exit. Any `return`, `break`, `continue`, or `goto` in it targets something inside the fragment. The one exception is a guard condition, which you can extract as a `bool` predicate as long as the original `return` stays at the call site.
- **W4.** It needs at most **three** parameters, all read-only (passed by value or `const&`/`const*`), and it returns at most one value.
- **W5.** All of its dependencies can be passed in as plain parameters. It does not call non-public member functions, bind delegates to member functions, or take member function pointers.

- **W6. Duplication:** when the same logic appears at two or more sites in the file, extract it once and use the helper at **every** site where it's identical. Don't use it at a site that only looks similar.

## 3. When NOT to extract

- **N1.** Don't extract if the helper would need a lambda, `std::function`, template callback, or member function pointer to call back into the class. Leave that code in the method.
- **N2.** Don't extract if the helper would need more than three parameters, a non-`const` reference or pointer to member state, or an out-parameter.
- **N3.** Don't wrap an existing helper in another helper, don't create pass-through helpers that just forward to one call, and don't leave a method whose whole body is a single call to a new helper.
- **N4.** Don't guess types. If a parameter's type is declared somewhere you can't see (for example, a member declared in a header), don't write out a concrete type. Either make that parameter a template type parameter and only read from it, or don't extract.
- **N5.** Don't extract from a method that is already short (about 10 lines or fewer) and does one thing.
- **N6.** On a file that has already been refactored, don't restructure, merge, split, or wrap existing helpers. Only extract a new fragment that meets every rule in §2. If there isn't one, report "no further opportunities" and change nothing.

## 4. How to extract

- **H1.** **Naming:** Start the helper's name with a verb that describes its intent. A name beginning with `Is`, `Has`, `Can`, or `Should` must be a side-effect-free function that returns `bool`. The name must describe everything the helper does, so it can't hide a validation step or a side effect. Don't give parameters or helpers the same name as existing members or methods of the class.
- **H2.** **Parameters:** Pass only what the fragment reads. Use the same types the original expressions had. If those types aren't visible, follow N4. Mark pointers and references `const` when the fragment only reads through them.
- **H3.** **Return value:** If exactly one local variable that the fragment assigns is used after it, return that value. If more than one is, don't extract. Never return two values through a struct or out-parameters.
- **H4.** **Copy verbatim:** Move the fragment's statements unchanged. The only change allowed is replacing `this->member` or a member access with the corresponding parameter. Keep every condition, literal, cast, operator order, `const`, and comment exactly as written.
- **H5.** **Preserve evaluation order and count:** Every call that ran inside a loop must still run inside that loop, the same number of times and in the same order relative to other calls. Don't hoist, cache, combine, or reorder calls. Keep `&&` and `||` operands in their original order, so short-circuiting is unchanged.
- **H6.** **Preserve the call site:** Leave the surrounding control flow exactly as it was. Don't turn `else if` into early returns, merge or split nested `if`s, remove `const`, or add sentinel initializers. Any null check or guard that ran before the fragment must still run before the helper call.
- **H7.** **Edit safely:** Replace whole statements. After every edit, re-read the complete modified method and helper, and confirm that every statement is complete (it ends in `;` and has a full expression), that braces balance, and that nothing was truncated.

## 5. Rules you must never break

1. Don't change behavior in any way, including order of operations, number of calls, conditions, return values, or control flow.
2. Don't edit any file except the target `.cpp` files. Don't edit headers.
3. Don't change Unreal macros.
4. Don't rename anything or make unrelated changes.
5. Don't leave a statement truncated or incomplete.
6. Don't guess a type you haven't seen.
7. Don't claim the code compiles or behaves the same unless you have verified it. Don't blame an error on the original code unless you have confirmed the error exists there.

## 6. Self-check before finishing

Answer each question for every changed file. If any answer is "no", fix the problem or revert that extraction.

1. Does every line you removed from a method appear exactly once, unchanged, either in a helper or at the call site? (Compare your edits line by line.)
2. Is every statement complete, are all braces balanced, and is every helper declared before its first use?
3. Is every helper `static` and inside the anonymous namespace, with no other declarations added there?
4. Does every helper have three or fewer read-only parameters, no out-parameters, and no callbacks?
5. Are all parameter types copied from visible declarations, or are they template type parameters?
6. Is every call in the same loop, the same order, and the same short-circuit position as before?
7. Is the control flow at every call site unchanged, with no merged `if`s, rewritten `else`, removed `const`, or moved guards?
8. Are headers, macros, names, comments, and formatting outside the extracted fragments untouched?
9. Does every helper make its calling method clearer, and is none of them a wrapper or pass-through?
10. Are the line numbers in your report taken from the file **as it was at the start of this run**?

## 7. Required report format

Produce exactly this, once per target file:

```
### <FileName>
Status: Refactored | Unchanged

<If Unchanged>
Reason: <one sentence: no method bodies / no fragment met §2 / already refactored, no new opportunities>

<If Refactored>
| Method | Helper (signature) | Lines moved (from file as it was at start of this run) | What it does | Call sites |
|---|---|---|---|---|
| ... | ... | e.g. 60–66 | ... | e.g. 2 (Method A, Method B) |

Considered but not extracted: <method or fragment, rule that blocked it (e.g. N1, N2)>
Behavior preservation: <one line per helper: why order, conditions, and returns are unchanged>
Compilation: Built successfully | Not built (reason)
Pre-existing issues noticed (not changed): <none | description + line, confirmed present before this run>
```
