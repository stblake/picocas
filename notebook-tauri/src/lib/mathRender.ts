import katex from 'katex';

/**
 * Convert PicoCAS output string to LaTeX, then render via KaTeX.
 * Returns an HTML string on success, or null if KaTeX fails (caller should fallback to plain text).
 */
export function renderMath(picoOutput: string): string | null {
  try {
    const latex = picoToLatex(picoOutput);
    return katex.renderToString(latex, {
      throwOnError: true,
      displayMode: true,
    });
  } catch {
    return null;
  }
}

/**
 * Convert PicoCAS output notation to LaTeX string.
 */
function picoToLatex(s: string): string {
  s = s.trim();

  // Handle empty
  if (!s) return '';

  // Parse and convert the expression
  const tokens = tokenize(s);
  const ast = parseExpr(tokens, 0, tokens.length);
  return astToLatex(ast);
}

// --- Tokenizer ---

type Token =
  | { type: 'number'; value: string }
  | { type: 'symbol'; value: string }
  | { type: 'op'; value: string }
  | { type: 'lbracket' }
  | { type: 'rbracket' }
  | { type: 'lbrace' }
  | { type: 'rbrace' }
  | { type: 'lparen' }
  | { type: 'rparen' }
  | { type: 'comma' }
  | { type: 'string'; value: string };

function tokenize(s: string): Token[] {
  const tokens: Token[] = [];
  let i = 0;
  while (i < s.length) {
    if (s[i] === ' ' || s[i] === '\t') { i++; continue; }
    if (s[i] === '"') {
      let j = i + 1;
      while (j < s.length && s[j] !== '"') {
        if (s[j] === '\\') j++;
        j++;
      }
      tokens.push({ type: 'string', value: s.slice(i, j + 1) });
      i = j + 1;
      continue;
    }
    if (/[0-9]/.test(s[i]) || (s[i] === '.' && i + 1 < s.length && /[0-9]/.test(s[i + 1]))) {
      let j = i;
      while (j < s.length && /[0-9.]/.test(s[j])) j++;
      if (j < s.length && /[eE]/.test(s[j])) {
        j++;
        if (j < s.length && /[+-]/.test(s[j])) j++;
        while (j < s.length && /[0-9]/.test(s[j])) j++;
      }
      tokens.push({ type: 'number', value: s.slice(i, j) });
      i = j;
      continue;
    }
    if (/[a-zA-Z$]/.test(s[i])) {
      let j = i;
      while (j < s.length && /[a-zA-Z$0-9]/.test(s[j])) j++;
      tokens.push({ type: 'symbol', value: s.slice(i, j) });
      i = j;
      continue;
    }
    if (s[i] === '[') { tokens.push({ type: 'lbracket' }); i++; continue; }
    if (s[i] === ']') { tokens.push({ type: 'rbracket' }); i++; continue; }
    if (s[i] === '{') { tokens.push({ type: 'lbrace' }); i++; continue; }
    if (s[i] === '}') { tokens.push({ type: 'rbrace' }); i++; continue; }
    if (s[i] === '(') { tokens.push({ type: 'lparen' }); i++; continue; }
    if (s[i] === ')') { tokens.push({ type: 'rparen' }); i++; continue; }
    if (s[i] === ',') { tokens.push({ type: 'comma' }); i++; continue; }
    // Operators
    if (s[i] === '^' || s[i] === '+' || s[i] === '-' || s[i] === '*' || s[i] === '/' || s[i] === '=') {
      tokens.push({ type: 'op', value: s[i] });
      i++;
      continue;
    }
    // Skip unknown chars
    i++;
  }
  return tokens;
}

// --- AST ---

type Ast =
  | { kind: 'num'; value: string }
  | { kind: 'sym'; value: string }
  | { kind: 'str'; value: string }
  | { kind: 'call'; head: string; args: Ast[] }
  | { kind: 'list'; items: Ast[] }
  | { kind: 'binop'; op: string; left: Ast; right: Ast }
  | { kind: 'neg'; child: Ast }
  | { kind: 'group'; child: Ast };

