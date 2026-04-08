import { StreamLanguage, StringStream } from '@codemirror/language';
import { tags } from '@lezer/highlight';

const KEYWORDS = new Set([
  'Plus', 'Times', 'Power', 'Subtract', 'Divide',
  'Sin', 'Cos', 'Tan', 'ArcSin', 'ArcCos', 'ArcTan',
  'Log', 'Exp', 'Sqrt', 'Abs',
  'List', 'Table', 'Map', 'Apply', 'Select', 'Sort', 'Range',
  'If', 'Which', 'Switch', 'Do', 'While', 'For',
  'Set', 'SetDelayed', 'Rule', 'RuleDelayed',
  'Expand', 'Factor', 'Simplify', 'Together', 'Apart',
  'D', 'Integrate', 'Sum', 'Product', 'Limit',
  'Head', 'Length', 'Part', 'Extract', 'Position',
  'Append', 'Prepend', 'Join', 'Flatten', 'Partition',
  'Replace', 'ReplaceAll', 'ReplaceRepeated',
  'MatchQ', 'FreeQ', 'MemberQ', 'NumericQ', 'IntegerQ',
  'True', 'False', 'Null', 'None',
  'Print', 'Module', 'Block', 'With',
  'Hold', 'HoldForm', 'Evaluate', 'ReleaseHold',
  'Rational', 'Complex', 'Infinity', 'Pi', 'E', 'I',
  'Count', 'Cases', 'DeleteCases', 'Tally',
  'Quit',
]);

interface PicoCASState {
  commentDepth: number;
}

export const picocasLanguage = StreamLanguage.define<PicoCASState>({
  name: 'picocas',

  startState(): PicoCASState {
    return { commentDepth: 0 };
  },

  token(stream: StringStream, state: PicoCASState): string | null {
    // Inside a comment
    if (state.commentDepth > 0) {
      while (!stream.eol()) {
        if (stream.match('(*')) {
          state.commentDepth++;
        } else if (stream.match('*)')) {
          state.commentDepth--;
          if (state.commentDepth === 0) return 'comment';
        } else {
          stream.next();
        }
      }
      return 'comment';
    }

    // Start of comment
    if (stream.match('(*')) {
      state.commentDepth = 1;
      while (!stream.eol()) {
        if (stream.match('(*')) {
          state.commentDepth++;
        } else if (stream.match('*)')) {
          state.commentDepth--;
          if (state.commentDepth === 0) return 'comment';
        } else {
          stream.next();
        }
      }
      return 'comment';
    }

    // Strings
    if (stream.match('"')) {
      while (!stream.eol()) {
        const ch = stream.next();
        if (ch === '\\') {
          stream.next(); // skip escaped char
        } else if (ch === '"') {
          return 'string';
        }
      }
      return 'string';
    }

    // Numbers (reals and integers)
    if (stream.match(/^[0-9]+\.[0-9]*([eE][+-]?[0-9]+)?/) ||
        stream.match(/^\.[0-9]+([eE][+-]?[0-9]+)?/) ||
        stream.match(/^[0-9]+([eE][+-]?[0-9]+)/)) {
      return 'number';
    }
    if (stream.match(/^[0-9]+/)) {
      return 'number';
    }

    // Multi-char operators (must check before single-char)
    if (stream.match(':=') || stream.match('->') || stream.match(':>') ||
        stream.match('@@') || stream.match('/@') || stream.match('/.')  ||
        stream.match('//.')|| stream.match('===')|| stream.match('=!=') ||
        stream.match('!=') || stream.match('>=') || stream.match('<=') ||
        stream.match(';;') || stream.match('___')|| stream.match('__')) {
      return 'operator';
    }

    // Single-char operators and brackets
    if (stream.match(/^[+\-*/^=<>!@#&|~?`_]/)) {
      return 'operator';
    }

    // Brackets
    if (stream.match(/^[[\]{}()]/)) {
      return 'bracket';
    }

    // Separators
    if (stream.match(',') || stream.match(';')) {
      return 'separator';
    }

    // Identifiers / keywords
    if (stream.match(/^[a-zA-Z$][a-zA-Z$0-9]*/)) {
      const word = stream.current();
      if (KEYWORDS.has(word)) {
        return 'keyword';
      }
      return 'variableName';
    }

    // Whitespace
    if (stream.match(/^\s+/)) {
      return null;
    }

    // Anything else
    stream.next();
    return null;
  },

  languageData: {
    commentTokens: { block: { open: '(*', close: '*)' } },
  },

  tokenTable: {
    keyword: tags.keyword,
    comment: tags.comment,
    string: tags.string,
    number: tags.number,
    operator: tags.operator,
    bracket: tags.bracket,
    separator: tags.separator,
    variableName: tags.variableName,
  },
});
