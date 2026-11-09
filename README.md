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
- BVH se može uključiti ili isključiti pri pokretanju, bez ponovnog kompajliranja

**Ostalo**
- Izlaz u **PNG** formatu (`stb_image_write`), slika se automatski otvara
- Ispis vremena izgradnje scene i vremena renderovanja
- Build sistem: **CMake**
- 9 ugrađenih scena (od jednostavnih sfera do Cornell boxa i finalne scene)

## Struktura projekta

```
Project/
├── CMakeLists.txt
├── src/              <- main.cc i svi .h fajlovi
│   └── external/     <- stb_image.h, stb_image_write.h
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

Program se pokreće iz foldera `Project/`:

```
# Windows
build\Release\raytracer

# Linux / macOS
./build/Release/raytracer
```

Program zatim:

1. ispiše listu scena i pita koju da renderuje,
2. za scene koje koriste BVH (0, 1 i 9) pita `Koristiti BVH? (d/n)`; Enter znači „da“,
3. renderuje sliku (u terminalu se vidi `Scanlines remaining`),
4. snimi sliku u `output/` kao PNG i **automatski je otvori**,
5. ispiše vrijeme izgradnje scene (uključujući BVH stablo) i vrijeme renderovanja.

Primjer ispisa:

```
----------------------------------------------
Scena:            1 - bouncing_spheres
BVH:              da
Izgradnja scene:  0.000 s
Renderovanje:     4.676 s
Ukupno:           4.676 s
Slika:            output/1_bouncing_spheres_bvh.png
----------------------------------------------
```

Folder `output/` se pravi automatski. Ime slike sadrži broj scene i informaciju o BVH-u
(`_bvh` ili `_bez_bvh`), tako da se slike s BVH-om i bez njega ne prepisuju.

### Pokretanje s argumentima (bez pitanja)

Za mjerenja i skripte, izbor se može zadati u komandnoj liniji. Tada program ništa ne pita:

```
raytracer [scena 0-9] [--bvh | --no-bvh] [--no-open]
```

| Argument    | Značenje                                      |
|-------------|-----------------------------------------------|
| `0`–`9`     | broj scene (zadano: 0)                        |
| `--bvh`     | koristi BVH (zadano)                          |
| `--no-bvh`  | ne koristi BVH, svi objekti se testiraju redom |
| `--no-open` | ne otvaraj sliku nakon renderovanja           |

Primjer poređenja s BVH-om i bez njega:

```
./build/Release/raytracer 1 --bvh
./build/Release/raytracer 1 --no-bvh
```

Slike s BVH-om i bez njega su identične do piksela, jer BVH samo ubrzava traženje
presjeka, a ne mijenja rezultat.

## Scene

| # | Scena | BVH |
|---|---|---|
| 0 | `final_scene` brza verzija (400 px, 250 uzoraka, dubina 4) | da |
| 1 | `bouncing_spheres` (oko 480 sfera) | da |
| 2 | `checkered_spheres` | ne |
| 3 | `earth` | ne |
| 4 | `perlin_spheres` | ne |
| 5 | `quads` | ne |
| 6 | `simple_light` | ne |
| 7 | `cornell_box` | ne |
| 8 | `cornell_smoke` | ne |
| 9 | `final_scene` visok kvalitet (800 px, 10000 uzoraka, dubina 40), traje satima | da |

Što su veći `samples_per_pixel` i `max_depth`, slika je kvalitetnija, ali je renderovanje sporije.
Parametri svake scene se mijenjaju u odgovarajućoj funkciji u `src/main.cc`.

**Napomena 1:** scene 3, 0 i 9 koriste `images/earthmap.jpg`. Program se zato pokreće iz
foldera `Project/`, inače se tekstura neće učitati (Zemlja će biti cijan boje).

**Napomena 2:** zamućenje pokreta (motion blur) je izbačeno iz koda, pa su sfere u scenama 1 i 9 statične.