function parseExpr(tokens: Token[], start: number, end: number): Ast {
  // Strip outer parens
  if (start < end && tokens[start].type === 'lparen' && findClosing(tokens, start, 'lparen', 'rparen') === end - 1) {
    return { kind: 'group', child: parseExpr(tokens, start + 1, end - 1) };
  }

  // Look for lowest-precedence binary op (+ or - at top level)
  let depth = 0;
  let lastPlus = -1;
  for (let i = end - 1; i >= start; i--) {
    const t = tokens[i];
    if (t.type === 'rbracket' || t.type === 'rbrace' || t.type === 'rparen') depth++;
    if (t.type === 'lbracket' || t.type === 'lbrace' || t.type === 'lparen') depth--;
    if (depth === 0 && t.type === 'op' && (t.value === '+' || t.value === '-') && i > start) {
      lastPlus = i;
      break;
    }
  }
  if (lastPlus !== -1) {
    const op = (tokens[lastPlus] as { type: 'op'; value: string }).value;
    return {
      kind: 'binop',
      op,
      left: parseExpr(tokens, start, lastPlus),
      right: parseExpr(tokens, lastPlus + 1, end),
    };
  }

  // Multiplication and division (implicit multiplication via adjacency handled later)
  depth = 0;
  let lastMul = -1;
  for (let i = end - 1; i >= start; i--) {
    const t = tokens[i];
    if (t.type === 'rbracket' || t.type === 'rbrace' || t.type === 'rparen') depth++;
    if (t.type === 'lbracket' || t.type === 'lbrace' || t.type === 'lparen') depth--;
    if (depth === 0 && t.type === 'op' && (t.value === '*' || t.value === '/') && i > start) {
      lastMul = i;
      break;
    }
  }
  if (lastMul !== -1) {
    const op = (tokens[lastMul] as { type: 'op'; value: string }).value;
    return {
      kind: 'binop',
      op,
      left: parseExpr(tokens, start, lastMul),
      right: parseExpr(tokens, lastMul + 1, end),
    };
  }

  // Power (right-associative, find leftmost ^)
  depth = 0;
  for (let i = start; i < end; i++) {
    const t = tokens[i];
    if (t.type === 'lbracket' || t.type === 'lbrace' || t.type === 'lparen') depth++;
    if (t.type === 'rbracket' || t.type === 'rbrace' || t.type === 'rparen') depth--;
    if (depth === 0 && t.type === 'op' && t.value === '^' && i > start) {
      return {
        kind: 'binop',
        op: '^',
        left: parseExpr(tokens, start, i),
        right: parseExpr(tokens, i + 1, end),
      };
    }
  }

  // Unary minus
  if (tokens[start].type === 'op' && (tokens[start] as { type: 'op'; value: string }).value === '-') {
    return { kind: 'neg', child: parseExpr(tokens, start + 1, end) };
  }

  // Function call: symbol[ ... ]
  if (end - start >= 3 && tokens[start].type === 'symbol' && tokens[start + 1].type === 'lbracket') {
    const closeIdx = findClosing(tokens, start + 1, 'lbracket', 'rbracket');
    if (closeIdx === end - 1) {
      const args = splitByComma(tokens, start + 2, closeIdx);
      return {
        kind: 'call',
        head: (tokens[start] as { type: 'symbol'; value: string }).value,
        args: args.map(([s, e]) => parseExpr(tokens, s, e)),
      };
    }
  }

  // List: { ... }
  if (tokens[start].type === 'lbrace' && findClosing(tokens, start, 'lbrace', 'rbrace') === end - 1) {
    const items = splitByComma(tokens, start + 1, end - 1);
    return {
      kind: 'list',
      items: items.map(([s, e]) => parseExpr(tokens, s, e)),
    };
  }

  // Single token
  if (end - start === 1) {
    const t = tokens[start];
    if (t.type === 'number') return { kind: 'num', value: t.value };
    if (t.type === 'symbol') return { kind: 'sym', value: t.value };
    if (t.type === 'string') return { kind: 'str', value: t.value };
  }

  // Implicit multiplication: multiple adjacent atoms
  if (end - start > 1) {
    const parts = splitImplicitMul(tokens, start, end);
    if (parts.length > 1) {
      let result = parseExpr(tokens, parts[0][0], parts[0][1]);
      for (let i = 1; i < parts.length; i++) {
        result = {
          kind: 'binop',
          op: '*',
          left: result,
          right: parseExpr(tokens, parts[i][0], parts[i][1]),
        };
      }
      return result;
    }
  }

  // Fallback: concatenate tokens as symbol
  const raw = tokens.slice(start, end).map(t => {
    if ('value' in t) return t.value;
    if (t.type === 'lbracket') return '[';
    if (t.type === 'rbracket') return ']';
    if (t.type === 'lbrace') return '{';
    if (t.type === 'rbrace') return '}';
    if (t.type === 'lparen') return '(';
    if (t.type === 'rparen') return ')';
    if (t.type === 'comma') return ',';
    return '';
  }).join('');
  return { kind: 'sym', value: raw };
}

