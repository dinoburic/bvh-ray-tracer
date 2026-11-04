#!/usr/bin/env bash
# Export a .docx to PDF with Microsoft Word (macOS) or LibreOffice (any OS).
# usage: to_pdf.sh report.docx [out.pdf]
set -euo pipefail
in="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
out="${2:-${in%.*}.pdf}"
case "$out" in /*) ;; *) out="$(pwd)/$out" ;; esac
if command -v soffice >/dev/null 2>&1; then
  soffice --headless --convert-to pdf --outdir "$(dirname "$out")" "$in" >/dev/null
  gen="$(dirname "$out")/$(basename "${in%.*}").pdf"
  [ "$gen" = "$out" ] || mv "$gen" "$out"
elif [ -d "/Applications/Microsoft Word.app" ]; then
  # First run: Word may ask to "Grant Access" to the folder - click it.
  osascript <<OSA
with timeout of 300 seconds
tell application "Microsoft Word"
  set d to open file name (POSIX file "$in" as text)
  save as d file name (POSIX file "$out" as text) file format format PDF
  close (first document whose full name is "$in") saving no
end tell
end timeout
OSA
else
  echo "Neither LibreOffice (soffice) nor Microsoft Word found; open the .docx and export to PDF manually." >&2
  exit 1
fi
echo "wrote $out"
