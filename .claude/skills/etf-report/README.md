# etf-report — zajednički format izvještaja

Claude Code skill + Word šablon za sve izvještaje tima (bilo koji predmet).

## Instalacija
- **U ovom repozitoriju**: radi automatski. Claude Code učitava `.claude/skills/`.
- **Za sve projekte/predmete**: kopiraj folder u lični skills direktorij:
  ```bash
  cp -R .claude/skills/etf-report ~/.claude/skills/
  npm install --prefix ~/.claude/skills/etf-report/scripts
  ```
- Potrebno: Node.js ≥ 18; za PDF: Microsoft Word (macOS) ili LibreOffice.

## Korištenje
- U Claude Code: „napiši izvještaj za predmet X u našem formatu“, „formatiraj
  ovaj dokument po šablonu“ ili `/etf-report`.
- Ručno:
  ```bash
  cp .claude/skills/etf-report/templates/izvjestaj_sablon.md docs/moj_izvjestaj.md
  node .claude/skills/etf-report/scripts/build_report.mjs docs/moj_izvjestaj.md
  .claude/skills/etf-report/scripts/to_pdf.sh docs/moj_izvjestaj.docx
  ```
- Samo Word: otvori `templates/ETF_Izvjestaj_Sablon.dotx`.

## Izmjena formata
Format je jedan za cijeli tim. Izmjene se dogovaraju i rade na jednom mjestu:
objekat `F` u `scripts/build_report.mjs` + `FORMAT.md`. Nakon izmjene ponovo
generiši šablone:
```bash
cd .claude/skills/etf-report/templates
node ../scripts/build_report.mjs izvjestaj_sablon.md -o ETF_Izvjestaj_Sablon.docx
node ../scripts/build_report.mjs izvjestaj_sablon.md -o ETF_Izvjestaj_Sablon.dotx
../scripts/to_pdf.sh ETF_Izvjestaj_Sablon.docx preview.pdf
```