function findClosing(tokens: Token[], start: number, open: string, close: string): number {
  let depth = 0;
  for (let i = start; i < tokens.length; i++) {
    if (tokens[i].type === open) depth++;
    if (tokens[i].type === close) { depth--; if (depth === 0) return i; }
  }
  return -1;
}

function splitByComma(tokens: Token[], start: number, end: number): [number, number][] {
  if (start >= end) return [];
  const parts: [number, number][] = [];
  let depth = 0;
  let partStart = start;
  for (let i = start; i < end; i++) {
    const t = tokens[i];
    if (t.type === 'lbracket' || t.type === 'lbrace' || t.type === 'lparen') depth++;
    if (t.type === 'rbracket' || t.type === 'rbrace' || t.type === 'rparen') depth--;
    if (depth === 0 && t.type === 'comma') {
      parts.push([partStart, i]);
      partStart = i + 1;
    }
  }
  parts.push([partStart, end]);
  return parts;
}

function splitImplicitMul(tokens: Token[], start: number, end: number): [number, number][] {
  const parts: [number, number][] = [];
  let i = start;
  while (i < end) {
    const partStart = i;
    const t = tokens[i];
    if (t.type === 'symbol' && i + 1 < end && tokens[i + 1].type === 'lbracket') {
      const close = findClosing(tokens, i + 1, 'lbracket', 'rbracket');
      if (close !== -1) { i = close + 1; parts.push([partStart, i]); continue; }
    }
    if (t.type === 'lbrace') {
      const close = findClosing(tokens, i, 'lbrace', 'rbrace');
      if (close !== -1) { i = close + 1; parts.push([partStart, i]); continue; }
    }
    if (t.type === 'lparen') {
      const close = findClosing(tokens, i, 'lparen', 'rparen');
      if (close !== -1) { i = close + 1; parts.push([partStart, i]); continue; }
    }
    i++;
    parts.push([partStart, i]);
  }
  return parts;
}

// --- AST to LaTeX ---

const SYMBOL_MAP: Record<string, string> = {
  'Pi': '\\pi',
  'E': 'e',
  'I': 'i',
  'Infinity': '\\infty',
  'True': '\\text{True}',
  'False': '\\text{False}',
  'Null': '\\text{Null}',
};

const TRIG_FUNCS: Record<string, string> = {
  'Sin': '\\sin',
  'Cos': '\\cos',
  'Tan': '\\tan',
  'ArcSin': '\\arcsin',
  'ArcCos': '\\arccos',
  'ArcTan': '\\arctan',
  'Log': '\\log',
  'Exp': '\\exp',
};

