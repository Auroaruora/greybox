# Engine architecture

How the engine runs, from `main()` to one frame. Each diagram lists the calls in order as `Class::method()`. Amber marks post-processing; teal marks the camera path.

## 1. Overview

![Engine overview](diagrams/1-overview.svg)

## 2. Initialize

![App::initialize](diagrams/2-initialize.svg)

### 2b. SceneRenderer::initialize

![SceneRenderer::initialize](diagrams/2b-scenerenderer-initialize.svg)

### 2c. App::addScreen

![App::addScreen](diagrams/2c-addscreen.svg)

## 3. Update

![App::update](diagrams/3-update.svg)

### 3b. Screen::update

![Screen::update](diagrams/3b-screen-update.svg)

## 4. Render

![App::render](diagrams/4-render.svg)

### 4b. SceneRenderer::render

![SceneRenderer::render](diagrams/4b-scenerenderer-render.svg)

### 4c. SceneRenderer::drawScene

![SceneRenderer::drawScene](diagrams/4c-drawscene.svg)
