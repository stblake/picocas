import { useMemo } from 'react';
import { renderMath } from '../lib/mathRender';
import 'katex/dist/katex.min.css';

interface CellOutputProps {
  output: string;
  isError?: boolean;
}

export function CellOutput({ output, isError }: CellOutputProps) {
  const rendered = useMemo(() => {
    if (isError) return null;
    return renderMath(output);
  }, [output, isError]);

  if (isError) {
    return <div className="cell-output-error">{output}</div>;
  }

  if (rendered) {
    return (
      <div
        className="cell-output-math"
        dangerouslySetInnerHTML={{ __html: rendered }}
      />
    );
  }

  return <div className="cell-output-text">{output}</div>;
}