function astToLatex(ast: Ast): string {
  switch (ast.kind) {
    case 'num':
      return ast.value;

    case 'sym':
      return SYMBOL_MAP[ast.value] ?? ast.value;

    case 'str':
      return `\\text{${ast.value}}`;

    case 'neg':
      return `-${wrapIfComplex(ast.child)}`;

    case 'group':
      return `\\left(${astToLatex(ast.child)}\\right)`;

    case 'list':
      return `\\left\\{${ast.items.map(astToLatex).join(', ')}\\right\\}`;

    case 'binop': {
      if (ast.op === '^') {
        const base = wrapIfComplex(ast.left);
        const exp = astToLatex(ast.right);
        return `${base}^{${exp}}`;
      }
      if (ast.op === '/') {
        return `\\frac{${astToLatex(ast.left)}}{${astToLatex(ast.right)}}`;
      }
      if (ast.op === '*') {
        const l = astToLatex(ast.left);
        const r = astToLatex(ast.right);
        // Use \cdot between numbers, implicit multiplication otherwise
        if (ast.left.kind === 'num' && ast.right.kind === 'num') {
          return `${l} \\cdot ${r}`;
        }
        // Number times symbol: no explicit operator (e.g., "3 x")
        return `${l} \\, ${r}`;
      }
      if (ast.op === '+') {
        return `${astToLatex(ast.left)} + ${astToLatex(ast.right)}`;
      }
      if (ast.op === '-') {
        return `${astToLatex(ast.left)} - ${astToLatex(ast.right)}`;
      }
      return `${astToLatex(ast.left)} ${ast.op} ${astToLatex(ast.right)}`;
    }

    case 'call': {
      // Special functions
      if (ast.head === 'Sqrt' && ast.args.length === 1) {
        return `\\sqrt{${astToLatex(ast.args[0])}}`;
      }
      if (ast.head === 'Abs' && ast.args.length === 1) {
        return `\\left|${astToLatex(ast.args[0])}\\right|`;
      }
      if (TRIG_FUNCS[ast.head] && ast.args.length === 1) {
        return `${TRIG_FUNCS[ast.head]}\\left(${astToLatex(ast.args[0])}\\right)`;
      }
      if (ast.head === 'Rational' && ast.args.length === 2) {
        return `\\frac{${astToLatex(ast.args[0])}}{${astToLatex(ast.args[1])}}`;
      }
      if (ast.head === 'Complex' && ast.args.length === 2) {
        const re = astToLatex(ast.args[0]);
        const im = astToLatex(ast.args[1]);
        if (re === '0') return `${im} \\, i`;
        return `${re} + ${im} \\, i`;
      }
      if (ast.head === 'Integrate' && ast.args.length === 2) {
        return `\\int ${astToLatex(ast.args[0])} \\, d${astToLatex(ast.args[1])}`;
      }
      if (ast.head === 'D' && ast.args.length === 2) {
        return `\\frac{d}{d${astToLatex(ast.args[1])}} ${wrapIfComplex(ast.args[0])}`;
      }
      if (ast.head === 'Sum' && ast.args.length >= 2) {
        return `\\sum ${astToLatex(ast.args[0])}`;
      }
      if (ast.head === 'Product' && ast.args.length >= 2) {
        return `\\prod ${astToLatex(ast.args[0])}`;
      }
      if (ast.head === 'Limit' && ast.args.length >= 2) {
        return `\\lim ${astToLatex(ast.args[0])}`;
      }
      if (ast.head === 'Power' && ast.args.length === 2) {
        return `${wrapIfComplex(ast.args[0])}^{${astToLatex(ast.args[1])}}`;
      }
      if (ast.head === 'Times' && ast.args.length >= 2) {
        return ast.args.map(a => wrapIfComplex(a)).join(' \\, ');
      }
      if (ast.head === 'Plus' && ast.args.length >= 2) {
        return ast.args.map(astToLatex).join(' + ');
      }
      if (ast.head === 'List') {
        return `\\left\\{${ast.args.map(astToLatex).join(', ')}\\right\\}`;
      }
      if (ast.head === 'Factor') {
        // Factor didn't evaluate — show as-is
        return `\\text{Factor}\\left[${ast.args.map(astToLatex).join(', ')}\\right]`;
      }

      // Generic function
      return `\\text{${ast.head}}\\left(${ast.args.map(astToLatex).join(', ')}\\right)`;
    }
  }
}

function wrapIfComplex(ast: Ast): string {
  const latex = astToLatex(ast);
  if (ast.kind === 'binop' && (ast.op === '+' || ast.op === '-')) {
    return `\\left(${latex}\\right)`;
  }
  return latex;
}
