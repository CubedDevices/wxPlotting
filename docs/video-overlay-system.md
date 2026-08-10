# Video Overlay System — Design Document

**Status:** Design (reference for implementation)  
**Copyright:** Kenneth S. Davis, 2026  
**Related codebases:** `wxPlotting` library; host wxWidgets video application  
**Last updated:** 2026-08-01  

This document is the continuity reference for building a **video overlay / HUD system**: data-bound visual elements arranged over a video feed, interactively edited, and persisted. Implementers should update the **Phase status** section as work completes.

---

## 1. Goals

### 1.1 Primary goals

- Overlay **sensor and/or REST API data** on a live (or recorded) **video** surface.
- Allow the operator to **place, arrange, and resize** overlay regions.
- Support **styling**: fonts, foreground/background colours, opacity (and related attributes).
- **Bind** elements to live data sources (sensors, REST, etc.).
- **Save**, **load**, and **re-edit** complete layouts.
- Evolve to include **plots**, **PNG images**, and **watermarks**.
- Use normal desktop UX: **dialogs**, **right-click context menus**, selection handles, etc.

### 1.2 Non-goals (initial)

- Cross-process sharing of `wxWindow` instances (not supported by wxWidgets).
- Replacing the full interactive `wxPlotting` analysis UI (that remains a separate control).
- Building every HUD readout as a full multi-axis plot control.

### 1.3 Relationship to `wxPlotting`

| Use | Approach |
|-----|----------|
| Interactive multi-axis analysis | Existing `wxPlotting` `wxScrolledWindow` control |
| Small strip chart / sparkline on video | Future overlay **element type** that renders via off-screen bitmap (see §7) |
| HUD text, values, panels, logos | Overlay **document + compositor** (this design) |

**Principle:** Overlay composition is a **document + renderer** problem. `wxPlotting` is one possible **element renderer**, not the layout host for every HUD item.

---

## 2. Architecture overview

```
┌────────────────────────────────────────────────────────────┐
│  OverlayDocument (save / load / version)                   │
│    canvas policy · elements[] · source descriptors         │
└───────────────────────────┬────────────────────────────────┘
                            │
          ┌─────────────────┼─────────────────┐
          ▼                 ▼                 ▼
   DataBus / Bindings   Layout Editor UI   OverlayRenderer
   (REST, sensors)      (select, move,     (document + data
          │              resize, RMB,        → RGBA bitmap)
          │              property dialogs)
          │                 │                 │
          └─────────────────┴────────┬────────┘
                                     ▼
                            Video compositor
                            (video frame + overlay)
                                     ▼
                                  Present
```

### 2.1 Three layers

| Layer | Responsibility | Source of truth |
|-------|----------------|-----------------|
| **Document** | What exists: geometry, styles, bindings, z-order | Yes — serialize this |
| **Data** | Live values from sensors / REST | DataBus |
| **View** | Edit chrome + composite onto video | No — rebuild from document |

Do **not** treat a free-floating tree of `wxWindow` children as the long-term source of truth. Widgets implement **editing** and **presentation**; the **document** is what is saved.

### 2.2 Process and threading

- All of this runs in **one wxWidgets process** (one `wxApp`).
- **Share** `OverlayDocument`, `DataBus`, and rendered bitmaps in-process.
- **Do not** share `wxWindow` / HWND / GTK widgets across processes.
- Configuration and document mutation: **UI thread**.
- Acquisition / REST: **worker threads**, marshaling updates to UI (or a defined render path).
- Series/point-style buffers (if plots are added): follow existing `wxPlotting` rules (point data may update off UI thread; schedule plot rebuild on UI thread).

---

## 3. Document model

### 3.1 Canvas

| Field | Purpose |
|-------|---------|
| Logical width / height | Design resolution (e.g. 1920×1080), or “match video” |
| Fit mode | `letterbox` / `stretch` / `crop` when video aspect ≠ canvas |
| Optional global defaults | Default font, colours, opacity |

**Recommendation:** Store element geometry in **normalized coordinates** (0–1 relative to canvas). Convert to pixels at render time from the current video or canvas pixel size. This survives resolution changes better than absolute pixels alone.

### 3.2 Element — common fields

Every overlay item includes at least:

