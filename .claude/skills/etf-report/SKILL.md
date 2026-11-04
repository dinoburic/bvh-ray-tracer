---
name: etf-report
description: Write or reformat a university report (izvještaj, seminarski rad, projektni izvještaj, checkpoint, laboratorijska vježba, essay, research report) in the team's standard Word format — UNSA/ETF cover page, Times New Roman 12 pt justified body, numbered headings, captioned figures and tables, numbered [n] references, header/footer with page numbers. Use whenever someone asks to create, format, restyle, or "make it look like our reports" for a .docx/PDF report for any subject, or mentions the ETF/team report template.
---

# Team report format (ETF / UNSA)

Every report the team hands in uses one format, taken from the
*P16: BVH Ray Tracer – Checkpoint 1* report. Reports are written in **Markdown
with YAML front matter** and built into `.docx`, then `.pdf`, by
`scripts/build_report.mjs`. The script holds all the formatting, so never
format by hand. Write the content and let the builder apply the layout.

Paths below are relative to this skill's directory (the folder holding this
file).

- `FORMAT.md`: the full format spec (margins, fonts, spacing). Read it when someone asks *why* something looks the way it does, or when they edit in Word by hand.
- `templates/izvjestaj_sablon.md`: starting skeleton with every supported field and element.
- `templates/ETF_Izvjestaj_Sablon.dotx` / `.docx`: the same skeleton as a Word template, for people who don't use Markdown.
- `examples/primjer_bvh_checkpoint1.md`: a filled-in real example (cover, abstract, figure, table, references).

## Workflow

1. **Dependencies.** If `scripts/node_modules` is missing, run
   `npm install --prefix <skill-dir>/scripts`. This needs Node ≥ 18.

2. **Cover metadata.** Get these from the user, from an earlier report in the repo,
   or from the document being reformatted: `title`, `doc_type`, `milestone`
   (optional), `subject`, `mentor`, `authors` (name + index number), `date`.
   Ask only for fields you cannot find. Never invent names or index numbers.
   If one is unknown, leave a visible `[placeholder]`.

3. **Write the Markdown** next to where the report belongs (e.g. `docs/<faza>/`).
   Copy `templates/izvjestaj_sablon.md` as the starting point, and put images in
   the same folder or in a subfolder. Conventions:
   - `# Naslov` and `## Podnaslov` are numbered automatically (`1.`, `1.1.`).
     Never type numbers yourself. Add `{-}` to a heading to leave it unnumbered.
   - The abstract goes in front-matter `abstract:`. It is rendered as "Sažetak", unnumbered and without indent.
   - References go in front-matter `references:` as a list. They are rendered as the last
     numbered section "Literatura" as `[1] …`. Cite in the text as `[1]` or `[2, 3]`,
     with numbers in order of the list.
   - Figure: `![Caption text.](file.png){width=14cm}` on its own paragraph. This produces
     "Slika N. Caption text." Use PNG or JPEG. Max width is 15.9 cm.
   - Table: a GFM pipe table, optionally followed by `: Caption` on the next
     paragraph ("Tabela N."). Column widths are computed automatically.
   - Lists `-` / `1.`, `**bold**`, `*italic*`, `` `code` ``, fenced code blocks,
     `> quote`, and `\newpage` for a page break.
   - `language: en` switches the labels to English (Figure, Table, Authors, Abstract, References).
   - Other universities or faculties: set `university`, `faculty`, and `logos:`
     (a list of `{path, width}`, or `[]` for none).

4. **Build and export:**
   ```bash
   node <skill-dir>/scripts/build_report.mjs report.md            # -> report.docx
   <skill-dir>/scripts/to_pdf.sh report.docx                       # -> report.pdf
   ```
   `to_pdf.sh` uses LibreOffice if it is installed, otherwise Microsoft Word on macOS.
   The first time, Word may show a "Grant Access" dialog. Ask the user to click it.

5. **Verify visually.** Render the PDF pages to images (PyMuPDF `page.get_pixmap`,
   or `pdftoppm -r 80`) and look at them. Check for orphaned headings, figures
   that overflow the page, empty pages, and broken numbering. Fix the Markdown, not the output.

## Reformatting an existing document

When given a `.docx`, `.pdf`, Google Doc, or plain text to bring into the team format:
extract the text (`unzip -p file.docx word/document.xml`, PyMuPDF for PDF), then
map its structure onto the Markdown conventions above: headings, paragraphs,
figures (extract the images), tables, and references. Keep the author's wording
**verbatim**. Only formatting changes, unless the user asks for edits. Remove manual
heading numbers ("3.2.") because the builder adds them.

## Writing conventions (when drafting content)

- Use the report's language consistently. For Bosnian, use the ijekavian standard ("istraživanje", "vrijeme")
  and keep established English technical terms in parentheses on first use:
  "praćenje zraka (ray tracing)".
- Use the first-person plural ("planiramo", "istražujemo") and an objective tone, without marketing language.
- Every figure and table is referenced in the text, and every source in `references` is cited.
- Do not change the format constants in `build_report.mjs` for a single report. If the
  team agrees to a change, edit the `F` object and `FORMAT.md` together, then rebuild
  the templates (see `README.md`).
