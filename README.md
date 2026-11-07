# \# BVH Ray Tracer (C++)

# 

# Path tracer u C++-u, rađen prema knjizi \*Ray Tracing: The Next Week\* (Peter Shirley, Trevor David Black, Steve Hollasch). Projekat se fokusira na ubrzanje renderovanja pomoću \*\*BVH\*\* (Bounding Volume Hierarchy) strukture.

# 

# \## Mogućnosti

# 

# \*\*Kamera i uzorkovanje\*\*

# \- Podesiva kamera (pozicija, smjer, vidno polje, omjer stranica)

# \- Antialiasing (više nasumičnih uzoraka po pikselu, `samples\_per\_pixel`)

# \- Defocus blur / dubinska oštrina

# 

# \*\*Materijali\*\*

# \- `lambertian` (difuzni)

# \- `metal` (refleksija, podesivi fuzz)

# \- `dielectric` (staklo, refrakcija)

# \- `diffuse\_light` (izvor svjetlosti)

# \- `isotropic` (raspršenje u volumenima)

# 

# \*\*Teksture\*\*

# \- Jednobojna, šahovska (3D), slikovna (`stb\_image`) i Perlin noise (turbulencija, mramor)

# 

# \*\*Geometrija\*\*

# \- Sfere (uključujući pokretne)

# \- Četverougao (`quad`) i kutije (`box`)

# \- Instance: `translate` i `rotate\_y`

# \- Volumeni konstantne gustoće (`constant\_medium`): dim i magla

# 

# \*\*Ubrzanje\*\*

# \- `aabb` (axis-aligned bounding box, slab metoda)

# \- `bvh\_node` (BVH stablo, podjela po najdužoj osi)

# 

# \*\*Ostalo\*\*

# \- Izlaz u \*\*PPM\*\* formatu

# \- Build sistem: \*\*CMake\*\*

# \- 9 ugrađenih scena (od jednostavnih sfera do Cornell boxa i finalne scene)

# 

# \## Struktura projekta

# 

# ```

# Project/

# ├── CMakeLists.txt

# ├── src/              <- main.cc i svi .h fajlovi

# │   └── external/     <- stb\_image.h

# ├── images/           <- earthmap.jpg (tekstura)

# └── output/           <- renderovane slike

# ```

# 

# \## Build

# 

# Sve komande se pokreću iz foldera `Project/`.

# 

# \*\*Windows (Visual Studio / MSVC):\*\*

# 

# ```

# cmake -B build

# cmake --build build --config Release

# ```

# 

# \*\*Linux / macOS:\*\*

# 

# ```

# cmake -B build/Release -DCMAKE\_BUILD\_TYPE=Release

# cmake --build build/Release

# ```

# 

# `cmake -B ...` je potrebno ponoviti samo kad se mijenja `CMakeLists.txt`. Nakon izmjene koda dovoljna je druga komanda.

# 

# Preporučuje se \*\*Release\*\* build, koji je višestruko brži od Debug builda.

# 

# \## Pokretanje

# 

# Prvo napravi folder za izlaz (samo jednom):

# 

# ```

# mkdir output

# ```

# 

# Pokretanje (slika se preusmjerava u PPM datoteku):

# 

# \*\*DIGRESIJA:\*\* za sljedeću komandu korisiti \*\*cmd\*\* umjesto PowerShell-a zbog ">" koji se drugačije čita u PowerShell-u

# 

# ```

# \# Windows

# build\\Release\\raytracer > output\\image.ppm

# 

# \# Linux / macOS

# ./build/Release/raytracer > output/image.ppm

# ```

# 

# Tokom renderovanja se u terminalu ispisuje preostali broj redova (`Scanlines remaining`).

# 

# PPM se može otvoriti npr. u \[ImageMagick](https://imagemagick.org/), GIMP-u, IrfanView-u ili online PPM vieweru.

# 

# \## Odabir scene

# 

# U `src/main.cc`, u funkciji `main()`, promijeni broj u `switch`:

# 

# | # | Scena |

# |---|---|

# | 1 | `bouncing\_spheres` |

# | 2 | `checkered\_spheres` |

# | 3 | `earth` |

# | 4 | `perlin\_spheres` |

# | 5 | `quads` |

# | 6 | `simple\_light` |

# | 7 | `cornell\_box` |

# | 8 | `cornell\_smoke` |

# | 9 | `final\_scene` (sve mogućnosti zajedno) |

# 

# Za finalnu scenu parametri su `final\_scene(širina, uzorci\_po\_pikselu, max\_dubina)`:

# 

# \- `final\_scene(800, 10000, 40)`: visok kvalitet (traje vrlo dugo)

# \- `final\_scene(400, 250, 4)`: brza testna verzija (loš kvalitet slike)

# 

# Što su veći `samples\_per\_pixel` i `max\_depth`, slika je kvalitetnija, ali je renderovanje sporije.

# 

# \*\*Napomena 1:\*\* scene 3 i 9 koriste `images/earthmap.jpg`. Datoteka mora biti dostupna pri pokretanju, inače se tekstura neće učitati.

# 

# \*\*Napomena 2:\*\* scena 1 je beskorisna jer je motion blur izbačen iz koda



