#!/usr/bin/env node
// Builds a report in the team's standard format (modelled on the
// "P16: BVH Ray Tracer – Checkpoint 1" report) from a Markdown file with
// YAML front matter.
//
//   node build_report.mjs report.md                 -> report.docx
//   node build_report.mjs report.md -o out.docx
//   node build_report.mjs report.md -o sablon.dotx  -> Word template (.dotx)
//
// See ../FORMAT.md for the format specification and ../SKILL.md for usage.

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import yaml from "js-yaml";
import { marked } from "marked";
import JSZip from "jszip";
import {
  AlignmentType, BorderStyle, Document, Footer, Header, HeadingLevel, ImageRun,
  LevelFormat, Packer, PageBreak, PageNumber, Paragraph, ShadingType, Table,
  TableCell, TableOfContents, TableRow, TextRun, VerticalAlign, WidthType,
  ExternalHyperlink,
} from "docx";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ASSETS = path.join(HERE, "..", "assets");

// ---------------------------------------------------------------- format spec
// All values measured from the reference PDF. DXA: 1440 = 1 inch, 567 ≈ 1 cm.
// Font sizes are in half-points (24 = 12 pt).
export const F = {
  font: "Times New Roman",
  monoFont: "Consolas",
  page: { width: 11906, height: 16838 },                  // A4
  margin: { top: 1134, bottom: 1134, left: 1560, right: 1380, header: 397, footer: 510 },
  body: { size: 24, line: 276, after: 120, firstLine: 283 }, // 12 pt, 1.15, 6 pt, 0.5 cm
  h1: { size: 27, before: 240, after: 200 },               // 13.5 pt bold
  h2: { size: 24, before: 160, after: 60 },                // 12 pt bold
  h3: { size: 24, before: 120, after: 40 },                // 12 pt bold italic
  caption: { size: 18, after: 120 },                       // 9 pt italic, centred
  table: { size: 20, border: "BFBFBF", headerFill: "F2F2F2" }, // 10 pt
  refs: { size: 18, hanging: 260, after: 40 },             // 9 pt, hanging 0.46 cm
  code: { size: 18, fill: "F5F5F5", border: "D9D9D9" },    // 9 pt Consolas
  header: { size: 16 },                                    // 8 pt caps
  footer: { size: 17, pageSize: 22 },                      // 8.5 pt, page number 11 pt
};
const TEXT_WIDTH = F.page.width - F.margin.left - F.margin.right; // 9026 DXA ≈ 15.9 cm
const CM_TO_PX = 96 / 2.54;

const LABELS = {
  bs: { figure: "Slika", table: "Tabela", authors: "Autori", abstract: "Sažetak",
        references: "Literatura", toc: "Sadržaj", subject: "Predmet", mentor: "Mentor" },
  en: { figure: "Figure", table: "Table", authors: "Authors", abstract: "Abstract",
        references: "References", toc: "Contents", subject: "Course", mentor: "Mentor" },
};

// ------------------------------------------------------------------- helpers
function readFrontMatter(src) {
  const m = src.match(/^---\r?\n([\s\S]*?)\r?\n---\r?\n?([\s\S]*)$/);
  if (!m) return { meta: {}, body: src };
  return { meta: yaml.load(m[1]) || {}, body: m[2] };
}

function imageInfo(file) {
  const buf = fs.readFileSync(file);
  const ext = path.extname(file).toLowerCase();
  if (buf.slice(1, 4).toString() === "PNG") {
    return { buf, type: "png", w: buf.readUInt32BE(16), h: buf.readUInt32BE(20) };
  }
  if (buf[0] === 0xff && buf[1] === 0xd8) {
    let i = 2;
    while (i < buf.length) {
      if (buf[i] !== 0xff) { i++; continue; }
      const marker = buf[i + 1];
      const len = buf.readUInt16BE(i + 2);
      if (marker >= 0xc0 && marker <= 0xcf && ![0xc4, 0xc8, 0xcc].includes(marker)) {
        return { buf, type: "jpg", h: buf.readUInt16BE(i + 5), w: buf.readUInt16BE(i + 7) };
      }
      i += 2 + len;
    }
  }
  if (ext === ".gif") return { buf, type: "gif", w: buf.readUInt16LE(6), h: buf.readUInt16LE(8) };
  if (ext === ".bmp") return { buf, type: "bmp", w: buf.readInt32LE(18), h: Math.abs(buf.readInt32LE(22)) };
  throw new Error(`Unsupported image (use PNG or JPEG): ${file}`);
}

