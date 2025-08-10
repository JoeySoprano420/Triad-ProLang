import * as vscode from 'vscode';

/** Simple per-file index */
interface FileIndex {
  uri: vscode.Uri;
  vars: Map<string, vscode.Position>;            // var -> first assignment
  tupleShapes: Map<string, string[]>;            // var -> fields
  pureDefs: Map<string, { cls?: string, name: string, args: string[], range: vscode.Range, doc?: string }>; // name + Class.name
  inferredShapes: Map<string, Set<string>>;      // from usage t.x, t.y
}

interface WorkspaceIndex {
  byFile: Map<string, FileIndex>;
  varDefs: Map<string, vscode.Location[]>;       // var -> locations
  pureDefs: Map<string, vscode.Location>;        // method or Class.method -> location
}

const KEYWORDS = ['if','else','for','in','and','or','say','echo','new','pure','def','try','catch','finally','throw'];

const RE_PUREDEF = /\bpure\s+def\s+([A-Za-z_][\w]*)\s*\.\s*([A-Za-z_][\w]*)\s*\(([^)]*)\)/g;
const RE_VARS = /\b([A-Za-z_][\w]*)\s*=\s*/g;
const RE_ASSIGN_TUPLE = /\b([A-Za-z_][\w]*)\s*=\s*\(([^)]*)\)/g;
const RE_DOC_BLOCK = /\/\*\*([\s\S]*?)\*\//g;
const RE_DOT_USE = /\b([A-Za-z_][\w]*)\s*\.\s*([A-Za-z_][\w]*)\b/g;

function parseArgsList(s: string): string[] {
  const out: string[] = [];
  for (const part of s.split(',')){
    const p = part.trim();
    if (!p) continue;
    const name = p.split(':')[0].trim();
    if (name) out.push(name);
  }
  return out;
}

function wordRange(doc: vscode.TextDocument, idx: number, len: number): vscode.Range {
  const start = doc.positionAt(idx);
  const end = doc.positionAt(idx + len);
  return new vscode.Range(start, end);
}

