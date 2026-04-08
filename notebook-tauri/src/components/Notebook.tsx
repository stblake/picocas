import { Cell } from './Cell';
import type { NotebookContext } from '../hooks/useNotebook';

interface NotebookProps {
  notebook: NotebookContext;
}

export function Notebook({ notebook }: NotebookProps) {
  const {
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
  } = notebook;

  const filename = currentPath
    ? currentPath.split('/').pop() ?? currentPath
    : 'Untitled';

  return (
    <div className="notebook">
      <div className="notebook-header">
        <div className="notebook-title-row">
          <h1 className="notebook-title">PicoCAS</h1>
          <span className="notebook-filename">{filename}</span>
        </div>
        <div className="notebook-toolbar">
          <button className="nb-btn" onClick={newNotebook} title="New notebook">New</button>
          <button className="nb-btn" onClick={openNotebook} title="Open notebook">Open</button>
          <button className="nb-btn" onClick={saveNotebook} title="Save (Cmd+S)">Save</button>
          <button className="nb-btn" onClick={saveNotebookAs} title="Save as new file">Save As</button>
        </div>
      </div>

      <div className="notebook-cells">
        {cells.map((cell, idx) => (
          <Cell
            key={cell.id}
            cell={cell}
            onEvaluate={() => evaluate(cell.id)}
            onUpdateInput={(text) => updateInput(cell.id, text)}
            onDelete={() => deleteCell(cell.id)}
            onMoveUp={() => moveCell(cell.id, 'up')}
            onMoveDown={() => moveCell(cell.id, 'down')}
            isFirst={idx === 0}
            isLast={idx === cells.length - 1}
            isOnly={cells.length === 1}
          />
        ))}
      </div>

      <div className="add-cell-row">
        <button className="add-cell-btn" onClick={() => addCell('code')}>+ Code</button>
        <button className="add-cell-btn" onClick={() => addCell('text')}>+ Text</button>
      </div>
    </div>
  );
}