function imageRun(file, widthCm, maxHeightCm = 20) {
  const img = imageInfo(file);
  let w = widthCm * CM_TO_PX;
  let h = (w * img.h) / img.w;
  const maxH = maxHeightCm * CM_TO_PX;
  if (h > maxH) { w = (w * maxH) / h; h = maxH; }
  return new ImageRun({ type: img.type, data: img.buf, transformation: { width: Math.round(w), height: Math.round(h) } });
}

function parseCm(v, fallback) {
  if (v == null) return fallback;
  const s = String(v).trim();
  if (s.endsWith("%")) return (parseFloat(s) / 100) * (TEXT_WIDTH / 567);
  if (s.endsWith("mm")) return parseFloat(s) / 10;
  if (s.endsWith("in")) return parseFloat(s) * 2.54;
  return parseFloat(s);
}

const decode = (s) => s
  .replace(/&amp;/g, "&").replace(/&lt;/g, "<").replace(/&gt;/g, ">")
  .replace(/&quot;/g, '"').replace(/&#39;/g, "'");

// Markdown inline tokens -> TextRun / ExternalHyperlink list.
function inlineRuns(tokens, base = {}) {
  const out = [];
  for (const t of tokens || []) {
    switch (t.type) {
      case "strong": out.push(...inlineRuns(t.tokens, { ...base, bold: true })); break;
      case "em": out.push(...inlineRuns(t.tokens, { ...base, italics: true })); break;
      case "del": out.push(...inlineRuns(t.tokens, { ...base, strike: true })); break;
      case "codespan":
        out.push(new TextRun({ ...base, text: decode(t.text), font: F.monoFont, size: base.size ? base.size - 2 : F.body.size - 2 }));
        break;
      case "br": out.push(new TextRun({ ...base, break: 1 })); break;
      case "link":
        out.push(new ExternalHyperlink({ link: t.href, children: inlineRuns(t.tokens, { ...base, color: "1F4E79", underline: {} }) }));
        break;
      case "image": break; // handled at block level
      case "text":
      case "escape":
        if (t.tokens && t.tokens.length) out.push(...inlineRuns(t.tokens, base));
        else out.push(new TextRun({ ...base, text: decode(t.text) }));
        break;
      default:
        if (t.text) out.push(new TextRun({ ...base, text: decode(t.text) }));
    }
  }
  return out;
}

// ------------------------------------------------------------------- builder
export function buildDocument(mdPath) {
  const mdDir = path.dirname(path.resolve(mdPath));
  const { meta, body } = readFrontMatter(fs.readFileSync(mdPath, "utf8"));
  const lang = (meta.language || "bs").toLowerCase().startsWith("en") ? "en" : "bs";
  const L = { ...LABELS[lang], ...(meta.labels || {}) };
  const resolve = (p) => (path.isAbsolute(p) ? p : path.join(mdDir, p));

  const university = meta.university ?? "Univerzitet u Sarajevu";
  const faculty = meta.faculty ?? "Elektrotehnički fakultet";
  const title = meta.title ?? "Naslov izvještaja";
  const milestone = meta.milestone ?? "";

  let fig = 0, tab = 0, h1 = 0, h2 = 0, h3 = 0, listInstance = 0;
  const children = [];

  // ---------------------------------------------------------------- cover
  const logos = meta.logos === undefined
    ? [{ path: path.join(ASSETS, "logo_unsa.png"), width: 2.95 }, { path: path.join(ASSETS, "logo_etf.png"), width: 7.55 }]
    : (meta.logos || []).map((l) => (typeof l === "string" ? { path: resolve(l), width: 3 } : { ...l, path: resolve(l.path), width: parseCm(l.width, 3) }));

  if (logos.length) {
    const none = { style: BorderStyle.NONE, size: 0, color: "FFFFFF" };
    const noBorders = { top: none, bottom: none, left: none, right: none, insideHorizontal: none, insideVertical: none };
    const cellW = logos.map((l) => Math.round(l.width * 567 + 80));
    children.push(new Table({
      alignment: AlignmentType.CENTER,
      width: { size: cellW.reduce((a, b) => a + b, 0), type: WidthType.DXA },
      columnWidths: cellW,
      borders: noBorders,
      rows: [new TableRow({ children: logos.map((l, i) => new TableCell({
        width: { size: cellW[i], type: WidthType.DXA },
        borders: noBorders,
        verticalAlign: VerticalAlign.CENTER,
        children: [new Paragraph({ alignment: AlignmentType.CENTER, children: [imageRun(l.path, l.width, 3.5)] })],
      })) })],
    }));
  }

  const cover = (text, opts = {}) => new Paragraph({
    alignment: AlignmentType.CENTER,
    spacing: { before: opts.before ?? 0, after: opts.after ?? 0, line: 240 },
    children: [new TextRun({ text, bold: !!opts.bold, size: opts.size ?? 22, allCaps: !!opts.caps })],
  });

  children.push(cover(university, { bold: true, size: 26, caps: true, before: 900 }));
  children.push(cover(faculty, { size: 24, caps: true }));
  children.push(cover(title, { bold: true, size: 38, caps: meta.title_caps !== false, before: 2150, after: 280 }));
  if (meta.doc_type) children.push(cover(meta.doc_type, { size: 28, after: 140 }));
  if (milestone) children.push(cover(milestone, { bold: true, size: 26, caps: true, after: 80 }));
  if (meta.subject) children.push(cover(`${L.subject}: ${meta.subject}`, { size: 22 }));
  if (meta.mentor) children.push(cover(`${L.mentor}: ${meta.mentor}`, { size: 22 }));
  for (const line of meta.cover_extra || []) children.push(cover(String(line), { size: 22 }));

  const authors = meta.authors || [];
  if (authors.length) {
    children.push(cover(meta.authors_label ?? L.authors, { bold: true, size: 24, before: 2440, after: 220 }));
    for (const a of authors) {
      const line = typeof a === "string" ? a : [a.name, a.id].filter(Boolean).join("  -  ");
      children.push(cover(line, { size: 22, after: 100 }));
    }
  }
  if (meta.date) children.push(cover(meta.date, { size: 22, before: authors.length ? 1020 : 2200 }));
  children.push(new Paragraph({ children: [new PageBreak()] }));

  // ---------------------------------------------------------------- front
  if (meta.toc) {
    children.push(new Paragraph({ style: "HeadingUnnumbered", text: L.toc }));
    children.push(new TableOfContents(L.toc, { hyperlink: true, headingStyleRange: "1-3" }));
    children.push(new Paragraph({ children: [new PageBreak()] }));
  }
  if (meta.abstract) {
    children.push(new Paragraph({ heading: HeadingLevel.HEADING_1, children: [new TextRun(meta.abstract_title ?? L.abstract)] }));
    for (const p of String(meta.abstract).trim().split(/\n\s*\n/)) {
      const toks = marked.lexer(p)[0];
      children.push(new Paragraph({ style: "Abstract", children: inlineRuns(toks?.tokens || [{ type: "text", text: p }]) }));
    }
  }

  // ---------------------------------------------------------------- body
  const heading = (depth, rawText, inlineTokens) => {
    let text = rawText.trim();
    const unnumbered = /\s*\{-\}\s*$/.test(text) || /\s*\{\.unnumbered\}\s*$/.test(text) || meta.numbered_headings === false;
    const strip = (s) => s.replace(/\s*\{(-|\.unnumbered)\}\s*$/, "");
    let prefix = "";
    if (!unnumbered) {
      if (depth === 1) { h1++; h2 = 0; h3 = 0; prefix = `${h1}. `; }
      else if (depth === 2) { h2++; h3 = 0; prefix = `${h1}.${h2}. `; }
      else { h3++; prefix = `${h1}.${h2}.${h3}. `; }
    }
    const level = depth === 1 ? HeadingLevel.HEADING_1 : depth === 2 ? HeadingLevel.HEADING_2 : HeadingLevel.HEADING_3;
    const toks = (inlineTokens || []).map((t, i, arr) => (i === arr.length - 1 && t.type === "text" ? { ...t, text: strip(t.text), tokens: undefined } : t));
    return new Paragraph({ heading: level, children: [new TextRun(prefix), ...inlineRuns(toks.length ? toks : [{ type: "text", text: strip(text) }])] });
  };

  const tableBlock = (t, caption) => {
    const cols = t.header.length;
    // Natural width of each column ≈ its longest cell (10 pt Times ≈ 100 DXA/char).
    // Columns narrower than an equal share keep their natural width; the
    // remaining columns split what is left in proportion to their content.
    const natural = t.header.map((h, i) =>
      Math.max(h.text.length, ...t.rows.map((r) => (r[i]?.text || "").length), 4) * 100 + 250);
    let widths;
    if (natural.reduce((a, b) => a + b, 0) <= TEXT_WIDTH) {
      const k = TEXT_WIDTH / natural.reduce((a, b) => a + b, 0);
      widths = natural.map((n) => Math.floor(n * k));
    } else {
      const fixed = natural.map((n) => n < TEXT_WIDTH / cols);
      const rest = TEXT_WIDTH - natural.reduce((a, n, i) => a + (fixed[i] ? n : 0), 0);
      const flexSum = natural.reduce((a, n, i) => a + (fixed[i] ? 0 : n), 0);
      widths = natural.map((n, i) => Math.floor(fixed[i] ? n : (rest * n) / flexSum));
    }
    widths[cols - 1] += TEXT_WIDTH - widths.reduce((a, b) => a + b, 0);

    const b = { style: BorderStyle.SINGLE, size: 4, color: F.table.border };
    const borders = { top: b, bottom: b, left: b, right: b };
    const align = (a) => (a === "center" ? AlignmentType.CENTER : a === "right" ? AlignmentType.RIGHT : AlignmentType.LEFT);
    const cell = (c, i, header) => new TableCell({
      width: { size: widths[i], type: WidthType.DXA },
      borders,
      margins: { top: 30, bottom: 30, left: 100, right: 100 },
      verticalAlign: VerticalAlign.CENTER,
      shading: header && meta.table_header_fill ? { type: ShadingType.CLEAR, color: "auto", fill: F.table.headerFill } : undefined,
      children: [new Paragraph({ style: "TableText", alignment: align(t.align[i]), children: inlineRuns(c.tokens, header ? { bold: true } : {}) })],
    });

    const out = [];
    if (caption) {
      tab++;
      out.push(new Paragraph({ style: "Caption", keepNext: true, children: [new TextRun(`${L.table} ${tab}. `), ...inlineRuns(caption)] }));
    }
    out.push(new Table({
      width: { size: TEXT_WIDTH, type: WidthType.DXA },
      columnWidths: widths,
      rows: [
        new TableRow({ tableHeader: true, children: t.header.map((c, i) => cell(c, i, true)) }),
        ...t.rows.map((r) => new TableRow({ cantSplit: true, children: r.map((c, i) => cell(c, i, false)) })),
      ],
    }));
    out.push(new Paragraph({ style: "AfterTable", children: [] }));
    return out;
  };

  const listBlock = (list, level = 0, instance = ++listInstance) => {
    const out = [];
    for (const item of list.items) {
      let first = true;
      for (const sub of item.tokens) {
        if (sub.type === "list") { out.push(...listBlock(sub, level + 1)); continue; }
        const runs = inlineRuns(sub.tokens || [{ type: "text", text: sub.text || "" }]);
        out.push(new Paragraph({
          style: "ListText",
          numbering: first
            ? { reference: list.ordered ? "ordered" : "bullets", level, instance: list.ordered ? instance : undefined }
            : undefined,
          indent: first ? undefined : { left: 567 * (level + 1) },
          children: runs,
        }));
        first = false;
      }
    }
    return out;
  };

  const tokens = marked.lexer(body, { gfm: true });
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    switch (t.type) {
      case "heading":
        children.push(heading(Math.min(t.depth, 3), t.text, t.tokens));
        break;

      case "paragraph": {
        const raw = t.text.trim();
        if (raw === "\\newpage" || raw === "\\pagebreak") { children.push(new Paragraph({ children: [new PageBreak()] })); break; }
        if (/^: /.test(raw)) break; // table caption already consumed
        const img = t.tokens.find((x) => x.type === "image");
        const others = t.tokens.filter((x) => x !== img && !(x.type === "text" && /^\s*(\{[^}]*\})?\s*$/.test(x.text)));
        if (img && others.length === 0) {
          const attrs = (t.text.match(/\{([^}]*)\}\s*$/) || [])[1] || "";
          const width = parseCm((attrs.match(/width\s*=\s*([\d.]+\s*(cm|mm|in|%)?)/) || [])[1], 14);
          children.push(new Paragraph({ style: "Figure", keepNext: true, children: [imageRun(resolve(decodeURI(img.href)), width)] }));
          if (img.text) {
            fig++;
            children.push(new Paragraph({ style: "Caption", children: [new TextRun(`${L.figure} ${fig}. `), new TextRun(decode(img.text))] }));
          }
          break;
        }
        children.push(new Paragraph({ style: "Normal", children: inlineRuns(t.tokens) }));
        break;
      }

      case "table": {
        const next = tokens[i + 1]?.type === "space" ? tokens[i + 2] : tokens[i + 1];
        let caption = null;
        if (next?.type === "paragraph" && /^: /.test(next.text)) caption = marked.lexer(next.text.slice(2))[0]?.tokens;
        children.push(...tableBlock(t, caption));
        break;
      }

      case "list": children.push(...listBlock(t)); break;

      case "code":
        for (const line of t.text.split("\n")) {
          children.push(new Paragraph({ style: "Code", children: [new TextRun({ text: line.replace(/\t/g, "    ") || " " })] }));
        }
        children.push(new Paragraph({ style: "AfterTable", children: [] }));
        break;

      case "blockquote":
        for (const sub of t.tokens) if (sub.tokens) children.push(new Paragraph({ style: "Quote", children: inlineRuns(sub.tokens) }));
        break;

      case "html":
        if (/<!--\s*(pagebreak|newpage)\s*-->/i.test(t.text)) children.push(new Paragraph({ children: [new PageBreak()] }));
        break;

      case "hr": children.push(new Paragraph({ children: [new PageBreak()] })); break;
      default: break;
    }
  }

  // ---------------------------------------------------------------- references
  if (meta.references && meta.references.length) {
    const refTitle = meta.references_title ?? L.references;
    children.push(heading(1, meta.references_numbered === false ? `${refTitle} {-}` : refTitle));
    meta.references.forEach((r, k) => {
      const toks = marked.lexer(String(r))[0]?.tokens || [{ type: "text", text: String(r) }];
      children.push(new Paragraph({ style: "Reference", children: [new TextRun(`[${k + 1}] `), ...inlineRuns(toks)] }));
    });
  }

  // ---------------------------------------------------------------- header / footer
  const headerText = meta.header ?? `${university}  |  ${faculty}`.toUpperCase();
  const footerText = meta.footer ?? [title, milestone].filter(Boolean).join("  |  ");
  const header = new Header({ children: [new Paragraph({ style: "Header", children: [new TextRun(headerText)] })] });
  const footer = new Footer({ children: [new Paragraph({ style: "Footer", children: [
    new TextRun(footerText ? `${footerText}  |  ` : ""),
    new TextRun({ children: [PageNumber.CURRENT], size: F.footer.pageSize }),
  ] })] });
  const empty = () => new Paragraph({ children: [] });

  return new Document({
    creator: Array.isArray(authors) && authors.length ? authors.map((a) => (typeof a === "string" ? a : a.name)).join(", ") : "",
    title: [title, milestone].filter(Boolean).join(" – "),
    features: { updateFields: !!meta.toc },
    styles: styles(),
    numbering: {
      config: [
        { reference: "bullets", levels: [0, 1, 2].map((lvl) => ({
          level: lvl, format: LevelFormat.BULLET, text: ["•", "◦", "▪"][lvl], alignment: AlignmentType.LEFT,
          style: { paragraph: { indent: { left: 567 * (lvl + 1), hanging: 283 } } },
        })) },
        { reference: "ordered", levels: [0, 1, 2].map((lvl) => ({
          level: lvl, format: [LevelFormat.DECIMAL, LevelFormat.LOWER_LETTER, LevelFormat.LOWER_ROMAN][lvl],
          text: `%${lvl + 1}.`, alignment: AlignmentType.LEFT,
          style: { paragraph: { indent: { left: 567 * (lvl + 1), hanging: 340 } } },
        })) },
      ],
    },
    sections: [{
      properties: {
        titlePage: true,
        page: { size: F.page, margin: F.margin },
      },
      headers: { default: header, first: new Header({ children: [empty()] }) },
      footers: { default: footer, first: new Footer({ children: [empty()] }) },
      children,
    }],
  });
}

