# Git Runbook for Salamander Fork Maintenance

This runbook is tailored to your current branching model:

- `main`: mirror/integration branch that tracks the original project plus local `.gitignore` changes.
- `customization`: long-lived branch with fork branding/experimental custom changes.
- `feature/<name>`: short-lived branches for individual features/fixes.

The goal is to reduce merge stress, avoid history chaos, and make releases repeatable.

---

## 1) One-time repository setup

### 1.1 Configure remotes

- `origin` = your fork
- `upstream` = original project

```bash
git remote -v
git remote add upstream <URL-OF-ORIGINAL-REPO>   # if missing
git fetch --all --prune
```

### 1.2 Enable conflict memory (`rerere`)

This helps Git remember how you resolved repeated conflicts.

```bash
git config --global rerere.enabled true
git config --global rerere.autoupdate true
```

### 1.3 Safe push default

```bash
git config --global push.default simple
```

---

## 2) Branch policy (simple, low-stress)

1. **Do not rebase shared branches** (`main`, `customization`, opened PR branches).
2. Use **merge commits** for integration (`--no-ff`).
3. Rebase is allowed **only** on private local work that has not been pushed.
4. If rewrite is unavoidable, use `--force-with-lease` (never plain `--force`).

---

## 3) Regular upstream sync (for `main`)

Perform this before any release cycle and at least weekly.

```bash
git checkout main
git fetch upstream origin
git pull --ff-only origin main
git merge --no-ff upstream/main -m "chore(sync): merge upstream/main into main"
# run build/tests here
git push origin main
```

If conflicts appear:

1. Resolve file-by-file (no blind "accept theirs/ours" globally).
2. Build and run smoke checks immediately.
3. Commit conflict resolution (merge commit message already captures context).

---

## 4) Update `customization` from `main`

This keeps branding changes current without rewriting history.

```bash
git checkout customization
git pull --ff-only origin customization
git merge --no-ff main -m "chore(sync): merge main into customization"
# run build/tests here
git push origin customization
```

Notes:

- If recurring conflicts happen in branding files, `rerere` will reduce repeated work over time.
- Keep customization deltas focused; avoid mixing unrelated features into this branch.

---

## 5) Feature branch lifecycle

## 5.1 Start a feature

Base features from `main` by default.

```bash
git checkout main
git pull --ff-only origin main
git checkout -b feature/<short-name>
```

Use `customization` as base only when the feature truly depends on branding-specific code:

```bash
git checkout customization
git pull --ff-only origin customization
git checkout -b feature/<short-name>
```

## 5.2 Keep feature current (without history rewrite)

```bash
git checkout feature/<short-name>
git fetch origin
git merge --no-ff origin/main
# or: git merge --no-ff origin/customization  (if based there)
```

Avoid rebasing after branch is pushed/opened for review.

## 5.3 Merge feature

- Open PR into the same base branch you started from (`main` or `customization`).
- Prefer **Create a merge commit**.
- Keep PRs small to reduce conflict risk.

---

## 6) Release runbook (`v5.0-samandarin-0.1` style)

Use this sequence for every release candidate:

1. Sync `main` from `upstream/main`.
2. Merge `main` into `customization`.
3. Merge approved pending feature branches.
4. Run full build/test/smoke.
5. Tag release from the exact tested commit.

Example:

```bash
git checkout customization
git pull --ff-only origin customization
# ensure this commit is the tested release commit
git tag -a v5.0-samandarin-0.1 -m "Release v5.0-samandarin-0.1"
git push origin v5.0-samandarin-0.1
```

If your deploy artifacts come from `main`, tag on `main` instead; tag only the branch that actually ships.

---

## 7) Conflict-resolution checklist (copy/paste)

When a merge conflicts:

1. `git status` to list conflicted files.
2. Resolve one file at a time.
3. After each logical set, run build/smoke.
4. Verify final diff carefully:
   - `git diff --staged`
   - `git log --graph --oneline --decorate -n 20`
5. Complete merge commit.
6. Push and open/update PR.

Golden rule: if unsure, abort and retry cleanly.

```bash
git merge --abort
```

---

## 8) Emergency recovery commands

### Recover pre-merge/rebase state

```bash
git reflog
git reset --hard <reflog-entry>
```

### Safe force push (only when truly required)

```bash
git push --force-with-lease
```

### View history clearly

```bash
git log --graph --oneline --decorate --all
```

---

## 9) Practical team rules to prevent burnout

1. No direct commits to `main`/`customization` without PR (except emergency hotfix).
2. One feature branch = one concern.
3. No giant “catch-all” PRs.
4. Always run at least a smoke build after conflict resolution.
5. Prefer merge strategy consistency over “perfect linear history”.

---

## 10) Suggested cadence

- **Weekly**: sync `main` from upstream, then sync `customization` from `main`.
- **Per feature**: merge base branch into feature before final review.
- **Per release**: run Section 6 checklist from top to bottom.

---

## 11) Optional naming conventions

- `feature/<area>-<short-topic>`
- `fix/<area>-<bug-id-or-topic>`
- `chore/<topic>`

Consistent naming makes release planning and regression tracing easier.
