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
- Volumeni konstantne gustoće (`constant_medium`): dim i magla

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
├── src/              <- main.cc, animation.cc i svi .h fajlovi (scenes.h: finalna scena)
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
- Frejm se renderuje na **svim jezgrima procesora** (redovi slike se dijele nitima).
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
./build/Release/animation                                    # 72 frejma, 400x400, 50 spp

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
| `--spp S` | uzorci po pikselu | 50 |
| `--depth D` | maksimalna dubina zrake | 8 |
| `--fps F` | brzina reprodukcije u prozoru | 24 |
| `--threads T` | broj niti (0 = sva jezgra) | 0 |
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

Procjena trajanja za 72 frejma (Apple M5, sva jezgra; preračunato iz probnog rendera od ~0,6 s po frejmu za 200×200 i 20 spp):

| Komanda | Po frejmu | Ukupno |
|---|---|---|
| `--width 400 --spp 50` (podrazumijevano) | ~6 s | ~7 min |
| `--width 400 --spp 200 --depth 10` | ~25 s | ~30 min |
| `--width 400 --spp 500 --depth 10` | ~1 min | ~1,2 h |
| `--width 600 --spp 500 --depth 10` | ~2,3 min | ~2,7 h |

Savjeti:

- **Probaj kvalitet na jednom frejmu** prije dugog rendera: `--frames 1` snimi samo `frame_0000.png`.
- **Manje frejmova skraćuje vrijeme:** `--frames 48 --fps 16` daje isti krug kamere za 2/3 vremena, ali je kretanje malo manje glatko.
- **Dugi render bez prozora:** dodaj `--no-window`. Frejmovi se snimaju jedan po jedan, pa se prekidom ne gube već gotovi.

**Napomena:** samo `animation` koristi više niti i zaseban generator slučajnih brojeva po niti (`RTW_MULTITHREADED` u `rtweekend.h`). `raytracer` je i dalje sekvencijalan i radi isto kao prije, pa mjerenja iz izvještaja ostaju uporediva.

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
- `final_scene(400, 250, 4)`: brza testna verzija (loš kvalitet slike)

Što su veći `samples_per_pixel` i `max_depth`, slika je kvalitetnija, ali je renderovanje sporije.

**Napomena 1:** scene 3 i 9 koriste `images/earthmap.jpg`. Datoteka mora biti dostupna pri pokretanju, inače se tekstura neće učitati.

**Napomena 2:** scena 1 je brskorisna jer je motion blur izbačen iz koda
