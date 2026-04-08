interface CellToolbarProps {
  onDelete: () => void;
  onMoveUp: () => void;
  onMoveDown: () => void;
  isFirst: boolean;
  isLast: boolean;
  isOnly: boolean;
}

export function CellToolbar({ onDelete, onMoveUp, onMoveDown, isFirst, isLast, isOnly }: CellToolbarProps) {
  return (
    <div className="cell-toolbar">
      <button
        className="cell-toolbar-btn"
        onClick={onMoveUp}
        disabled={isFirst}
        title="Move up"
        aria-label="Move cell up"
      >
        &#x25B2;
      </button>
      <button
        className="cell-toolbar-btn"
        onClick={onMoveDown}
        disabled={isLast}
        title="Move down"
        aria-label="Move cell down"
      >
        &#x25BC;
      </button>
      <button
        className="cell-toolbar-btn cell-toolbar-delete"
        onClick={onDelete}
        disabled={isOnly}
        title="Delete cell"
        aria-label="Delete cell"
      >
        &#x2715;
      </button>
    </div>
  );
}
