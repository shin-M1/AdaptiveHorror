# Pull Request Review Task

Reusable task for reviewing, merging when explicitly requested, and validating a GitHub Pull Request.

## Purpose

- Review a Pull Request against its task scope.
- Verify build, automation, runtime smoke, logs, and GitHub checks.
- Merge only when explicitly requested.
- Validate `main` after merge.

## Inputs

- PR number:
- Repository:
- Expected base:
- Expected compare:
- Expected commits:
- Dependency branches:

## Source of truth

1. Current user instruction.
2. PR task document.
3. `AGENTS.md`.
4. `REVIEW.md`.
5. `TEST_PLAN.md`.
6. `REQUIREMENTS.md`.

## Scope

### In scope

- PR identity confirmation.
- Diff review.
- Documentation review.
- Pre-merge validation.
- GitHub checks review.
- Merge by normal Merge Commit when explicitly requested.
- Post-merge validation on `main`.

### Out of scope

- Squash merge.
- Rebase merge.
- Direct push to `main`.
- New implementation beyond the PR under review.
- Fixing post-merge failures directly on `main`.
- Continuing into the next task automatically.

## PR identity confirmation

Run:

```powershell
git status --short --branch
git fetch origin
gh auth status
gh pr view <number> --repo <owner/repo>
```

Confirm:

- PR number and URL.
- Base branch.
- Compare branch.
- Expected commits.
- Current state.
- Working tree clean.

## Diff review

Run:

```powershell
git diff --stat origin/<base>...origin/<compare>
git diff --name-status origin/<base>...origin/<compare>
git diff --check origin/<base>...origin/<compare>
```

Review:

- Changed files.
- Production files.
- Protected systems.
- Expected change scope.
- Unrelated changes.

## Documentation review

Confirm:

- Task document matches implementation.
- Status documents are current.
- Human verification items are not claimed as Codex-verified.
- Markdown code blocks are closed.
- No stale citation placeholders or generated-artifact markers.

## Pre-merge validation

Run the task's required validation command. Default:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunCodexValidation.ps1 -MaxParallelActions 4
```

Confirm:

- Generate Project Files.
- Development Editor / Win64 Build.
- Automation.
- Runtime Smoke.
- Current-run Log Scan.
- `git diff --check`.
- Working tree clean.

## GitHub checks

Run:

```powershell
gh pr checks <number> --repo <owner/repo>
```

If checks are absent, report that fact. If required checks fail or are pending, stop without merging.

## Stop conditions

Stop without merging when any of these occur:

- PR base or compare mismatch.
- Missing expected commit or dependency.
- Unrelated production change.
- Protected-system change outside task scope.
- Documentation materially disagrees with implementation.
- Build, Automation, Runtime Smoke, Log Scan, or required GitHub checks fail.
- Unresolved conflicts.
- GitHub CLI unavailable, unauthenticated, or unauthorized.
- Branch protection or approval blocker.

## Merge method

Only when explicitly requested:

```powershell
gh pr merge <number> --repo <owner/repo> --merge
```

Do not use squash or rebase. Do not directly merge and push local `main` as a replacement for GitHub PR merge.

## Post-merge validation

Run:

```powershell
git checkout main
git pull --ff-only origin main
git merge-base --is-ancestor <expected-commit> main
powershell -ExecutionPolicy Bypass -File .\Scripts\RunCodexValidation.ps1 -MaxParallelActions 4
git status --short --branch
```

If validation fails, do not fix on `main`. Report the cause and whether a new fix branch is needed.

## Human-only verification

Report human-only checks separately:

- Human verification required.
- Human PIE not verified.
- Previously human-verified.

Do not convert human PIE results into new Codex claims.

## Final report format

- PR number and URL.
- Base / compare.
- Review result.
- Scope review.
- Production files reviewed.
- Unrelated system changes found.
- Documentation review.
- Build result before merge.
- Automation result before merge.
- Runtime Smoke result before merge.
- Log Scan result before merge.
- GitHub checks result.
- Merge method.
- Merge commit hash.
- Expected commit inclusion in main.
- Build result after merge.
- Automation result after merge.
- Runtime Smoke result after merge.
- Log Scan result after merge.
- Working tree status.
- Human PIE items not verified.
- Remaining follow-up tasks.
- Blockers or approval issues.
