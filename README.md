# BVH Ray Tracer (C++)

Path tracer u C++-u, rađen prema knjizi *Ray Tracing: The Next Week* (Peter Shirley, Trevor David Black, Steve Hollasch). Projekat se fokusira na ubrzanje renderovanja pomoću **BVH** (Bounding Volume Hierarchy) strukture.

## Mogućnosti

**Kamera i uzorkovanje**
- Podesiva kamera (pozicija, smjer, vidno polje, omjer stranica)
- Antialiasing (više nasumičnih uzoraka po pikselu, `samples_per_pixel`)
- Defocus blur / dubinska oštrina

**Materijali**
- `lambertian` (difuzni)
- `metal` (refleksija, podesivi fuzz)
- `dielectric` (staklo, refrakcija)
- `diffuse_light` (izvor svjetlosti)
- `isotropic` (raspršenje u volumenima)

**Teksture**
- Jednobojna, šahovska (3D), slikovna (`stb_image`) i Perlin noise (turbulencija, mramor)

**Geometrija**
- Sfere (uključujući pokretne)
- Četverougao (`quad`) i kutije (`box`)
- Instance: `translate` i `rotate_y`
- Volumeni konstantne gustoće (`constant_medium`): dim i magla (u finalnoj sceni plava staklena sfera s maglom unutra; velika sfera magle oko cijele scene iz knjige je izbačena, vidi izvještaj za Checkpoint 3)

**Ubrzanje**
- `aabb` (axis-aligned bounding box, slab metoda)
- `bvh_node` (BVH stablo, podjela po najdužoj osi)

**Ostalo**
- Izlaz u **PPM** formatu
- Build sistem: **CMake**
- 9 ugrađenih scena (od jednostavnih sfera do Cornell boxa i finalne scene)

## Struktura projekta

```
Project/
├── CMakeLists.txt
├── src/              <- main.cc, animation.cc, interactive.cc i .h fajlovi (scenes.h: finalna scena, gl_display.h: GLFW prozor)
│   └── external/     <- stb_image.h
├── images/           <- earthmap.jpg (tekstura)
└── output/           <- renderovane slike
```

## Build

Sve komande se pokreću iz foldera `Project/`.

**Windows (Visual Studio / MSVC):**

```
cmake -B build
cmake --build build --config Release
```

**Linux / macOS:**

```
cmake -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release
```

`cmake -B ...` je potrebno ponoviti samo kad se mijenja `CMakeLists.txt`. Nakon izmjene koda dovoljna je druga komanda.

Preporučuje se **Release** build, koji je višestruko brži od Debug builda.

## Pokretanje

Prvo napravi folder za izlaz (samo jednom):

```
mkdir output
```

Pokretanje (slika se preusmjerava u PPM datoteku):

**DIGRESIJA:** za sljedeću komandu korisiti **cmd** umjesto PowerShell-a zbog ">" koji se drugačije čita u PowerShell-u

```
# Windows
build\Release\raytracer > output\image.ppm

# Linux / macOS
./build/Release/raytracer > output/image.ppm
```

Tokom renderovanja se u terminalu ispisuje preostali broj redova (`Scanlines remaining`).

