import { Viewer } from "/viewer.js";

const app = document.getElementById("app");
const STATUS_NAMES = { used: "In use", idea: "Ideas", shelved: "Shelved" };
const STATUS_ONE = { used: "In use", idea: "Idea", shelved: "Shelved" };
const REVIEW = [["", "Any review"], ["new", "Needs review"], ["redo", "Redo asked"], ["ok", "All ok"], ["none", "No model"]];
const AREA_ORDER = ["desk", "wall", "shelf", "print", "furniture", "living", "kitchen", "entry", "shell"];
const ui = { tab: "used", search: "", area: "", level: "", review: "", selected: null, adding: false };
const page = {};
let list = null;
let viewer = null;
let asset = null;

function h(tag, props, ...children) {
  const node = document.createElement(tag);
  for (const [key, value] of Object.entries(props || {})) {
    if (value === undefined || value === null || value === false) continue;
    if (key === "class") node.className = value;
    else if (key === "text") node.textContent = value;
    else if (key === "style") node.style.cssText = value;
    else if (key.startsWith("on")) node.addEventListener(key.slice(2), value);
    else if (key in node && typeof value !== "string") node[key] = value;
    else node.setAttribute(key, value === true ? "" : value);
  }
  for (const child of children.flat(Infinity)) {
    if (child === null || child === undefined || child === false) continue;
    node.append(child instanceof Node ? child : document.createTextNode(String(child)));
  }
  return node;
}

function fill(node, ...children) {
  node.replaceChildren(...children.flat(Infinity).filter((c) => c !== null && c !== undefined && c !== false));
}

function options(pairs, current) {
  return pairs.map(([value, label]) => h("option", { value, text: label, selected: value === current }));
}

function pretty(id) {
  const words = id.replace(/([A-Z])/g, " $1").replace(/(\d+)/g, " $1").trim().toLowerCase();
  return words.charAt(0).toUpperCase() + words.slice(1);
}

async function api(path, body) {
  const response = await fetch(path, body ? { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(body) } : {});
  const result = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(result.error || response.statusText);
  return result;
}

function toast(text, isError) {
  const node = document.getElementById("toast");
  node.textContent = text;
  node.className = "show" + (isError ? " error" : "");
  clearTimeout(toast.timer);
  toast.timer = setTimeout(() => (node.className = ""), 1800);
}

function dot(state) {
  return h("span", { class: "dot " + state, title: { ok: "ok", redo: "redo asked", redoImage: "redo with images", new: "not reviewed" }[state] });
}