// Named styles, so that people editing the result (or the .dotx) in Word can
// reapply the same formatting from the Styles pane.
function styles() {
  const font = F.font;
  const heading = (s, extra = {}) => ({
    run: { font, size: s.size, bold: true, color: "000000", ...extra },
    paragraph: { alignment: AlignmentType.LEFT, indent: { firstLine: 0 }, spacing: { before: s.before, after: s.after, line: 240 }, keepNext: true, keepLines: true },
  });
  return {
    default: {
      document: { run: { font, size: F.body.size, language: { value: "bs-Latn-BA" } } },
      heading1: heading(F.h1),
      heading2: heading(F.h2),
      heading3: heading(F.h3, { italics: true }),
    },
    paragraphStyles: [
      { id: "Normal", name: "Normal", run: { font, size: F.body.size },
        paragraph: { alignment: AlignmentType.JUSTIFIED, spacing: { before: 0, after: F.body.after, line: F.body.line },
          indent: { firstLine: F.body.firstLine } } },
      { id: "Abstract", name: "Abstract", basedOn: "Normal", next: "Normal", quickFormat: true,
        paragraph: { indent: { firstLine: 0 } } },
      { id: "HeadingUnnumbered", name: "Heading Unnumbered", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: F.h1.size, bold: true }, paragraph: { alignment: AlignmentType.LEFT, indent: { firstLine: 0 },
          spacing: { before: F.h1.before, after: F.h1.after, line: 240 }, keepNext: true } },
      { id: "Caption", name: "Caption", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: F.caption.size, italics: true, bold: false, color: "000000" },
        paragraph: { alignment: AlignmentType.CENTER, indent: { firstLine: 0 }, spacing: { before: 40, after: F.caption.after, line: 240 } } },
      { id: "Figure", name: "Figure", basedOn: "Normal", next: "Caption", quickFormat: true,
        paragraph: { alignment: AlignmentType.CENTER, indent: { firstLine: 0 }, spacing: { before: 120, after: 0, line: 240 }, keepNext: true } },
      { id: "TableText", name: "Table Text", basedOn: "Normal", quickFormat: true,
        run: { size: F.table.size },
        paragraph: { alignment: AlignmentType.LEFT, indent: { firstLine: 0 }, spacing: { before: 0, after: 0, line: 240 } } },
      { id: "AfterTable", name: "After Table", basedOn: "Normal", next: "Normal",
        run: { size: 8 }, paragraph: { spacing: { before: 0, after: 80, line: 240 } } },
      { id: "ListText", name: "List Text", basedOn: "Normal", quickFormat: true,
        paragraph: { indent: { firstLine: 0 }, spacing: { after: 60, line: F.body.line } } },
      { id: "Reference", name: "Reference", basedOn: "Normal", quickFormat: true,
        run: { size: F.refs.size },
        paragraph: { indent: { left: F.refs.hanging, hanging: F.refs.hanging, firstLine: 0 }, spacing: { after: F.refs.after, line: 240 } } },
      { id: "Code", name: "Code", basedOn: "Normal", quickFormat: true,
        run: { font: F.monoFont, size: F.code.size },
        paragraph: { alignment: AlignmentType.LEFT, indent: { firstLine: 0, left: 113 }, spacing: { before: 0, after: 0, line: 240 },
          shading: { type: ShadingType.CLEAR, color: "auto", fill: F.code.fill } } },
      { id: "Quote", name: "Quote", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { italics: true }, paragraph: { indent: { left: 567, right: 567, firstLine: 0 } } },
      { id: "Header", name: "header", basedOn: "Normal",
        run: { size: F.header.size, allCaps: true },
        paragraph: { alignment: AlignmentType.CENTER, indent: { firstLine: 0 }, spacing: { after: 0, line: 240 } } },
      { id: "Footer", name: "footer", basedOn: "Normal",
        run: { size: F.footer.size },
        paragraph: { alignment: AlignmentType.CENTER, indent: { firstLine: 0 }, spacing: { after: 0, line: 240 } } },
    ],
  };
}

// --------------------------------------------------------------------- CLI
async function main() {
  const args = process.argv.slice(2);
  if (!args.length || args.includes("-h") || args.includes("--help")) {
    console.log("usage: node build_report.mjs <report.md> [-o out.docx|out.dotx]");
    process.exit(args.length ? 0 : 1);
  }
  const input = args[0];
  const oi = args.indexOf("-o");
  const out = oi >= 0 ? args[oi + 1] : input.replace(/\.md$/i, "") + ".docx";

  let buf = await Packer.toBuffer(buildDocument(input));
  if (out.toLowerCase().endsWith(".dotx")) {
    const zip = await JSZip.loadAsync(buf);
    const ct = await zip.file("[Content_Types].xml").async("string");
    zip.file("[Content_Types].xml", ct.replace(
      "application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml",
      "application/vnd.openxmlformats-officedocument.wordprocessingml.template.main+xml"));
    buf = await zip.generateAsync({ type: "nodebuffer", compression: "DEFLATE" });
  }
  fs.writeFileSync(out, buf);
  console.log(`wrote ${out}`);
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch((e) => { console.error(e.message || e); process.exit(1); });
}
