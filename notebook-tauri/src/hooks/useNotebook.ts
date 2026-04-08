import { useState, useCallback, useRef } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { open, save } from '@tauri-apps/plugin-dialog';
import { v4 as uuidv4 } from 'uuid';

export type Point = [number, number];
export type CellType = 'code' | 'text';

export interface PlotData {
  points: Point[];
  varName: string;
  expr: string;
  xMin: number;
  xMax: number;
}

export interface Cell {
  id: string;
  cellNumber: number;
  type: CellType;
  input: string;
  output: string | null;
  outN: number | null;
  plotData: PlotData | null;
  status: 'idle' | 'evaluating' | 'done' | 'error';
}

export interface NotebookContext {
  cells: Cell[];
  currentPath: string | null;
  evaluate: (cellId: string) => void;
  addCell: (type?: CellType) => void;
  deleteCell: (id: string) => void;
  updateInput: (id: string, text: string) => void;
  moveCell: (id: string, direction: 'up' | 'down') => void;
  newNotebook: () => void;
  openNotebook: () => Promise<void>;
  saveNotebook: () => Promise<void>;
  saveNotebookAs: () => Promise<void>;
}

interface EvalResult {
  output: string;
  out_n: number;
}

interface RawPlotData {
  points: Point[];
  var_name: string;
  expr: string;
  x_min: number;
  x_max: number;
}

interface NotebookFile {
  version: number;
  cells: { type: CellType; input: string }[];
}

const HELLO_WORLD: { type: CellType; input: string }[] = [
  { type: 'text', input: '# PicoCAS Notebook\nA symbolic math notebook. Click a cell and press **Shift+Enter** to evaluate.' },
  { type: 'text', input: '## Basic Arithmetic' },
  { type: 'code', input: '2 + 3' },
  { type: 'code', input: '10!' },
  { type: 'text', input: '## Plotting' },
  { type: 'code', input: 'Plot[x^2, {x, -2, 2}]' },
];

function makeCell(num: number, type: CellType = 'code', input = ''): Cell {
  return {
    id: uuidv4(),
    cellNumber: num,
    type,
    input,
    output: null,
    outN: null,
    plotData: null,
    status: 'idle',
  };
}

function helloWorldCells(): Cell[] {
  let n = 1;
  return HELLO_WORLD.map(({ type, input }) => makeCell(n++, type, input));
}

export function useNotebook(): NotebookContext {
  const [cells, setCells] = useState<Cell[]>(helloWorldCells);
  const [currentPath, setCurrentPath] = useState<string | null>(null);
  const nextNumber = useRef(HELLO_WORLD.length + 1);
  const cellsRef = useRef(cells);
  cellsRef.current = cells;

  const evaluate = useCallback((cellId: string) => {
    const cell = cellsRef.current.find(c => c.id === cellId);
    if (!cell || cell.type !== 'code') return;
    const input = cell.input.trim();
    if (!input) return;

    setCells(prev =>
      prev.map(c =>
        c.id === cellId ? { ...c, status: 'evaluating' as const, output: null, outN: null, plotData: null } : c
      )
    );

    const onError = (err: unknown) =>
      setCells(prev =>
        prev.map(c =>
          c.id === cellId ? { ...c, status: 'error' as const, output: String(err) } : c
        )
      );

    if (/^Plot\s*\[/.test(input)) {
      invoke<RawPlotData>('plot_data', { input })
        .then(raw => {
          const plotData: PlotData = {
            points: raw.points,
            varName: raw.var_name,
            expr: raw.expr,
            xMin: raw.x_min,
            xMax: raw.x_max,
          };
          setCells(prev =>
            prev.map(c =>
              c.id === cellId ? { ...c, status: 'done' as const, output: null, plotData } : c
            )
          );
        })
        .catch(onError);
    } else {
      invoke<EvalResult>('evaluate', { input })
        .then(result =>
          setCells(prev =>
            prev.map(c =>
              c.id === cellId
                ? { ...c, status: 'done' as const, output: result.output, outN: result.out_n }
                : c
            )
          )
        )
        .catch(onError);
    }
  }, []);

  const addCell = useCallback((type: CellType = 'code') => {
    const num = nextNumber.current++;
    setCells(prev => [...prev, makeCell(num, type)]);
  }, []);

  const deleteCell = useCallback((id: string) => {
    setCells(prev => {
      if (prev.length <= 1) return prev;
      return prev.filter(c => c.id !== id);
    });
  }, []);

  const updateInput = useCallback((id: string, text: string) => {
    setCells(prev =>
      prev.map(c => (c.id === id ? { ...c, input: text } : c))
    );
  }, []);

  const moveCell = useCallback((id: string, direction: 'up' | 'down') => {
    setCells(prev => {
      const idx = prev.findIndex(c => c.id === id);
      if (idx === -1) return prev;
      if (direction === 'up' && idx === 0) return prev;
      if (direction === 'down' && idx === prev.length - 1) return prev;
      const next = [...prev];
      const swapIdx = direction === 'up' ? idx - 1 : idx + 1;
      [next[idx], next[swapIdx]] = [next[swapIdx], next[idx]];
      return next;
    });
  }, []);

  const newNotebook = useCallback(() => {
    const fresh = helloWorldCells();
    nextNumber.current = fresh.length + 1;
    setCells(fresh);
    setCurrentPath(null);
    invoke('reset_session').catch(() => {});
  }, []);

  const loadFromContent = useCallback((content: string, path: string) => {
    const nb: NotebookFile = JSON.parse(content);
    let n = 1;
    const loaded = (nb.cells ?? []).map(({ type, input }) => makeCell(n++, type, input));
    nextNumber.current = n;
    setCells(loaded.length ? loaded : helloWorldCells());
    setCurrentPath(path);
    invoke('reset_session').catch(() => {});
  }, []);

  const openNotebook = useCallback(async () => {
    const result = await open({
      filters: [{ name: 'PicoCAS Notebook', extensions: ['pico'] }],
      multiple: false,
    });
    const path = Array.isArray(result) ? result[0] : result;
    if (!path) return;
    const content = await invoke<string>('load_notebook', { path });
    loadFromContent(content, path);
  }, [loadFromContent]);

  const doSave = useCallback(async (path: string) => {
    const nb: NotebookFile = {
      version: 1,
      cells: cellsRef.current.map(c => ({ type: c.type, input: c.input })),
    };
    await invoke('save_notebook', { path, content: JSON.stringify(nb, null, 2) });
    setCurrentPath(path);
  }, []);

  const saveNotebook = useCallback(async () => {
    const path = currentPath ?? await save({
      defaultPath: 'notebook.pico',
      filters: [{ name: 'PicoCAS Notebook', extensions: ['pico'] }],
    });
    if (!path) return;
    await doSave(path);
  }, [currentPath, doSave]);

  const saveNotebookAs = useCallback(async () => {
    const path = await save({
      defaultPath: currentPath ?? 'notebook.pico',
      filters: [{ name: 'PicoCAS Notebook', extensions: ['pico'] }],
    });
    if (!path) return;
    await doSave(path);
  }, [currentPath, doSave]);

  return {
    cells,
    currentPath,
    evaluate,
    addCell,
    deleteCell,
    updateInput,
    moveCell,
    newNotebook,
    openNotebook,
    saveNotebook,
    saveNotebookAs,
  };
}