| Field | Description |
|-------|-------------|
| `id` | Stable unique id (UUID or slug) |
| `type` | Discriminator: `label`, `value`, `panel`, `image`, `plot`, … |
| `name` | Human-readable name for lists / status |
| `rect` | `{ x, y, w, h }` (normalized 0–1 preferred) |
| `z` | Z-order (higher drawn later / on top) |
| `visible` | Show/hide |
| `locked` | Prevent move/resize in editor |
| `style` | Colours, font, opacity, padding, border (type-specific extras allowed) |
| `binding` | Optional `DataRef` (see §4); omit for static labels/images |

### 3.3 Element types

| Type | Phase | Description |
|------|-------|-------------|
| **panel** | 1 | Background plate (colour, opacity, optional border) |
| **label** | 1 | Static or lightly templated text |
| **value** | 1 | Bound numeric/text readout + format string |
| **image** | 2 | PNG/logo/watermark path + opacity |
| **plot** | 3 | Mini plot via off-screen render (wxPlotting scene) |
| **group** | 3+ | Optional grouping for multi-select / align |

### 3.4 Example JSON schema (illustrative)

```json
{
  "version": 1,
  "canvas": {
    "width": 1920,
    "height": 1080,
    "fit": "letterbox"
  },
  "sources": [
    {
      "id": "plant-rest",
      "type": "rest",
      "url": "https://example.invalid/api/snapshot",
      "intervalMs": 500
    }
  ],
  "elements": [
    {
      "id": "e1",
      "type": "value",
      "name": "Oil temp",
      "rect": { "x": 0.02, "y": 0.05, "w": 0.22, "h": 0.08 },
      "z": 10,
      "visible": true,
      "locked": false,
      "style": {
        "fg": "#FFFFFFFF",
        "bg": "#000000AA",
        "font": { "face": "Consolas", "size": 18, "bold": true },
        "format": "%.1f °C",
        "padding": 6
      },
      "binding": {
        "source": "plant-rest",
        "path": "sensors.oilTemp"
      }
    },
    {
      "id": "wm",
      "type": "image",
      "name": "Logo",
      "rect": { "x": 0.85, "y": 0.90, "w": 0.12, "h": 0.08 },
      "z": 5,
      "style": { "opacity": 0.6 },
      "image": "branding/logo.png"
    }
  ]
}
```

### 3.5 Persistence rules

| Topic | Guidance |
|-------|----------|
| Format | JSON (human-diffable); version field required |
| Images | Prefer paths relative to layout/project root; optional base64 pack later |
| Secrets | API keys / tokens in a **connection profile**, not in layout JSON when avoidable |
| Migration | Bump `version`; add load-time migrators when fields change |
| Round-trip | Load → edit → save must preserve unknown fields if possible (forward compatibility) |

---

## 4. Data attachment (sensors / REST)

### 4.1 DataBus

Central service used by elements and the editor:

```
DataRef  = { sourceId, pathOrChannel, optional transform id }
Sample   = { value, timestamp, quality }
DataBus.Get(ref) → Sample
DataBus.Subscribe(ref, callback)  // marshal to UI thread as needed
```

### 4.2 Source plugins

| Source type | Role |
|-------------|------|
| `rest` | Poll URL; extract field via path (JSON pointer / dotted path) |
| `sensor` | Vendor SDK / bus / OPC / MQTT adapter |
| `sim` | Generators for demos and tests |

**Elements never own HTTP clients.** They store a **DataRef**. Sources are configured in the document’s `sources` array and/or a separate connection registry.

### 4.3 Binding modes

| Mode | Behavior |
|------|----------|
| Live | Update on poll/change |
| Snapshot | Freeze value when element created or on explicit capture |
| Template (later) | `"Temp: {value} °C"` / multi-field expressions |

### 4.4 Quality and alarms (later)

Optional: map `quality` or thresholds to style overrides (e.g. red foreground when out of range) without changing geometry.

---

## 5. Editor UX

### 5.1 Modes

| Mode | Behavior |
|------|----------|
| **Runtime** | Video + overlays; minimal chrome |
| **Edit** | Same composite + selection, handles, context menus, property dialogs |

Toggle via toolbar, menu, or hotkey. Prefer **one renderer** for both modes so edit view matches export/runtime (WYSIWYG).

### 5.2 Interaction map

| Action | UX |
|--------|-----|
| Add element | Right-click video → **Add** → Label / Value / Panel / Image… |
| Select | Left-click element (hit-test in document space) |
| Move | Drag body |
| Resize | Drag corner/edge handles (optional aspect lock) |
| Properties | Double-click or context **Properties…** → dialog |
| Z-order | Context: Bring to front / Send to back |
| Delete / Duplicate | Context menu / Delete key |
| Align | Phase 3+: multi-select |