function indexOne(doc: vscode.TextDocument): FileIndex {
  const text = doc.getText();
  const vars = new Map<string, vscode.Position>();
  const tupleShapes = new Map<string, string[]>();
  const pureDefs = new Map<string, { cls?: string, name: string, args: string[], range: vscode.Range, doc?: string }>();
  const inferredShapes = new Map<string, Set<string>>();

  for (let m; (m = RE_VARS.exec(text));){
    const v = m[1];
    if (!vars.has(v)) vars.set(v, doc.positionAt(m.index));
  }

  for (let m; (m = RE_ASSIGN_TUPLE.exec(text));){
    const v = m[1];
    const inner = m[2];
    const fields: string[] = [];
    for (const seg of inner.split(',')){
      const kv = seg.trim().split(':').map(s => s.trim());
      if (kv.length>=2 && kv[0]) fields.push(kv[0]);
    }
    if (fields.length) tupleShapes.set(v, fields);
  }

  // inferred shapes from dotted usage
  for (let m; (m = RE_DOT_USE.exec(text));){
    const base = m[1], field = m[2];
    if (!inferredShapes.has(base)) inferredShapes.set(base, new Set());
    inferredShapes.get(base)!.add(field);
  }

  for (let m; (m = RE_PUREDEF.exec(text)); ){
    const cls = m[1];
    const name = m[2];
    const args = parseArgsList(m[3] ?? '');
    // doc block just above
    let docTxt: string | undefined;
    const before = text.substring(0, m.index);
    const docMatch = before.match(RE_DOC_BLOCK);
    if (docMatch && docMatch.length){
      const raw = docMatch[docMatch.length-1];
      if (before.endsWith(raw + "\n")) {
        docTxt = raw.replace(/\/\*\*|\*\//g, '').replace(/^\s*\*\s?/gm, '').trim();
      }
    }
    const rng = wordRange(doc, m.index, m[0].length);
    const entry = { cls, name, args, range: rng, doc: docTxt };
    pureDefs.set(name, entry);
    pureDefs.set(`${cls}.${name}`, entry);
  }

  return { uri: doc.uri, vars, tupleShapes, pureDefs, inferredShapes };
}

async function buildWorkspaceIndex(): Promise<WorkspaceIndex> {
  const byFile = new Map<string, FileIndex>();
  const varDefs = new Map<string, vscode.Location[]>();
  const pureDefs = new Map<string, vscode.Location>();

  const files = await vscode.workspace.findFiles('**/*.triad');
  for (const uri of files){
    const doc = await vscode.workspace.openTextDocument(uri);
    const idx = indexOne(doc);
    byFile.set(uri.toString(), idx);
    for (const [v, pos] of idx.vars){
      const arr = varDefs.get(v) || [];
      arr.push(new vscode.Location(uri, pos));
      varDefs.set(v, arr);
    }
    for (const [k, pd] of idx.pureDefs){
      pureDefs.set(k, new vscode.Location(uri, pd.range.start));
    }
  }

  return { byFile, varDefs, pureDefs };
}

function currentPrefix(line: string, col: number): string {
  const left = line.slice(0, col);
  const m = left.match(/([A-Za-z_][\w]*)$/);
  return m ? m[1] : '';
}

export async function activate(ctx: vscode.ExtensionContext) {
  const wsIndex = await buildWorkspaceIndex();

  // Rebuild on save
  ctx.subscriptions.push(vscode.workspace.onDidSaveTextDocument(async (doc)=>{
    if (doc.languageId!=='triad') return;
    const updated = indexOne(doc);
    wsIndex.byFile.set(doc.uri.toString(), updated);
    wsIndex.varDefs.clear();
    wsIndex.pureDefs.clear();
    for (const [uri, idx] of wsIndex.byFile){
      for (const [v, pos] of idx.vars){
        const arr = wsIndex.varDefs.get(v) || [];
        arr.push(new vscode.Location(vscode.Uri.parse(uri), pos));
        wsIndex.varDefs.set(v, arr);
      }
      for (const [k, pd] of idx.pureDefs){
        wsIndex.pureDefs.set(k, new vscode.Location(vscode.Uri.parse(uri), pd.range.start));
      }
    }
  }));

  // Completion
  ctx.subscriptions.push(vscode.languages.registerCompletionItemProvider('triad', {
    provideCompletionItems(doc, pos) {
      const idx = wsIndex.byFile.get(doc.uri.toString()) || indexOne(doc);
      const line = doc.lineAt(pos.line).text;
      const left = line.slice(0, pos.character);
      const items: vscode.CompletionItem[] = [];

      for (const k of KEYWORDS) items.push(new vscode.CompletionItem(k, vscode.CompletionItemKind.Keyword));
      for (const [name] of (idx.pureDefs)) {
        const it = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
        it.detail = 'pure method';
        items.push(it);
      }
      for (const v of idx.vars.keys()) items.push(new vscode.CompletionItem(v, vscode.CompletionItemKind.Variable));

      // dot context fields: explicit tuple shape or inferred shape
      const dotIdx = left.lastIndexOf('.');
      if (dotIdx >= 0) {
        const base = left.slice(0, dotIdx).match(/([A-Za-z_][\w]*)$/)?.[1];
        if (base){
          const fields = new Set<string>();
          const ex = idx.tupleShapes.get(base) || [];
          ex.forEach(f=>fields.add(f));
          const inf = idx.inferredShapes.get(base) || new Set<string>();
          inf.forEach(f=>fields.add(f));
          for (const f of fields){
            const it = new vscode.CompletionItem(f, vscode.CompletionItemKind.Field);
            it.detail = 'field';
            items.push(it);
          }
        }
      }

      return items;
    }
  }, '.', ':'));

  // Hover
  ctx.subscriptions.push(vscode.languages.registerHoverProvider('triad', {
    provideHover(doc, pos) {
      const idx = wsIndex.byFile.get(doc.uri.toString()) || indexOne(doc);
      const range = doc.getWordRangeAtPosition(pos, /[A-Za-z_][\w]*(?:\.[A-Za-z_][\w]*)?/);
      if (!range) return;
      const text = doc.getText(range);

      const p = idx.pureDefs.get(text) || idx.pureDefs.get(text.split('.').pop() || '');
      if (p) {
        const sig = `${p.cls ? p.cls + '.' : ''}${p.name}(${p.args.join(', ')})`;
        const md = new vscode.MarkdownString();
        md.appendCodeblock(sig, 'triad');
        if (p.doc) md.appendMarkdown('\n' + p.doc);
        md.appendMarkdown('\n
— *pure*');
        md.isTrusted = true;
        return new vscode.Hover(md, range);
      }

      // var shape (explicit or inferred)
      if (idx.vars.has(text)){
        const md = new vscode.MarkdownString();
        const fields = new Set<string>();
        (idx.tupleShapes.get(text) || []).forEach(f=>fields.add(f));
        (idx.inferredShapes.get(text) || new Set<string>()).forEach(f=>fields.add(f));
        if (fields.size){
          md.appendMarkdown(`**shape** ${text}\n`);
          md.appendCodeblock('(' + Array.from(fields).map(f=> f + ': _').join(', ') + ')', 'triad');
          return new vscode.Hover(md, range);
        }
      }
      return;
    }
  }));

  // Go to definition: variables (first assignment) and pure defs
  ctx.subscriptions.push(vscode.languages.registerDefinitionProvider('triad', {
    provideDefinition(doc, pos) {
      const idx = wsIndex.byFile.get(doc.uri.toString()) || indexOne(doc);
      const range = doc.getWordRangeAtPosition(pos, /[A-Za-z_][\w]*(?:\.[A-Za-z_][\w]*)?/);
      if (!range) return;
      const text = doc.getText(range);

      // Pure def
      const pLoc = wsIndex.pureDefs.get(text) || wsIndex.pureDefs.get(text.split('.').pop() || '');
      if (pLoc) return pLoc;

      // Variable (local file only for now)
      const posVar = idx.vars.get(text);
      if (posVar) return new vscode.Location(doc.uri, posVar);

      return;
    }
  }));
}

export function deactivate() {}
