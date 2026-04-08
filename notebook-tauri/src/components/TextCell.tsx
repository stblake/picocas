import { useState, useRef, useCallback, useEffect } from 'react';

interface TextCellProps {
  input: string;
  onChange: (text: string) => void;
}

function esc(s: string): string {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

function renderMarkdown(text: string): string {
  return text
    .split('\n')
    .map(line => {
      if (line.startsWith('### ')) return `<h3>${esc(line.slice(4))}</h3>`;
      if (line.startsWith('## ')) return `<h2>${esc(line.slice(3))}</h2>`;
      if (line.startsWith('# ')) return `<h1>${esc(line.slice(2))}</h1>`;
      const inline = esc(line)
        .replace(/\*\*(.+?)\*\*/g, '<strong>$1</strong>')
        .replace(/\*(.+?)\*/g, '<em>$1</em>');
      return line.trim() ? `<p>${inline}</p>` : '<br>';
    })
    .join('');
}

function autoResize(el: HTMLTextAreaElement) {
  el.style.height = 'auto';
  el.style.height = el.scrollHeight + 'px';
}

export function TextCell({ input, onChange }: TextCellProps) {
  const [editing, setEditing] = useState(input.trim() === '');
  const textareaRef = useRef<HTMLTextAreaElement>(null);

  // Size textarea to content whenever editing starts or content changes
  useEffect(() => {
    if (editing && textareaRef.current) {
      autoResize(textareaRef.current);
    }
  }, [editing, input]);

  const startEditing = useCallback(() => setEditing(true), []);

  const stopEditing = useCallback(() => {
    if (input.trim()) setEditing(false);
  }, [input]);

  const handleChange = useCallback(
    (e: React.ChangeEvent<HTMLTextAreaElement>) => {
      onChange(e.target.value);
      autoResize(e.target);
    },
    [onChange]
  );

  const handleKeyDown = useCallback(
    (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
      if (e.key === 'Enter' && e.shiftKey) {
        e.preventDefault();
        textareaRef.current?.blur();
      }
    },
    []
  );

  if (editing) {
    return (
      <textarea
        ref={textareaRef}
        className="text-cell-editor"
        value={input}
        onChange={handleChange}
        onKeyDown={handleKeyDown}
        onBlur={stopEditing}
        placeholder="# Heading&#10;Text with **bold** and *italic*"
        autoFocus
      />
    );
  }

  return (
    <div
      className="text-cell-rendered"
      onClick={startEditing}
      dangerouslySetInnerHTML={{ __html: renderMarkdown(input) }}
    />
  );
}