### 5.3 Property dialog (typical fields)

- Name, visible, locked  
- Rect (or only via handles)  
- Foreground / background / opacity  
- Font face, size, bold/italic  
- Format string (value elements)  
- Binding: source + path browser  
- Image path (image elements)  
- Type-specific options  

### 5.4 Hit-testing

Implement hit-test against **document geometry** (normalized rects → pixel rects), not against dozens of permanent child controls. This mirrors the plotting control’s model-based hit-test approach.

Optional hybrid: temporary edit-only child widgets for text entry; still serialize from the document.

---

## 6. Video composition pipeline

### 6.1 Per-frame / per-tick flow

```
1. Acquire video frame → bitmap / texture
2. If overlay dirty OR bound data changed:
     OverlayRenderer.Paint(document, dataBus, pixelSize, dpi)
       → overlayBitmap (RGBA preferred)
3. Composite: video ⊕ overlay (alpha blend or premultiplied alpha)
4. Present
```

### 6.2 Refresh rates

| Stream | Suggested rate |
|--------|----------------|
| Video | Full frame rate (e.g. 30/60 Hz) |
| Overlay | Dirty only, or capped (e.g. 5–30 Hz) when data ticks |

Decouple overlay rebuild from video frame rate for performance.

### 6.3 Transparency

| Strategy | When |
|----------|------|
| **RGBA alpha** | Preferred for text/panels/images |
| **Chroma key** | Fallback if pipeline is RGB-only |
| **Layered OS window** | Alternative for true window-over-video; platform-specific |

Clear overlay buffer to transparent (or key colour) before drawing elements.

### 6.4 Coordinate conversion

```
pixelRect = NormalizeRect(element.rect) × videoOrCanvasPixelSize
```

Apply canvas fit mode when video aspect ratio differs from design canvas.

---

## 7. Integration with `wxPlotting` (future)

Current `wxPlotting` is a realized `wxScrolledWindow` with an off-screen buffer. For video:

### 7.1 Preferred plot path (Phase 3)

1. Maintain a **plot scene** (axes, series, styles) — data model, not necessarily a visible window.
2. **`RenderToBitmap(size, scale, clearColour)`** (or equivalent API — *to be designed when Phase 3 starts*).
3. Treat the result as the plot element’s content; composite like an image.

### 7.2 Avoid for pure overlay

- Unrealized `wxScrolledWindow` as a render engine.
- Assuming MemoryDC “transparent erase” works identically on all platforms without testing.
- One interactive plot window per HUD value.

### 7.3 Interactive plots

If the user must pan/zoom a plot **on** the video, either:

- temporarily host a real `wxPlotting` child in edit/interaction mode, or  
- route mouse into a hit region and update the scene then re-render to bitmap.

Document the choice when Phase 3 is scheduled.

---

## 8. Component map (same process)

| Component | Responsibility |
|-----------|----------------|
| `VideoView` | Video decode/display surface |
| `OverlayDocument` | Serializable model |
| `OverlayRenderer` | Document + DataBus → RGBA |
| `OverlayEditor` | Edit mode, selection, RMB, dialogs |
| `DataBus` | REST / sensor plugins |
| `ElementFactory` | Create defaults by type |
| `PropertyDialogs` | Font, colours, binding picker |
| `LayoutIO` | Load/save JSON + version migration |

### 8.1 Sharing

| Share | OK? |
|-------|-----|
| Document + DataBus | Yes |
| Rendered overlay bitmap | Yes |
| Plot scene / series points | Yes (with threading rules) |
| `wxWindow*` across processes | No |

---

## 9. Phases

Update the **Status** column as work lands. Reference this table in commits/PRs: e.g. `Phase 1 — value binding`.

### Phase 0 — Prerequisites / spikes

| Item | Status | Notes |
|------|--------|-------|
| Confirm video pipeline blit API (bitmap vs GL texture) | Not started | Host app dependent |
| Spike: RGBA composite over one video frame | Not started | |
| Decide normalized vs pixel rects (locked: **normalized**) | **Decided** | §3.1 |
| Decide secrets storage approach | Not started | Profiles vs env |

### Phase 1 — HUD skeleton