function route() {
  if (viewer) {
    viewer.dispose();
    viewer = null;
  }
  const match = location.hash.match(/^#\/asset\/([A-Za-z0-9]+)(?:\/edit\/(r\d+)|\/([a-z]+)(?:\/([A-Za-z0-9]+))?)?/);
  const shown = match ? showAsset(match[1], { edit: match[2], variant: match[3], surface: match[4] })
    : location.hash.startsWith("#/requests") ? showRequests() : showList();
  shown.catch((error) => {
    app.replaceChildren(h("div", { class: "empty", text: "Could not load: " + error.message }));
  });
}

function reviewOf(a) {
  if (!a.variants.length) return "none";
  if (a.variants.some((v) => v.state === "redo" || v.state === "redoImage")) return "redo";
  if (a.variants.some((v) => v.state === "new")) return "new";
  return "ok";
}

function isShown(a) {
  if (a.status !== ui.tab) return false;
  if (ui.area && a.area !== ui.area) return false;
  if (ui.level && a.level !== ui.level) return false;
  if (ui.review && reviewOf(a) !== ui.review) return false;
  if (!ui.search) return true;
  return [a.id, pretty(a.id), a.why].join(" ").toLowerCase().includes(ui.search);
}

function areaRank(area) {
  const i = AREA_ORDER.indexOf(area);
  return i < 0 ? AREA_ORDER.length : i;
}

async function showList() {
  list = await api("/api/list");
  document.title = "Assets";
  page.tabs = h("div", { class: "tabs" });
  page.rows = h("div", { class: "rows" });
  page.side = h("aside", { class: "side" });
  const levels = Object.entries(list.levels).map(([id, l]) => [id, id + " " + l.name]);
  const filters = h("div", { class: "filters" },
    h("input", { type: "search", placeholder: "Search", value: ui.search, oninput: (e) => { ui.search = e.target.value.trim().toLowerCase(); renderRows(); } }),
    h("select", { onchange: (e) => { ui.area = e.target.value; renderRows(); } }, options([["", "All areas"], ...list.areas.sort((a, b) => areaRank(a) - areaRank(b)).map((a) => [a, a])], ui.area)),
    h("select", { onchange: (e) => { ui.level = e.target.value; renderRows(); } }, options([["", "All levels"], ...levels], ui.level)),
    h("select", { onchange: (e) => { ui.review = e.target.value; renderRows(); } }, options(REVIEW, ui.review)));
  const head = h("div", { class: "list-head" }, switcher("assets"),
    h("button", { class: "small", style: "margin-left:auto", onclick: () => { ui.adding = true; ui.selected = null; renderRows(); renderSide(); } }, "+ Add asset"));
  app.replaceChildren(h("div", { class: "list-page" }, h("div", { class: "list-main" }, head, page.tabs, filters, page.rows), page.side));
  renderRows();
  renderSide();
}

function renderTabs() {
  fill(page.tabs, ...Object.entries(STATUS_NAMES).map(([status, label]) => {
    const count = list.assets.filter((a) => a.status === status).length;
    return h("button", { class: "tab" + (ui.tab === status ? " on" : ""), onclick: () => { ui.tab = status; renderRows(); } }, label + " " + count);
  }));
}

function renderRows() {
  renderTabs();
  const shown = list.assets.filter(isShown).sort((a, b) => areaRank(a.area) - areaRank(b.area) || a.area.localeCompare(b.area) || a.id.localeCompare(b.id));
  const nodes = [];
  let area = null;
  for (const a of shown) {
    if (a.area !== area) {
      area = a.area;
      nodes.push(h("div", { class: "group", text: area + " · " + shown.filter((x) => x.area === area).length }));
    }
    const first = a.variants.find((v) => v.thumb);
    nodes.push(h("div", {
      class: "row" + (ui.selected === a.id ? " on" : ""),
      "data-id": a.id,
      onclick: () => {
        ui.selected = a.id;
        ui.adding = false;
        page.rows.querySelectorAll(".row").forEach((row) => row.classList.toggle("on", row.dataset.id === a.id));
        renderSide();
      },
      ondblclick: () => (location.hash = "#/asset/" + a.id),
    },
      first ? h("img", { class: "thumb", src: first.thumb, loading: "lazy", alt: "" }) : h("div", { class: "thumb" }),
      h("div", { style: "min-width:0" }, h("div", { class: "name", text: pretty(a.id) }), h("div", { class: "why", text: a.why })),
      h("span", { class: "pill", text: a.level }),
      a.variants.length ? h("div", { class: "dots" }, a.variants.map((v) => dot(v.state))) : h("div", { class: "none", text: "no model" })));
  }
  if (!nodes.length) nodes.push(h("div", { class: "empty", text: "Nothing here with these filters." }));
  fill(page.rows, ...nodes);
}

function replaceSummary(summary) {
  const i = list.assets.findIndex((a) => a.id === summary.id);
  if (i < 0) list.assets.push(summary);
  else list.assets[i] = summary;
  renderRows();
  renderSide();
}

async function update(id, changes) {
  try {
    replaceSummary(await api("/api/asset/" + id, changes));
    toast("Saved");
    return true;
  } catch (error) {
    toast(error.message, true);
    return false;
  }
}

function renderSide() {
  if (ui.adding) return renderAdd();
  const a = list.assets.find((x) => x.id === ui.selected);
  if (!a) {
    fill(page.side, h("div", { class: "empty", text: "Pick an asset to see why it is here." }));
    return;
  }
  const first = a.variants.find((v) => v.thumb);
  const status = h("select", { onchange: (e) => update(a.id, { status: e.target.value }) }, options(Object.entries(STATUS_ONE), a.status));
  const why = h("textarea", { text: a.why, onblur: async () => {
    const value = why.value.trim();
    if (value === a.why) return;
    if (!value) { why.value = a.why; return toast("Why can't be empty", true); }
    await update(a.id, { why: value });
  } });
  why.value = a.why;
  const levels = Object.entries(list.levels).map(([id, l]) => [id, id + " " + l.name]);
  fill(page.side, 
    h("div", { class: "side-head" }, h("h2", { text: pretty(a.id) }), h("button", { class: "small", onclick: () => (location.hash = "#/asset/" + a.id) }, "Open →")),
    h("div", { class: "mono muted", text: a.id }),
    first ? h("img", { class: "big", src: first.thumb, alt: "" }) : h("div", { class: "big empty", text: "No model yet" }),
    h("div", { class: "label", text: "Status" }), status,
    h("div", { class: "pair" },
      h("div", {}, h("div", { class: "label", text: "Level" }), h("select", { onchange: (e) => update(a.id, { level: e.target.value }) }, options(levels, a.level))),
      h("div", {}, h("div", { class: "label", text: "Serves" }), h("select", { onchange: (e) => update(a.id, { serves: e.target.value }) }, options(Object.keys(list.serves).map((s) => [s, s]), a.serves)))),
    h("div", { class: "muted", style: "font-size:12px;margin-top:4px", text: list.serves[a.serves] || "" }),
    h("div", { class: "label", text: "Why it's here" }), why);
}

function renderAdd() {
  const id = h("input", { placeholder: "deskPlant", style: "width:100%" });
  const area = h("input", { list: "areas", placeholder: "desk", style: "width:100%" });
  const levels = Object.entries(list.levels).map(([k, l]) => [k, k + " " + l.name]);
  const level = h("select", {}, options(levels, "L3"));
  const serves = h("select", {}, options(Object.keys(list.serves).map((s) => [s, s]), "rae"));
  const why = h("textarea", { placeholder: "What it says about Rae, or what it does in play" });
  const error = h("div", { class: "error" });
  fill(page.side, 
    h("h2", { text: "Add an asset" }),
    h("div", { class: "muted", style: "font-size:13px;margin-top:4px", text: "It starts as an idea. The id can't change later." }),
    h("div", { class: "label", text: "Id (camelCase)" }), id,
    h("div", { class: "label", text: "Area" }), area, h("datalist", { id: "areas" }, list.areas.map((x) => h("option", { value: x }))),
    h("div", { class: "pair" }, h("div", {}, h("div", { class: "label", text: "Level" }), level), h("div", {}, h("div", { class: "label", text: "Serves" }), serves)),
    h("div", { class: "label", text: "Why it's here" }), why, error,
    h("div", { class: "actions" }, h("button", { onclick: () => { ui.adding = false; renderSide(); } }, "Cancel"),
      h("button", { class: "primary", onclick: async () => {
        try {
          const summary = await api("/api/assets", { id: id.value.trim(), area: area.value.trim(), level: level.value, serves: serves.value, why: why.value });
          if (!list.areas.includes(summary.area)) list.areas.push(summary.area);
          ui.adding = false;
          ui.tab = "idea";
          ui.selected = summary.id;
          replaceSummary(summary);
          toast("Added " + summary.id);
        } catch (e) {
          error.textContent = e.message;
        }
      } }, "Add")));
  id.focus();
}

function resolveMaterial(name) {
  const own = asset.data.doc?.materials?.[name];
  if (own) return own.color.startsWith("#") ? { color: own.color } : { texture: "/files/" + asset.data.id + "/" + own.color };
  const shared = asset.data.palette[name];
  return shared ? { color: shared.color, emit: shared.emit, emitStrength: shared.emitStrength } : { color: "#888888" };
}

async function showAsset(id, open = {}) {
  asset = { data: await api("/api/asset/" + id), variant: null, surface: "", menu: null };
  document.title = pretty(id);
  asset.variant = activeVariants()[0]?.id || null;
  page.bar = h("div", { class: "asset-bar" });
  page.viewer = h("div", { class: "viewer" }, h("div", { class: "hint", text: "Drag or middle-drag to orbit · Shift or right-drag to pan · Scroll to zoom · 1 3 7 front, side, top (Ctrl: opposite) · . to frame" }));
  page.problems = h("div", { class: "problems" });
  page.variants = h("div");
  page.surfaces = h("div");
  page.requests = h("div");
  app.replaceChildren(h("div", { class: "asset-page" }, page.bar,
    h("div", { class: "asset-body" },
      h("div", { class: "asset-left" }, page.viewer, page.problems, page.variants, page.surfaces),
      h("div", { class: "asset-right" }, page.requests))));
  if (asset.variant) viewer = new Viewer(page.viewer);
  else page.viewer.append(h("div", { class: "loading", text: "No model yet. Add a request for a new variant." }));
  renderBar();
  renderVariants();
  renderRequests();
  if (asset.variant) await loadVariant(activeVariants().some((v) => v.id === open.variant) ? open.variant : asset.variant);
  if (open.surface && liveSurfaces(asset.variant).some((s) => s.id === open.surface)) {
    asset.surface = open.surface;
    applySurface();
    renderSurfaces();
  }
  if (open.variant || open.edit) history.replaceState(null, "", "#/asset/" + id);
  const editing = (asset.data.entry.requests || []).find((r) => r.id === open.edit);
  if (editing && ["open", "failed"].includes(editing.status)) openRequest({}, editing);
}

function activeVariants() {
  return asset.data.variants.filter((v) => !v.retired);
}

function findVariant(id) {
  return asset.data.variants.find((v) => v.id === id);
}

function findSurface(variant, surface) {
  return findVariant(variant)?.surfaces.find((s) => s.id === surface);
}

function liveSurfaces(variant) {
  return (findVariant(variant)?.surfaces || []).filter((s) => !s.retired);
}

async function refresh() {
  const keep = asset.variant;
  asset.data = await api("/api/asset/" + asset.data.id);
  renderBar();
  renderRequests();
  if (!activeVariants().some((v) => v.id === keep)) {
    asset.variant = activeVariants()[0]?.id || null;
    if (asset.variant) await loadVariant(asset.variant);
  } else if (asset.surface && !liveSurfaces(keep).some((s) => s.id === asset.surface)) {
    asset.surface = "";
    applySurface();
  } else {
    applySurface();
  }
  renderVariants();
}

function renderBar() {
  const e = asset.data.entry;
  fill(page.bar,
    h("button", { class: "link", onclick: () => (location.hash = "#/") }, "← Assets"),
    h("button", { class: "link", onclick: () => (location.hash = "#/requests") }, "Requests"),
    h("h1", { text: pretty(asset.data.id) }),
    h("span", { class: "muted", style: "font-size:13px", text: e.why }),
    h("span", { class: "meta", text: [e.area, e.level, e.serves, STATUS_ONE[e.status]].join(" · ") }));
  fill(page.problems, ...asset.data.problems.map((p) => h("div", { text: p })));
}

async function loadVariant(id) {
  asset.variant = id;
  asset.surface = "";
  renderVariants();
  const variant = findVariant(id);
  if (!variant?.glb || !viewer) return;
  await viewer.load(variant.glb);
  applySurface();
}

function applySurface() {
  if (!viewer) return;
  const spec = asset.surface ? asset.data.doc?.colors?.[asset.surface] : null;
  viewer.setColours(spec || null, resolveMaterial);
}

function menu(key, items) {
  if (asset.menu !== key) return null;
  return h("div", { class: "menu", onclick: (e) => e.stopPropagation() },
    items.filter(Boolean).map((item) => h("button", { class: item.danger ? "danger" : "", onclick: () => { asset.menu = null; item.run(); } }, item.label)));
}

function moreButton(key) {
  return h("button", { class: "more", title: "More", onclick: (e) => { e.stopPropagation(); asset.menu = asset.menu === key ? null : key; renderVariants(); } }, "⋯");
}

function renderVariants() {
  const active = activeVariants();
  const retired = asset.data.variants.filter((v) => v.retired);
  const cards = active.map((v) => h("div", { class: "variant" + (asset.variant === v.id ? " on" : ""), onclick: () => { asset.menu = null; loadVariant(v.id); } },
    v.thumb ? h("img", { src: v.thumb, alt: "" }) : h("div", { class: "ph" }),
    h("div", { class: "foot" }, h("span", { text: v.id }), dot(v.state), moreButton("v:" + v.id)),
    menu("v:" + v.id, [
      v.state === "ok" ? { label: "Clear review", run: () => post("/variants/" + v.id, { action: "unreview" }) } : { label: "Mark ok", run: () => post("/variants/" + v.id, { action: "ok" }) },
      { label: "Redo…", run: () => openRequest({ kind: "redo", variant: v.id }) },
      { label: "New variant from " + v.id + "…", run: () => openRequest({ kind: "new", base: v.id }) },
      { label: "New surface…", run: () => openRequest({ kind: "surface", variant: v.id }) },
      { label: "Retire…", danger: true, run: () => openRetire(v) },
    ])));
  fill(page.variants,
    h("div", { class: "label", text: "Variants" }),
    active.length ? h("div", { class: "variants" }, cards) : h("div", { class: "muted", text: "No variants yet." }),
    retired.length ? h("details", {}, h("summary", { text: "Retired variants · " + retired.length }),
      h("div", { style: "display:flex;gap:8px;flex-wrap:wrap;margin-top:8px" }, retired.map((v) =>
        h("div", { class: "variant", style: "width:96px;cursor:default" }, v.thumb ? h("img", { src: v.thumb, alt: "" }) : h("div", { class: "ph" }),
          h("div", { class: "foot" }, h("span", { text: v.id }), h("button", { class: "small", style: "margin-left:auto", onclick: () => post("/variants/" + v.id, { action: "restore" }) }, "Restore")))))) : null);
  renderSurfaces();
}

function renderSurfaces() {
  const v = asset.variant;
  if (!v) {
    fill(page.surfaces);
    return;
  }
  const surfaces = findVariant(v)?.surfaces || [];
  const live = surfaces.filter((s) => !s.retired);
  const retired = surfaces.filter((s) => s.retired);
  const base = "/variants/" + v + "/surfaces/";
  const chips = live.map((s) => h("div", { class: "surface" + (asset.surface === s.id ? " on" : ""), onclick: () => { asset.menu = null; asset.surface = s.id; applySurface(); renderSurfaces(); } },
    h("span", { text: s.name }), dot(s.state), moreButton("s:" + s.id),
    menu("s:" + s.id, [
      s.state === "ok" ? { label: "Clear review", run: () => post(base + s.id, { action: "unreview" }) } : { label: "Mark ok", run: () => post(base + s.id, { action: "ok" }) },
      { label: "Redo…", run: () => openRequest({ kind: "redo", variant: v, surface: s.id }) },
      { label: "Copy to variant…", run: () => openRequest({ kind: "surface", variant: activeVariants().find((x) => x.id !== v)?.id || v, copy: v + "|" + s.id }) },
      { label: "Rename…", run: () => { const name = prompt("Surface name", s.name); if (name && name.trim()) post(base + s.id, { action: "rename", name }); } },
      { label: "Retire", danger: true, run: () => post(base + s.id, { action: "retire" }) },
    ])));
  fill(page.surfaces,
    h("div", { class: "label", text: "Surfaces of " + v }),
    h("div", { class: "surfaces" },
      h("div", { class: "surface" + (asset.surface ? "" : " on"), onclick: () => { asset.surface = ""; applySurface(); renderSurfaces(); } }, "Default"),
      chips),
    retired.length ? h("details", {}, h("summary", { text: "Retired surfaces · " + retired.length }),
      retired.map((s) => h("div", { class: "muted", style: "font-size:13px;margin-top:6px;display:flex;gap:8px;align-items:center" }, s.name,
        h("button", { class: "small", onclick: () => post(base + s.id, { action: "restore" }) }, "Restore")))) : null);
}

async function post(path, body) {
  asset.menu = null;
  try {
    await api("/api/asset/" + asset.data.id + path, body);
    await refresh();
    toast("Saved");
  } catch (error) {
    toast(error.message, true);
    renderVariants();
  }
}

function surfaceName(variant, surface) {
  return findSurface(variant, surface)?.name || surface;
}

function requestTitle(r) {
  if (r.kind === "new") return r.base ? "New variant from " + r.base : "New variant";
  if (r.kind === "redo") return r.redoSurface ? "Redo " + r.variant + " · " + surfaceName(r.variant, r.surface) : "Redo " + r.variant;
  if (r.copyFrom) return "Copy " + surfaceName(r.copyFrom.variant, r.copyFrom.surface) + " (" + r.copyFrom.variant + ") to " + r.variant;
  return "New surface for " + r.variant;
}

function requestResult(r) {
  const res = r.result;
  if (!res) return "";
  if (typeof res === "string") return "→ variant " + res;
  return "→ variant " + res.variant + (res.surface ? " · " + surfaceName(res.variant, res.surface) : "");
}

function imageRole(i) {
  return i.shape && i.surface ? "both" : i.shape ? "shape" : "surface";
}

function requestCard(r) {
  const id = asset.data.id;
  const images = (r.images || []).map((i) => (typeof i === "string" ? { file: i, shape: true, surface: true } : i));
  return h("div", { class: "request" },
    h("div", { class: "top" }, h("span", {}, requestTitle(r), h("span", { class: "muted mono", text: " " + r.id })), h("span", { class: "status " + r.status, text: r.status })),
    r.text ? h("div", { class: "text", text: r.text }) : null,
    images.length ? h("div", { class: "imgs" }, images.map((i) => h("a", { href: "/files/" + id + "/" + i.file, target: "_blank", class: "imgcell" },
      h("img", { src: "/files/" + id + "/" + i.file, alt: "" }), h("span", { class: "imgtag", text: imageRole(i) })))) : null,
    r.result ? h("div", { class: "muted", style: "font-size:12px;margin-top:4px", text: requestResult(r) }) : null,
    r.note ? h("div", { class: "muted", style: "font-size:12px;margin-top:4px", text: "Note: " + r.note }) : null,
    r.reason ? h("div", { class: "error", text: r.reason }) : null,
    ["open", "failed"].includes(r.status) ? h("div", { class: "actions" },
      h("button", { class: "small", onclick: () => openRequest({}, r) }, "Edit"),
      r.status === "failed" ? h("button", { class: "small", onclick: () => requestAction(r.id, "reopen") }, "Reopen") : null,
      h("button", { class: "small", onclick: () => requestAction(r.id, "cancel") }, "Cancel")) : null);
}

function renderRequests() {
  const all = [...(asset.data.entry.requests || [])].reverse();
  const live = all.filter((r) => ["open", "working", "failed"].includes(r.status));
  const past = all.filter((r) => !["open", "working", "failed"].includes(r.status));
  fill(page.requests,
    h("div", { class: "side-head" }, h("h2", { text: "Requests" }), h("button", { class: "small", onclick: () => openRequest({ kind: asset.variant ? "redo" : "new", variant: asset.variant }) }, "+ New request")),
    live.length ? live.map(requestCard) : h("div", { class: "muted", style: "font-size:13px;margin-top:10px", text: "Nothing waiting." }),
    past.length ? h("details", {}, h("summary", { text: "Done · " + past.length }), past.map(requestCard)) : null);
}

async function requestAction(id, action) {
  try {
    await api("/api/asset/" + asset.data.id + "/requests/" + id, { action });
    await refresh();
  } catch (error) {
    toast(error.message, true);
  }
}

function closeOverlay() {
  document.querySelector(".overlay")?.remove();
  closeOverlay.cleanup?.();
  closeOverlay.cleanup = null;
}

async function toPng(file) {
  const bitmap = await createImageBitmap(file);
  const canvas = document.createElement("canvas");
  canvas.width = bitmap.width;
  canvas.height = bitmap.height;
  canvas.getContext("2d").drawImage(bitmap, 0, 0);
  const blob = await new Promise((resolve) => canvas.toBlob(resolve, "image/png"));
  return new File([blob], (file.name || "pasted").replace(/\.[^.]*$/, "") + ".png", { type: "image/png" });
}

function pastedImages(event) {
  return [...(event.clipboardData?.items || [])].filter((i) => i.kind === "file" && i.type.startsWith("image/")).map((i) => i.getAsFile()).filter(Boolean);
}

function readImage(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => resolve({ name: file.name || "pasted.png", data: String(reader.result).split(",")[1], url: reader.result });
    reader.onerror = reject;
    reader.readAsDataURL(file);
  });
}

