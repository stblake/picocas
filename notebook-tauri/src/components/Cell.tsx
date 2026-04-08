import { CellInput } from './CellInput';
import { CellOutput } from './CellOutput';
import { CellToolbar } from './CellToolbar';
import { PlotOutput } from './PlotOutput';
import { TextCell } from './TextCell';
import type { Cell as CellType } from '../hooks/useNotebook';

interface CellProps {
  cell: CellType;
  onEvaluate: () => void;
  onUpdateInput: (text: string) => void;
  onDelete: () => void;
  onMoveUp: () => void;
  onMoveDown: () => void;
  isFirst: boolean;
  isLast: boolean;
  isOnly: boolean;
}

export function Cell({
  cell,
  onEvaluate,
  onUpdateInput,
  onDelete,
  onMoveUp,
  onMoveDown,
  isFirst,
  isLast,
  isOnly,
}: CellProps) {
  if (cell.type === 'text') {
    return (
      <div className="cell cell-text">
        <CellToolbar
          onDelete={onDelete}
          onMoveUp={onMoveUp}
          onMoveDown={onMoveDown}
          isFirst={isFirst}
          isLast={isLast}
          isOnly={isOnly}
        />
        <TextCell input={cell.input} onChange={onUpdateInput} />
      </div>
    );
  }

  const isEvaluating = cell.status === 'evaluating';

  return (
    <div className={`cell ${isEvaluating ? 'cell-evaluating' : ''}`}>
      <CellToolbar
        onDelete={onDelete}
        onMoveUp={onMoveUp}
        onMoveDown={onMoveDown}
        isFirst={isFirst}
        isLast={isLast}
        isOnly={isOnly}
      />

      <div className="cell-input-row">
        <div className="cell-label cell-label-in">
          In[{cell.cellNumber}]:=
        </div>
        <div className="cell-input-wrapper">
          <CellInput
            value={cell.input}
            onChange={onUpdateInput}
            onEvaluate={onEvaluate}
            disabled={isEvaluating}
          />
        </div>
        <button
          className="cell-eval-btn"
          onClick={onEvaluate}
          disabled={isEvaluating}
          title="Evaluate (Shift+Enter)"
          aria-label="Evaluate cell"
        >
          {isEvaluating ? (
            <span className="spinner" />
          ) : (
            <span>&#x25B6;</span>
          )}
        </button>
      </div>

      {(cell.output !== null || cell.plotData !== null) && (
        <div className="cell-output-row">
          <div className="cell-label cell-label-out">
            Out[{cell.outN ?? cell.cellNumber}]=
          </div>
          <div className="cell-output-wrapper">
            {cell.plotData ? (
              <PlotOutput plotData={cell.plotData} />
            ) : (
              <CellOutput
                output={cell.output!}
                isError={cell.status === 'error'}
              />
            )}
          </div>
        </div>
      )}
    </div>
  );
}
