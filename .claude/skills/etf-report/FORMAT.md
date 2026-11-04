# Format izvještaja — specifikacija

Izmjereno iz `docs/checkpoint 1/BVHRayTracingIzvjestaj.pdf`. Sve vrijednosti su
definisane u objektu `F` u `scripts/build_report.mjs`, a u Word šablonu kao
imenovani stilovi (Normal, Heading 1–3, Caption, Reference, …).

## Stranica
| Svojstvo | Vrijednost |
|---|---|
| Format | A4, uspravno |
| Margine | gore 2,0 cm · dolje 2,0 cm · lijevo 2,75 cm · desno 2,43 cm |
| Zaglavlje / podnožje od ruba | 0,7 cm / 0,9 cm |
| Font | Times New Roman (kod: Consolas) |
| Numeracija strana | kontinuirana od naslovne; na naslovnoj se ne prikazuje |

## Naslovna strana (centrirano, bez zaglavlja/podnožja)
| Element | Stil |
|---|---|
| Logotipi UNSA + ETF | u jednom redu, visina ≈ 2,95 cm |
| UNIVERZITET U SARAJEVU | 13 pt, podebljano, verzal |
| ELEKTROTEHNIČKI FAKULTET | 12 pt, verzal |
| Naslov (npr. „P16: BVH RAY TRACER“) | 19 pt, podebljano, verzal |
| Vrsta dokumenta („Istraživački izvještaj“) | 14 pt |
| Faza („CHECKPOINT 1“) | 13 pt, podebljano, verzal |
| Predmet / Mentor | 11 pt |
| „Autori“ | 12 pt, podebljano |
| Autori: `Ime Prezime  -  broj indeksa` | 11 pt, po jedan red |
| Mjesto i datum („Sarajevo, novembar 2026.“) | 11 pt, pri dnu |

## Tijelo dokumenta
| Element | Stil |
|---|---|
| Zaglavlje | `UNIVERZITET U SARAJEVU  \|  ELEKTROTEHNIČKI FAKULTET`, 8 pt, verzal, centrirano |
| Podnožje | `Naslov  \|  Faza  \|  broj strane`, 8,5 pt (broj 11 pt), centrirano |
| Tekst (Normal) | 12 pt, obostrano poravnat, prored 1,15, razmak poslije 6 pt, uvlačenje prvog reda 0,5 cm |
| Sažetak | kao Normal, bez uvlačenja; naslov „Sažetak“ bez broja |
| Naslov 1 | 13,5 pt podebljano, `1. Naslov`, razmak 12 pt prije / 10 pt poslije |
| Naslov 2 | 12 pt podebljano, `1.1. Podnaslov`, 8 pt prije / 3 pt poslije |
| Naslov 3 | 12 pt podebljano kurziv, `1.1.1.` |
| Slika | centrirana, najviše 15,9 cm široka |
| Opis slike | ispod slike: `Slika N. Opis.` 9 pt kurziv, centrirano |
| Tabela | puna širina, tanki sivi okviri (#BFBFBF), 10 pt, zaglavlje podebljano |
| Opis tabele (opcionalno) | iznad tabele: `Tabela N. Opis.` 9 pt kurziv |
| Liste | 12 pt, uvučene 1 cm |
| Literatura | posljednja numerisana sekcija; `[n] Prezime, I. (godina). Naslov. Izvor. doi/URL.` 9 pt, obostrano, viseće uvlačenje |
| Citiranje u tekstu | `[1]`, `[2, 3]` — redoslijed prema listi literature |

## Ručni rad u Wordu
Ako ne koristiš Markdown: otvori `templates/ETF_Izvjestaj_Sablon.dotx` (dvoklik
stvara novi dokument), zamijeni tekst u `[uglastim zagradama]` i za nove
elemente koristi stilove iz okna *Styles*. Ne mijenjaj fontove i razmake
direktno. Numeraciju naslova, slika i tabela tada se piše ručno.
