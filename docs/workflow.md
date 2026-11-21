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

4. When the feature is ready, open a pull request and merge it with Squash and merge; the branch is deleted automatically. Pull requests need the repository admin's approval; the admin can merge their own.

5. Some files are touched by both people, such as `scenerenderer`, `default.frag`, `debuglayout`, `app.cpp`, `CMakeLists.txt`, `scenedata` and the export script. For these:
   - Tell the other person before you change one.
   - Add new code; don't reorder, reformat or rename code someone else wrote.
   - If you need to change an interface (a function's parameters, a struct's fields), put only that change in its own small pull request and merge it first, so the other person can build on it right away.

6. Only one person edits a `.blend` file at a time; say so before you start. `assets/scenes`, `meshes` and `textures` are not in git: run the Blender export after every pull (see `docs/setup.md`).

7. Before pushing, check that the Debug and Release builds both compile and run without errors.