**Outcome:** Editable HUD with live or mock data, save/load, composite over video (or still frame).

| Deliverable | Status | Notes |
|-------------|--------|-------|
| `OverlayDocument` + JSON load/save (`version: 1`) | Not started | |
| Element types: `panel`, `label`, `value` | Not started | |
| `OverlayRenderer` → RGBA | Not started | |
| Edit mode: select, move, resize | Not started | |
| Context menu: Add / Delete / Properties / Z-order | Not started | |
| Property dialog: colours, font, format | Not started | |
| `DataBus` + mock or REST source | Not started | |
| Binding picker for `value` elements | Not started | |
| Composite into host video view | Not started | |
| Basic unit/smoke tests for document IO | Not started | Optional |

**Exit criteria:** Create a layout, bind a value, save, restart app, load, see same layout with live updates.

### Phase 2 — Production polish

| Deliverable | Status | Notes |
|-------------|--------|-------|
| Element type: `image` (PNG watermark/logo) | Not started | |
| Opacity, lock, improved z-order UX | Not started | |
| Source manager UI (REST endpoints, intervals) | Not started | |
| Overlay refresh throttling | Not started | |
| Relative image paths + project root | Not started | |
| Error/quality display for bad bindings | Not started | |

**Exit criteria:** Field-usable layout with branding + multiple live fields; stable under long run.

### Phase 3 — Plots and rich layout

| Deliverable | Status | Notes |
|-------------|--------|-------|
| `wxPlotting` render-to-bitmap (or scene API) | Not started | Coordinate with library work |
| Element type: `plot` | Not started | |
| Multi-select / align / distribute | Not started | |
| Optional `group` type | Not started | |
| Alarm colouring from thresholds | Not started | |
| Layout templates | Not started | |

**Exit criteria:** At least one live mini-plot on video; layout tools adequate for multi-element HUDs.

### Phase 4 — Hardening

| Deliverable | Status | Notes |
|-------------|--------|-------|
| Schema migration framework | Not started | |
| Packaged layout bundles | Not started | |
| Performance profiling at full resolution | Not started | |
| Accessibility / large fonts | Not started | As needed |
| Documentation for operators | Not started | |

---

## 10. Operator flow (target experience)

1. Open application with video feed.  
2. Enable **Edit layout**.  
3. Right-click video → **Add → Value**.  
4. Drag and resize the box.  
5. Double-click → set binding, format, font, fg/bg.  
6. Add logo → **Add → Image**.  
7. **Save layout…** → `site-a-hud.json`.  
8. Later: **Load layout…**, adjust, save again.  
9. Exit edit mode → clean HUD over live video.

---

## 11. Decisions log

Record irreversible or cross-cutting decisions here so phases stay aligned.

| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-08-01 | Document + renderer + DataBus architecture | Avoid widget-tree-as-source-of-truth |
| 2026-08-01 | Normalized rects (0–1) preferred | Resolution independence |
| 2026-08-01 | Plots as Phase 3 bitmap elements | HUD first; reuse plot buffer design later |
| 2026-08-01 | No cross-process widget sharing | wxWidgets process model |
| | | |

---

## 12. Open questions

Track answers in the decisions log when resolved.

1. Host video API: software blit, DirectShow/Media Foundation, OpenGL/Vulkan, or other?  
2. Edit and runtime: always same process/module, or plugin DLL?  
3. Multi-layout switching while video runs (presets)?  
4. Localization of format strings and UI?  
5. Maximum element count / performance budget at 4K60?  

---

## 13. How to use this document

- **Before starting a phase:** Read §9 for that phase and §11–12.  
- **During implementation:** Update **Status** cells; add notes or PR links.  
- **When inventing new element types or JSON fields:** Update §3 and bump schema `version` if breaking.  
- **When `wxPlotting` gains headless/bitmap APIs:** Update §7 and Phase 3 rows.  
- **Do not** implement later-phase features in a way that blocks Phase 1 exit criteria.

---

## 14. Related documents / code

| Resource | Location |
|----------|----------|
| This design | `docs/video-overlay-system.md` |
| Plotting library | `wxPlotting.h` / `wxPlotting.cpp`, `series.h`, `axis.h` |
| Interactive demo | `wxPlottingApp.*` |
| Real-time series demo | `wxPlottingRealTimeDemo.*` |
| License | `COPYING` (GPLv2) |

---

*End of design document.*