let imageKey = 0;

function openRequest(start, existing) {
  const active = activeVariants();
  const id = asset.data.id;
  const form = existing
    ? { kind: existing.kind, variant: existing.variant || active[0]?.id, surface: existing.redoSurface ? existing.surface : "", base: existing.base || "",
        copy: existing.copyFrom ? existing.copyFrom.variant + "|" + existing.copyFrom.surface : "" }
    : { kind: active.length ? start.kind || "new" : "new", variant: start.variant || active[0]?.id, surface: start.surface || "", base: start.base || "", copy: start.copy || "" };
  const images = new Map();
  const zones = { shape: new Set(), surface: new Set() };
  for (const i of existing?.images || []) {
    const item = typeof i === "string" ? { file: i, shape: true, surface: true } : i;
    const key = "k" + imageKey++;
    images.set(key, { file: item.file, url: "/files/" + id + "/" + item.file });
    if (item.shape) zones.shape.add(key);
    if (item.surface) zones.surface.add(key);
  }
  let activeZone = "shape";
  const allSurfaces = active.flatMap((v) => liveSurfaces(v.id).map((s) => [v.id + "|" + s.id, v.id + " · " + s.name]));
  const seg = h("div", { class: "seg" });
  const targetLine = h("div", { class: "line" });
  const textInput = h("textarea", { placeholder: "What to make or change. Where a surface image goes, if it matters." });
  textInput.value = existing?.text || "";
  const files = h("input", { type: "file", accept: "image/*", multiple: true, class: "hidden", onchange: (e) => addFiles(e.target.files, activeZone) });
  const zoneBox = { shape: h("div", { class: "drop" }), surface: h("div", { class: "drop" }) };
  const zoneRow = { shape: h("div", { class: "zone" }, h("div", { class: "label", text: "Shape" }), zoneBox.shape), surface: h("div", { class: "zone" }, h("div", { class: "label", text: "Surface" }), zoneBox.surface) };
  const error = h("div", { class: "error" });

  function visible() {
    const surfaceOnly = form.kind === "surface" || (form.kind === "redo" && form.surface);
    return { shape: !surfaceOnly, surface: !(form.kind === "surface" && form.copy) };
  }

  function renderTarget() {
    const lines = [];
    if (form.kind === "new" && active.length) {
      lines.push(h("span", { class: "muted", text: "Start from" }),
        h("select", { onchange: (e) => (form.base = e.target.value) }, options([["", "Nothing, start fresh"], ...active.map((v) => [v.id, "Variant " + v.id])], form.base)));
    }
    if (form.kind === "redo") {
      const targets = active.flatMap((v) => [[v.id, "Variant " + v.id], ...liveSurfaces(v.id).map((s) => [v.id + "|" + s.id, v.id + " · " + s.name])]);
      lines.push(h("span", { class: "muted", text: "Redo" }),
        h("select", { onchange: (e) => { [form.variant, form.surface = ""] = e.target.value.split("|"); renderForm(); } }, options(targets, form.surface ? form.variant + "|" + form.surface : form.variant)));
    }
    if (form.kind === "surface") {
      lines.push(h("span", { class: "muted", text: "For variant" }),
        h("select", { onchange: (e) => (form.variant = e.target.value) }, options(active.map((v) => [v.id, v.id]), form.variant)),
        h("span", { class: "muted", text: "Copy from" }),
        h("select", { onchange: (e) => { form.copy = e.target.value; renderForm(); } }, options([["", "Nothing"], ...allSurfaces], form.copy)));
    }
    fill(targetLine, lines);
  }

  function renderZone(name) {
    const show = visible()[name];
    zoneRow[name].classList.toggle("hidden", !show);
    zoneBox[name].classList.toggle("active", activeZone === name);
    fill(zoneBox[name],
      [...zones[name]].map((key) => h("div", { class: "img", draggable: "true", ondragstart: (e) => e.dataTransfer.setData("text/x-image-key", key) },
        h("img", { src: images.get(key).url, alt: "" }),
        h("button", { title: "Remove", onclick: (e) => { e.stopPropagation(); zones[name].delete(key); renderForm(); } }, "×"))),
      h("span", { class: "hint", text: zones[name].size ? "Add more" : "Drop, paste or click" }));
  }

  function renderForm() {
    const kinds = [["new", "New variant"], ["redo", "Redo"], ["surface", "New surface"]];
    fill(seg, kinds.map(([k, label]) => h("button", { class: form.kind === k ? "on" : "", disabled: k !== "new" && !active.length, onclick: () => { form.kind = k; renderForm(); } }, label)));
    renderTarget();
    const show = visible();
    if (!show[activeZone]) activeZone = show.shape ? "shape" : "surface";
    renderZone("shape");
    renderZone("surface");
  }

  async function addFiles(list, zone) {
    error.textContent = "";
    for (let file of list) {
      if (!/^image\/(jpeg|png|webp)$/.test(file.type)) {
        try {
          file = await toPng(file);
        } catch {
          error.textContent = "Couldn't read that image. Save it as png or jpg and drop it in.";
          continue;
        }
      }
      const key = "k" + imageKey++;
      images.set(key, await readImage(file));
      zones[zone].add(key);
    }
    renderForm();
  }

  for (const name of ["shape", "surface"]) {
    const box = zoneBox[name];
    box.addEventListener("click", (e) => {
      activeZone = name;
      renderForm();
      if (e.target === box || e.target.classList.contains("hint")) files.click();
    });
    box.addEventListener("dragover", (e) => { e.preventDefault(); box.classList.add("over"); });
    box.addEventListener("dragleave", () => box.classList.remove("over"));
    box.addEventListener("drop", (e) => {
      e.preventDefault();
      box.classList.remove("over");
      const key = e.dataTransfer.getData("text/x-image-key");
      if (key && images.has(key)) {
        zones[name].add(key);
        renderForm();
      } else {
        addFiles(e.dataTransfer.files, name);
      }
    });
  }

  const onPaste = (e) => {
    const pasted = pastedImages(e);
    if (!pasted.length) return;
    e.preventDefault();
    addFiles(pasted, visible()[activeZone] ? activeZone : "surface");
  };
  document.addEventListener("paste", onPaste);
  closeOverlay.cleanup = () => document.removeEventListener("paste", onPaste);

  const submit = async () => {
    const show = visible();
    const list = [];
    for (const [key, img] of images) {
      const shape = show.shape && zones.shape.has(key);
      const surface = show.surface && zones.surface.has(key);
      if (!shape && !surface) continue;
      list.push(img.file ? { file: img.file, shape, surface } : { name: img.name, data: img.data, shape, surface });
    }
    if (!textInput.value.trim() && !list.length && !(form.kind === "surface" && form.copy)) {
      error.textContent = "Write a description or add an image.";
      return;
    }
    const body = { kind: form.kind, text: textInput.value, images: list };
    if (form.kind === "new" && form.base) body.base = form.base;
    if (form.kind !== "new") body.variant = form.variant;
    if (form.kind === "redo" && form.surface) body.surface = form.surface;
    if (form.kind === "surface" && form.copy) {
      const [variant, surface] = form.copy.split("|");
      body.copyFrom = { variant, surface };
    }
    try {
      if (existing) await api("/api/asset/" + id + "/requests/" + existing.id, { action: "edit", ...body });
      else await api("/api/asset/" + id + "/requests", body);
      closeOverlay();
      await refresh();
      toast(existing ? "Request updated" : "Request added");
    } catch (e) {
      error.textContent = e.message;
    }
  };

  const dialog = h("div", { class: "dialog" },
    h("div", { class: "head" }, h("h2", { text: (existing ? "Edit request " + existing.id : "New request") + " · " + pretty(id) }), h("button", { class: "link", onclick: closeOverlay }, "✕")),
    h("div", { class: "label", text: "What" }), seg, targetLine,
    h("div", { class: "label", text: "Description" }), textInput,
    zoneRow.shape, zoneRow.surface, files,
    h("div", { class: "muted", style: "font-size:12px;margin-top:6px", text: "Drag a picture from one box to the other to use it for both." }), error,
    h("div", { class: "actions" }, h("button", { onclick: closeOverlay }, "Cancel"), h("button", { class: "primary", onclick: submit }, existing ? "Save" : "Add request")));
  document.body.append(h("div", { class: "overlay", onclick: (e) => { if (e.target.classList.contains("overlay")) closeOverlay(); } }, dialog));
  renderForm();
  textInput.focus();
}

