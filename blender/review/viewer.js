import * as THREE from "three";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";

const SLOT = /^slot(\d+):(.*)$/;
const VIEWS = {
  Digit1: [0, 0, 1], Numpad1: [0, 0, 1],
  Digit3: [1, 0, 0], Numpad3: [1, 0, 0],
  Digit7: [0, 1, 0.0001], Numpad7: [0, 1, 0.0001],
};

export class Viewer {
  constructor(container) {
    this.container = container;
    this.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    this.renderer.setPixelRatio(window.devicePixelRatio);
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    container.prepend(this.renderer.domElement);
    this.renderer.domElement.tabIndex = 0;
    this.scene = new THREE.Scene();
    this.camera = new THREE.PerspectiveCamera(35, 1, 0.001, 100);
    this.scene.add(new THREE.HemisphereLight(0xffffff, 0x8a96a0, 1.6));
    const sun = new THREE.DirectionalLight(0xffffff, 1.8);
    sun.position.set(2, 4, 3);
    this.scene.add(sun);
    const fill = new THREE.DirectionalLight(0xffffff, 0.5);
    fill.position.set(-3, 1, -2);
    this.scene.add(fill);
    this.controls = new OrbitControls(this.camera, this.renderer.domElement);
    this.controls.mouseButtons = { LEFT: THREE.MOUSE.ROTATE, MIDDLE: THREE.MOUSE.ROTATE, RIGHT: THREE.MOUSE.PAN };
    this.controls.enableDamping = false;
    this.controls.addEventListener("change", () => this.draw());
    this.loader = new GLTFLoader();
    this.textures = new THREE.TextureLoader();
    this.materials = new Map();
    this.model = null;
    this.grid = null;
    this.centre = new THREE.Vector3();
    this.radius = 1;
    this.onKey = (event) => this.handleKey(event);
    window.addEventListener("keydown", this.onKey);
    this.observer = new ResizeObserver(() => this.resize());
    this.observer.observe(container);
    this.resize();
  }

  dispose() {
    window.removeEventListener("keydown", this.onKey);
    this.observer.disconnect();
    this.controls.dispose();
    this.renderer.dispose();
  }

  resize() {
    const { clientWidth: w, clientHeight: h } = this.container;
    if (!w || !h) return;
    this.renderer.setSize(w, h, false);
    this.camera.aspect = w / h;
    this.camera.updateProjectionMatrix();
    this.draw();
  }

  draw() {
    this.renderer.render(this.scene, this.camera);
  }

  async load(url) {
    const gltf = await this.loader.loadAsync(url);
    const keep = this.model ? this.camera.position.clone().sub(this.controls.target) : null;
    if (this.model) this.scene.remove(this.model);
    this.model = gltf.scene;
    this.scene.add(this.model);
    const box = new THREE.Box3().setFromObject(this.model);
    const sphere = box.getBoundingSphere(new THREE.Sphere());
    this.centre.copy(sphere.center);
    this.radius = Math.max(sphere.radius, 0.01);
    if (this.grid) this.scene.remove(this.grid);
    const size = Math.max(0.2, Math.ceil(this.radius * 4 * 10) / 10);
    this.grid = new THREE.GridHelper(size, 10, 0x9aa6b0, 0xc5ced5);
    this.grid.position.y = box.min.y;
    this.grid.material.transparent = true;
    this.grid.material.opacity = 0.5;
    this.scene.add(this.grid);
    this.camera.near = this.radius * 0.01;
    this.camera.far = this.radius * 100;
    if (keep && Math.abs(keep.length() - this.radius * 3.2) < this.radius * 6) {
      this.controls.target.copy(this.centre);
      this.camera.position.copy(this.centre).add(keep.setLength(this.distance()));
      this.camera.updateProjectionMatrix();
      this.controls.update();
    } else {
      this.view([0.75, 0.55, 1]);
    }
  }

  distance() {
    return this.radius / Math.sin(THREE.MathUtils.degToRad(this.camera.fov / 2)) * 1.1;
  }

  view(direction) {
    const d = new THREE.Vector3(...direction).normalize();
    this.controls.target.copy(this.centre);
    this.camera.position.copy(this.centre).add(d.multiplyScalar(this.distance()));
    this.camera.updateProjectionMatrix();
    this.controls.update();
    this.draw();
  }

  handleKey(event) {
    if (!this.container.isConnected || event.target.closest("input, textarea, select, [contenteditable]")) return;
    if (VIEWS[event.code]) {
      const sign = event.ctrlKey || event.metaKey ? -1 : 1;
      this.view(VIEWS[event.code].map((v) => v * sign));
      event.preventDefault();
    } else if (["Period", "NumpadDecimal", "Home"].includes(event.code)) {
      this.view(this.camera.position.clone().sub(this.controls.target).toArray());
      event.preventDefault();
    }
  }

  getMaterial(name, resolve) {
    if (!this.materials.has(name)) {
      const info = resolve(name);
      const material = new THREE.MeshStandardMaterial({ roughness: 0.85, metalness: 0 });
      if (info.texture) {
        const texture = this.textures.load(info.texture, () => this.draw());
        texture.colorSpace = THREE.SRGBColorSpace;
        texture.flipY = false;
        material.map = texture;
      } else {
        material.color.set(info.color || "#888888");
      }
      if (info.emit) {
        material.emissive.set(info.emit);
        material.emissiveIntensity = Math.min(info.emitStrength || 1, 3);
      }
      this.materials.set(name, material);
    }
    return this.materials.get(name);
  }

  setColours(spec, resolve) {
    if (!this.model) return;
    for (const node of this.model.children) {
      const part = node.name.split("_").slice(1).join("_");
      node.traverse((mesh) => {
        if (!mesh.isMesh) return;
        if (mesh.userData.slot === undefined) {
          const match = SLOT.exec(mesh.material.name) || [null, "0", ""];
          mesh.userData.slot = Number(match[1]);
          mesh.userData.original = match[2];
        }
        const slot = mesh.userData.slot;
        const value = spec ? spec[part] ?? spec["*"] : null;
        let name = mesh.userData.original;
        if (Array.isArray(value)) name = value[slot] || name;
        else if (value && slot === 0) name = value;
        mesh.material = this.getMaterial(name, resolve);
      });
    }
    this.draw();
  }
}
