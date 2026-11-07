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
├── src/              <- main.cc i svi .h fajlovi
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