function openRetire(variant) {
  const others = activeVariants().filter((v) => v.id !== variant.id);
  const replace = h("select", {}, options(others.map((v) => [v.id, v.id]), others[0]?.id));
  const error = h("div", { class: "error" });
  const used = variant.looks.length > 0;
  const dialog = h("div", { class: "dialog" },
    h("div", { class: "head" }, h("h2", { text: "Retire variant " + variant.id }), h("button", { class: "link", onclick: closeOverlay }, "✕")),
    h("p", { class: "muted", style: "font-size:13px", text: "It stays in the file and can be restored. It stops showing as an option here." }),
    used ? [h("div", { class: "label", text: "Used by looks: " + variant.looks.join(", ") }),
      others.length ? h("div", {}, h("span", { class: "muted", style: "font-size:13px", text: "They switch to " }), replace) : h("div", { class: "error", text: "No other variant to switch to. Make one first." })] : null,
    error,
    h("div", { class: "actions" }, h("button", { onclick: closeOverlay }, "Cancel"),
      h("button", { class: "primary", disabled: used && !others.length, onclick: async () => {
        try {
          await api("/api/asset/" + asset.data.id + "/variants/" + variant.id, { action: "retire", replace: used ? replace.value : "" });
          closeOverlay();
          await refresh();
          toast("Retired " + variant.id);
        } catch (e) {
          error.textContent = e.message;
        }
      } }, "Retire")));
  document.body.append(h("div", { class: "overlay", onclick: (e) => { if (e.target.classList.contains("overlay")) closeOverlay(); } }, dialog));
}