PPM se može otvoriti npr. u [ImageMagick](https://imagemagick.org/), GIMP-u, IrfanView-u ili online PPM vieweru.

## Animacija (GLFW)

Pored `raytracer`-a, build pravi i program `animation`, koji snima **finalnu scenu iz više uglova**: kamera se kreće po zatvorenoj putanji oko scene (obilazi je sa svih strana, primiče se i udaljava, penje se i spušta), a za svaku poziciju kamere renderuje se jedan frejm. Kad se frejmovi puste jedan za drugim, izgleda kao da se kamera kreće kroz scenu.

- Scena i BVH se grade **jednom**, a zatim se za svaki frejm pomjeri samo kamera.
- Frejm se po defaultu renderuje na **jednoj niti** (sekvencijalno). Opcijom `--threads` se redovi slike mogu podijeliti na više niti.
- U **GLFW** prozoru se vidi kako se trenutni frejm iscrtava red po red.
- Svaki gotov frejm se snima kao `output/frames/frame_XXXX.png`.
- Kad su svi frejmovi gotovi, prozor ih **pušta u petlji** kao video. Posljednji frejm se nastavlja na prvi, pa petlja nema skoka.

**GLFW:** ako je instaliran na sistemu (npr. `brew install glfw`), CMake ga koristi; ako nije, CMake ga sam preuzme i izgradi pri prvom `cmake -B ...` (potreban je internet). Animacija se može isključiti sa `-DBUILD_ANIMATION=OFF`.

### Pokretanje animacije, korak po korak

**1. Izgradi program** (iz foldera `Project/`). Prvi `cmake -B ...` preuzima GLFW, pa treba internet. Poslije izmjene koda dovoljna je samo druga komanda.

```
# Linux / macOS
cmake -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release

# Windows
cmake -B build
cmake --build build --config Release
```

**2. Pokreni animaciju**, takođe iz foldera `Project/` (inače se ne nađe tekstura `images/earthmap.jpg`):

```
# Linux / macOS
./build/Release/animation --frames 36 --width 200 --spp 20   # brzi pregled (~pola minute)
./build/Release/animation                                    # 72 frejma, 400x400, 1000 spp, dubina 20 (~2 h)

# Windows
build\Release\animation.exe --frames 36 --width 200 --spp 20
build\Release\animation.exe
```

Otvori se prozor u kojem se frejmovi iscrtavaju jedan po jedan. Kad su svi gotovi, animacija se sama pušta u petlji. Frejmovi ostaju snimljeni u `Project/output/frames/`.

Opcije:

| Opcija | Značenje | Podrazumijevano |
|---|---|---|
| `--frames N` | broj frejmova za jedan krug kamere | 72 |
| `--width W` | širina = visina frejma u pikselima | 400 |
| `--spp S` | uzorci po pikselu | 1000 |
| `--depth D` | maksimalna dubina zrake | 20 |
| `--fps F` | brzina reprodukcije u prozoru | 24 |
| `--threads T` | broj niti (0 = sva jezgra) | 1 |
| `--out DIR` | folder za PNG frejmove | `output/frames` |
| `--no-window` | samo renderuje PNG frejmove, bez prozora | — |

Tasteri u prozoru tokom reprodukcije: `SPACE` pauza, `←`/`→` frejm nazad/naprijed, `↑`/`↓` brže/sporije, `ESC` ili `Q` izlaz. Zatvaranje prozora tokom renderovanja prekida rad (već snimljeni frejmovi ostaju).

Od frejmova se može napraviti video pomoću [ffmpeg](https://ffmpeg.org/):

```
ffmpeg -framerate 24 -i output/frames/frame_%04d.png -c:v libx264 -pix_fmt yuv420p output/animation.mp4
```

### Kvalitet animacije

Šum (zrnca na slici) dolazi od premalo uzoraka po pikselu. Finalna scena ima samo jedno malo svjetlo, pa većina zraka ne pogodi svjetlo i slika je zrnasta.

| Opcija | Šta radi | Uticaj na vrijeme |
|---|---|---|
| `--spp` | **najvažnija**: više uzoraka po pikselu daje manje šuma | linearno (2× spp = 2× vrijeme) |
| `--width` | veća rezolucija, oštrija slika | kvadratno (2× širina = 4× vrijeme) |
| `--depth` | više odbijanja, svjetlije staklo i sjene | malo |

Šum opada sa korijenom iz spp: 4× više uzoraka daje upola manje šuma.

Procjena trajanja za 72 frejma (Apple M5, 1 nit, scena bez velike sfere magle; preračunato iz izmjerenih ~110 s za jednu sliku 400×400 sa 1000 spp i dubinom 20):

| Komanda | Po frejmu | Ukupno |
|---|---|---|
| `--width 200 --spp 50` | ~1,4 s | ~2 min |
| `--width 400 --spp 50` | ~5,5 s | ~7 min |
| `--width 400 --spp 250` | ~28 s | ~33 min |
| podrazumijevano (`--width 400 --spp 1000 --depth 20`) | ~110 s | ~2,2 h |

Savjeti:

- **Probaj kvalitet na jednom frejmu** prije dugog rendera: `--frames 1` snimi samo `frame_0000.png`.
- **Manje frejmova skraćuje vrijeme:** `--frames 48 --fps 16` daje isti krug kamere za 2/3 vremena, ali je kretanje malo manje glatko.
- **Dugi render bez prozora:** dodaj `--no-window`. Frejmovi se snimaju jedan po jedan, pa se prekidom ne gube već gotovi.

**Napomena:** samo `animation` može koristiti više niti (uz `--threads`) i ima zaseban generator slučajnih brojeva po niti (`RTW_MULTITHREADED` u `rtweekend.h`). `raytracer` je i dalje sekvencijalan i radi isto kao prije, pa mjerenja iz izvještaja ostaju uporediva.

## Interaktivni pregled s vremenskom akumulacijom (GLFW)

Program `interactive` prikazuje finalnu scenu u prozoru u kojem se kamera pomjera mišem ili tastaturom. Koristi **vremensku (progresivnu) akumulaciju** (temporal accumulation): svaki prolaz renderuje samo **1 uzorak po pikselu** i dodaje ga u bafer. Prozor prikazuje prosjek svih dosadašnjih prolaza, pa se slika pojavi nakon prvog prolaza i postepeno postaje čistija (šum opada kao 1/√N). Kad se kamera pomjeri, stari uzorci više ne odgovaraju pikselima, pa se bafer briše i akumulacija kreće ispočetka.

Program je **sekvencijalan**: sve radi na jednoj niti, kao `raytracer`, i koristi isti generator slučajnih brojeva (`std::rand()`), pa je scena ista kao na slikama iz izvještaja. Prozor se osvježava između redova slike, pa reaguje i dok traje prolaz.

Build je isti kao za animaciju (`cmake --build build/Release`). Pokretanje iz foldera `Project/`:

```
# Linux / macOS
./build/Release/interactive

# Windows
build\Release\interactive.exe
```

| Kontrola | Akcija |
|---|---|
| prevlačenje mišem / strelice | kamera kruži oko scene |
| `W` / `S` ili scroll | primicanje / udaljavanje |
| `R` | povratak na početni pogled |
| `P` | snimi trenutnu sliku u `output/accumulated_<spp>spp_XX.png` |
| `ESC` ili `Q` | izlaz |

U naslovu prozora piše koliko je uzoraka akumulirano i koliko je vremena prošlo od zadnjeg pomjeranja kamere. Kad se dostigne `--max-spp`, renderovanje stane i program čeka sljedeći pokret.

| Opcija | Značenje | Podrazumijevano |
|---|---|---|
| `--width W` | širina = visina slike | 400 |
| `--depth D` | maksimalna dubina zrake | 20 |
| `--max-spp N` | zaustavi akumulaciju na N uzoraka | 1000 |
| `--clamp C` | ograniči svjetlinu svakog uzorka na C (vidi ispod) | 0 (isključeno) |
| `--out DIR` | folder za snimljene slike | `output` |
| `--no-window` | bez prozora: akumulira do `--max-spp` s početnog pogleda, ispiše vremena, snimi PNG i izađe | — |

**Clamp (ograničavanje svijetlih uzoraka):** rijetki, vrlo svijetli uzorci (npr. zraka koja se rasprši u magli i zatim pogodi malo svjetlo) prave svijetle tačkice koje se dugo ne usrednje. `--clamp C` svaki uzorak čija je najveća komponenta boje veća od C proporcionalno umanji na C. Šum se smanji, ali se gubi dio svjetlosti, pa je slika tamnija. Mjerenje na 300×300 i 300 spp (urađeno dok je scena još imala veliku sferu magle):

| `--clamp` | šum u tamnoj pozadini | šum na podu | prosječna svjetlina |
|---|---|---|---|
| 0 (isključeno) | 24,0 | 14,8 | 68,3 |
| 10 | 24,0 | 14,8 | 68,3 (nema efekta, svjetlo ima jačinu 7) |
| 3 | 16,3 | 10,9 | 54,7 |
| 1 | 9,7 | 6,3 | 37,8 |

`--clamp 3` smanjuje šum za oko 30 %, ali je slika oko 20 % tamnija. Najveći dio šuma u ovoj sceni ne dolazi od pojedinačnih tačkica nego od malog broja uzoraka koji pogode svjetlo, pa ga clamp ne može ukloniti bez potamnjenja. Za to bi trebalo uzorkovanje svjetla (light sampling).

Za mjerenje se koristi `--no-window`. Primjer s istim parametrima kao brza verzija scene 9 u `raytracer`-u:

```
./build/Release/interactive --no-window --width 400 --depth 4 --max-spp 250
```

Na Apple M5 (finalna scena bez velike sfere magle) prva slika (1 spp) se pojavi za oko 0,08 s, 50 spp nakon oko 3,2 s, a 250 spp nakon oko 16,2 s. `raytracer` za istu sliku (`final_scene(400, 250, 4)`) treba oko 13,7 s, ali prvu sliku daje tek na kraju. Akumulacija je ukupno nešto sporija jer se nakon svakog prolaza ponovo računa prikazana slika (prosjek i gama korekcija za svaki piksel).

## Odabir scene

U `src/main.cc`, u funkciji `main()`, promijeni broj u `switch`:

| # | Scena |
|---|---|
| 1 | `bouncing_spheres` |
| 2 | `checkered_spheres` |
| 3 | `earth` |
| 4 | `perlin_spheres` |
| 5 | `quads` |
| 6 | `simple_light` |
| 7 | `cornell_box` |
| 8 | `cornell_smoke` |
| 9 | `final_scene` (sve mogućnosti zajedno) |

Za finalnu scenu parametri su `final_scene(širina, uzorci_po_pikselu, max_dubina)`:

- `final_scene(800, 10000, 40)`: visok kvalitet (traje vrlo dugo)
- `final_scene(400, 1000, 20)`: podrazumijevano (`default` u `main.cc`), dobar kvalitet, oko 110 s na Apple M5
- `final_scene(400, 250, 4)`: brza testna verzija (lošiji kvalitet slike), oko 14 s; s njom su rađena mjerenja u izvještajima

Što su veći `samples_per_pixel` i `max_depth`, slika je kvalitetnija, ali je renderovanje sporije.

**Napomena 1:** scene 3 i 9 koriste `images/earthmap.jpg`. Datoteka mora biti dostupna pri pokretanju, inače se tekstura neće učitati.

**Napomena 2:** scena 1 je brskorisna jer je motion blur izbačen iz koda
