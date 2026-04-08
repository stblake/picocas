# PicoCAS Notebook

A Mathematica-style notebook desktop app for [PicoCAS](https://github.com/stblake/picocas), built with [Tauri v2](https://tauri.app) (Rust backend + React frontend).

## Features

- **Code cells** — write and evaluate PicoCAS expressions with Shift+Enter
- **Text cells** — editable Markdown for titles, headings, and notes (`# H1`, `**bold**`, `*italic*`)
- **Plotting** — `Plot[expr, {x, min, max}]` renders an interactive line chart
- **Save / Open** — save notebooks as `.pico` files and reopen them later
- **Hello World notebook** — pre-loaded example cells on first launch
- Syntax highlighting via CodeMirror 6
- Math rendering via KaTeX

## Requirements

- [Node.js](https://nodejs.org) ≥ 18
- [Rust](https://rustup.rs) (stable toolchain)
- [Tauri CLI v2](https://tauri.app/start/prerequisites/)
- Python 3 (for the PTY wrapper)
- A built `picocas` binary (see below)

## Setup

### 1. Build picocas

From the repo root:

```sh
make
```

This produces the `picocas` binary in the repo root.

### 2. Bundle resources

The app expects `picocas` and `pty_wrap.py` inside `notebook-tauri/src-tauri/resources/`:

```sh
mkdir -p notebook-tauri/src-tauri/resources
cp picocas notebook-tauri/src-tauri/resources/
cp notebook-tauri/pty_wrap.py notebook-tauri/src-tauri/resources/
```

### 3. Install JS dependencies

```sh
cd notebook-tauri
npm install
```

## Development

```sh
cd notebook-tauri
npm run tauri dev
```

The Vite dev server starts on port 1420 and the desktop window opens automatically.

## Build (distributable)

```sh
cd notebook-tauri
npm run tauri build
```

The `.dmg` (macOS), `.msi` (Windows), or `.AppImage` (Linux) will appear in `src-tauri/target/release/bundle/`.

## Usage

| Action | How |
|--------|-----|
| Evaluate a cell | Click the ▶ button or press **Shift+Enter** |
| Add a code cell | Click **+ Code** at the bottom |
| Add a text cell | Click **+ Text** at the bottom |
| Edit a text cell | Click on it — it becomes a textarea |
| Move a cell | ▲ / ▼ buttons in the cell toolbar (hover to reveal) |
| Delete a cell | ✕ button in the cell toolbar |
| New notebook | **New** button in the header |
| Open notebook | **Open** button → pick a `.pico` file |
| Save notebook | **Save** (overwrites current file) or **Save As** |

## Notebook format

Notebooks are saved as JSON with a `.pico` extension:

```json
{
  "version": 1,
  "cells": [
    { "type": "text",  "input": "# My Notebook" },
    { "type": "code",  "input": "2 + 3" }
  ]
}
```

## Architecture

```
notebook-tauri/
├── src/                    React + TypeScript frontend
│   ├── hooks/useNotebook.ts  Notebook state, evaluate, save/load
│   ├── components/
│   │   ├── Cell.tsx          Routes to TextCell or code cell
│   │   ├── TextCell.tsx      Editable markdown cell
│   │   ├── CellInput.tsx     CodeMirror 6 editor
│   │   ├── CellOutput.tsx    KaTeX / text output
│   │   └── PlotOutput.tsx    Recharts line chart
│   └── lib/
│       └── picocasHighlight.ts  CodeMirror syntax highlighter
└── src-tauri/              Rust + Tauri backend
    └── src/
        ├── lib.rs            App setup, plugin registration
        ├── commands.rs       Tauri IPC commands
        └── picocas.rs        PicocasSession process manager
```

The Rust backend spawns `picocas` via a Python PTY wrapper (`pty_wrap.py`) so that `picocas` sees a TTY and uses line-buffered output. A background `std::thread` handles blocking I/O; Tauri async commands bridge via `tokio::sync::oneshot` channels.