document.addEventListener("keydown", (e) => {
  if (e.key !== "Escape") return;
  if (document.querySelector(".overlay")) closeOverlay();
  else if (asset?.menu) { asset.menu = null; renderVariants(); }
});
document.addEventListener("click", () => {
  if (asset?.menu) { asset.menu = null; renderVariants(); }
});
function switcher(current) {
  return h("div", { class: "switch" },
    h("button", { class: current === "assets" ? "on" : "", onclick: () => (location.hash = "#/") }, "Assets"),
    h("button", { class: current === "requests" ? "on" : "", onclick: () => (location.hash = "#/requests") }, "Requests"));
}

const rui = { search: "", area: "", level: "", kind: "", showDone: false };
const KINDS = [["", "All kinds"], ["new", "New variant"], ["redo", "Redo"], ["surface", "New surface"]];
const GROUPS = [["open", "Open"], ["working", "Working"], ["failed", "Failed"], ["done", "Done"]];
let requestData = null;

async function showRequests() {
  requestData = await api("/api/requests");
  document.title = "Requests";
  const waiting = requestData.requests.filter((r) => r.status === "open").length;
  const levels = Object.entries(requestData.levels).map(([id, l]) => [id, id + " " + l.name]);
  page.requestRows = h("div", { class: "rows" });
  const filters = h("div", { class: "filters" },
    h("input", { type: "search", placeholder: "Search", value: rui.search, oninput: (e) => { rui.search = e.target.value.trim().toLowerCase(); renderRequestRows(); } }),
    h("select", { onchange: (e) => { rui.area = e.target.value; renderRequestRows(); } }, options([["", "All areas"], ...requestData.areas.sort((a, b) => areaRank(a) - areaRank(b)).map((a) => [a, a])], rui.area)),
    h("select", { onchange: (e) => { rui.level = e.target.value; renderRequestRows(); } }, options([["", "All levels"], ...levels], rui.level)),
    h("select", { onchange: (e) => { rui.kind = e.target.value; renderRequestRows(); } }, options(KINDS, rui.kind)));
  const head = h("div", { class: "list-head" }, switcher("requests"),
    h("span", { class: "muted", style: "margin-left:auto;font-size:13px", text: waiting ? waiting + " waiting for /handle-requests" : "Nothing waiting" }));
  app.replaceChildren(h("div", { class: "requests-page" }, head, filters, page.requestRows));
  renderRequestRows();
}

