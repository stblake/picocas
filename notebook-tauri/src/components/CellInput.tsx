import { useEffect, useRef, useCallback } from 'react';
import { EditorView, keymap, placeholder } from '@codemirror/view';
import { EditorState } from '@codemirror/state';
import { defaultHighlightStyle, syntaxHighlighting } from '@codemirror/language';
import { picocasLanguage } from '../lib/picocasHighlight';

interface CellInputProps {
  value: string;
  onChange: (text: string) => void;
  onEvaluate: () => void;
  disabled?: boolean;
}

export function CellInput({ value, onChange, onEvaluate, disabled }: CellInputProps) {
  const containerRef = useRef<HTMLDivElement>(null);
  const viewRef = useRef<EditorView | null>(null);
  const onChangeRef = useRef(onChange);
  const onEvaluateRef = useRef(onEvaluate);

  onChangeRef.current = onChange;
  onEvaluateRef.current = onEvaluate;

  const createView = useCallback(() => {
    if (!containerRef.current) return;

    const state = EditorState.create({
      doc: value,
      extensions: [
        picocasLanguage,
        syntaxHighlighting(defaultHighlightStyle),
        placeholder('Enter expression...'),
        keymap.of([
          {
            key: 'Shift-Enter',
            run: () => {
              onEvaluateRef.current();
              return true;
            },
          },
        ]),
        EditorView.updateListener.of((update) => {
          if (update.docChanged) {
            onChangeRef.current(update.state.doc.toString());
          }
        }),
        EditorView.theme({
          '&': {
            fontSize: '14px',
            fontFamily: '"JetBrains Mono", "Fira Code", "SF Mono", Menlo, Monaco, monospace',
          },
          '.cm-content': {
            padding: '8px 0',
            minHeight: '24px',
          },
          '.cm-focused': {
            outline: 'none',
          },
          '.cm-line': {
            padding: '0 4px',
          },
          '.cm-placeholder': {
            color: '#999',
            fontStyle: 'italic',
          },
        }),
        EditorState.readOnly.of(!!disabled),
      ],
    });

    viewRef.current = new EditorView({
      state,
      parent: containerRef.current,
    });
  }, []); // Intentionally empty - we use refs for callbacks

  useEffect(() => {
    createView();
    return () => {
      viewRef.current?.destroy();
      viewRef.current = null;
    };
  }, [createView]);

  // Sync external value changes (but avoid loops from our own onChange)
  useEffect(() => {
    const view = viewRef.current;
    if (view && view.state.doc.toString() !== value) {
      view.dispatch({
        changes: { from: 0, to: view.state.doc.length, insert: value },
      });
    }
  }, [value]);

  return <div ref={containerRef} className="cell-input-editor" />;
}
