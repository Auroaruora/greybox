# Workflow

1. `main` always builds and runs. Nobody commits to `main` directly.

2. One branch per feature. First switch to `main` and download the latest version:

   ```bash
   git switch main && git pull
   ```

   Then create your branch from it and switch to it. Pick a short lowercase name with hyphens, such as `shadow-mapping`:

   ```bash
   git switch -c <branch-name>
   ```

3. Commit and push on your branch as often as you like. Every day or two, bring in what has landed on `main`:

   ```bash
   git pull origin main
   ```

   If there are conflicts, fix them in VS Code, then commit the merge.

4. When the feature is ready, push your branch and open a pull request on GitHub:

   ```bash
   git push
   ```

   - On the repository page, click **Compare & pull request**, or go to **Pull requests → New pull request**.
   - Check that it reads **base: main ← compare: your branch**.
   - The title becomes the commit message on `main`, so write it like one. Add a sentence or two on what changed.
   - The repository admin reviews it. To change something, keep committing and pushing on the same branch; the pull request updates by itself.
   - Pull requests need the repository admin's approval; the admin can merge their own.
   - Merge with **Squash and merge**. All the branch's commits become one commit on `main`, and the branch is deleted on GitHub.

5. After the merge, clean up your computer. Switch to `main` and download the merged commit:

   ```bash
   git switch main && git pull
   ```

   Delete your local branch. It needs a capital `-D`, because after a squash git can't tell the branch was merged:

   ```bash
   git branch -D <branch-name>
   ```

   Forget branches that were deleted on GitHub:

   ```bash
   git fetch --prune
   ```

6. Some files are touched by both people, such as `scenerenderer`, `phong.frag`, `debuglayout`, `app.cpp`, `CMakeLists.txt`, `scenedata` and the export script. For these:
   - Tell the other person before you change one.
   - Add new code; don't reorder, reformat or rename code someone else wrote.
   - If you need to change an interface (a function's parameters, a struct's fields), put only that change in its own small pull request and merge it first, so the other person can build on it right away.

7. Only one person edits a `.blend` file at a time; say so before you start. Exported OBJ files and `assets/materials/palette.json` are not in git: run the Blender export after every pull (see `docs/setup.md`).

8. Before pushing, check that the Debug and Release builds both compile and run without errors.