function groupOf(r) {
  return r.status === "cancelled" ? "done" : r.status;
}

function renderRequestRows() {
  const shown = requestData.requests.filter((r) => {
    if (rui.area && r.area !== rui.area) return false;
    if (rui.level && r.level !== rui.level) return false;
    if (rui.kind && r.kind !== rui.kind) return false;
    return !rui.search || [r.asset, pretty(r.asset), r.title, r.text, r.id].join(" ").toLowerCase().includes(rui.search);
  });
  const nodes = [];
  for (const [group, label] of GROUPS) {
    let items = shown.filter((r) => groupOf(r) === group);
    if (group === "done") {
      items.sort((a, b) => Number(Boolean(b.result?.needsReview)) - Number(Boolean(a.result?.needsReview)) || (b.finished || b.created).localeCompare(a.finished || a.created));
    } else {
      items.sort((a, b) => a.created.localeCompare(b.created) || a.asset.localeCompare(b.asset));
    }
    const toggle = group === "done" && items.length ? h("button", { class: "link", style: "font-size:12px", onclick: () => { rui.showDone = !rui.showDone; renderRequestRows(); } }, rui.showDone ? "hide" : "show") : null;
    nodes.push(h("div", { class: "group", style: "display:flex;justify-content:space-between" }, h("span", { text: label + " · " + items.length }), toggle));
    if (group === "done" && !rui.showDone) continue;
    nodes.push(...items.map(requestRow));
  }
  fill(page.requestRows, nodes);
}

function requestRow(r) {
  const goAsset = () => (location.hash = "#/asset/" + r.asset);
  const detail = r.status === "failed" ? r.reason : r.result ? "→ " + r.result.label + (r.result.needsReview ? " · waiting for your review" : "") : r.text;
  const buttons = [];
  if (["open", "failed"].includes(r.status)) buttons.push(h("button", { class: "small", onclick: () => (location.hash = "#/asset/" + r.asset + "/edit/" + r.id) }, "Edit"));
  if (r.status === "failed") buttons.push(h("button", { class: "small", onclick: () => listRequestAction(r, "reopen") }, "Reopen"));
  if (r.status === "open") buttons.push(h("button", { class: "small", onclick: () => listRequestAction(r, "cancel") }, "Cancel"));
  if (r.result?.needsReview) buttons.push(h("button", { class: "small", onclick: () => (location.hash = "#/asset/" + r.asset + "/" + r.result.variant + (r.result.surface ? "/" + r.result.surface : "")) }, "Review →"));
  return h("div", { class: "rrow" + (r.status === "done" || r.status === "cancelled" ? " past" : "") },
    r.thumb ? h("img", { class: "thumb", src: r.thumb, alt: "", loading: "lazy" }) : h("div", { class: "thumb" }),
    h("div", { style: "min-width:0" },
      h("div", {}, h("button", { class: "link name", onclick: goAsset }, pretty(r.asset)), h("span", { class: "muted mono", text: " " + r.id })),
      h("div", { class: "why", text: r.title + (detail ? " · " + detail : "") })),
    h("div", { class: "rimgs" }, r.images.slice(0, 4).map((i) => h("img", { src: "/files/" + r.asset + "/" + i.file, alt: "", title: imageRole(i), loading: "lazy" }))),
    h("span", { class: "pill", text: r.level }),
    h("span", { class: "status " + r.status, text: r.status }),
    h("div", { class: "actions", style: "margin-top:0" }, buttons));
}

async function listRequestAction(r, action) {
  try {
    await api("/api/asset/" + r.asset + "/requests/" + r.id, { action });
    await showRequests();
    toast("Saved");
  } catch (error) {
    toast(error.message, true);
  }
}

window.addEventListener("hashchange", route);
route();
