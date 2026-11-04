---
title: "P16: BVH Ray Tracer"
doc_type: Istraživački izvještaj
milestone: Checkpoint 1
subject: Paralelni računarski sistemi
mentor: Tarik Hrnjić, MA
authors:
  - { name: Dino Burić, id: 2502/2026 }
  - { name: Ali Fazlić, id: 2590/2026 }
  - { name: Zijad Doglod, id: 2576/2026 }
  - { name: Ajdin Dželo, id: 2527/19540 }
date: Sarajevo, novembar 2026.
footer: "P16: BVH Ray Tracer  |  Checkpoint 1"

abstract: |
  U okviru projekta P16: BVH Ray Tracer istražujemo kako se praćenje zraka (ray tracing)
  može ubrzati kombinovanjem prostorne strukture Bounding Volume Hierarchy (BVH) i
  paralelnog izvršavanja. Pregledom literature nastojali smo razumjeti na koji način se BVH
  stablo gradi, zašto njegov kvalitet utiče na brzinu renderovanja te koje mogućnosti nude
  višejezgreni procesori i SIMD instrukcije. Naša ideja je izraditi jednostavan ray tracer u
  C++-u i porediti sekvencijalnu izvedbu bez BVH-a, sekvencijalnu izvedbu s BVH-om i
  paralelnu verziju s OpenMP-om. Ovaj dokument predstavlja prvu istraživačku fazu
  (Checkpoint 1); implementacija i mjerenja tek slijede.

references:
  - "Waldemarson, G., Doggett, M. (2025). Parallel Axis Split Tasks for Bounding Volume Construction with OpenMP. GRAPP/VISIGRAPP. doi:10.5220/0013317100003912."
  - "Meister, D., Bittner, J. (2017/2018). Parallel Locally-Ordered Clustering for Bounding Volume Hierarchy Construction. IEEE TVCG. dcgi.fel.cvut.cz/projects/ploc/ploc-tvcg.pdf."
  - "Nishimura, S. (2021). Grid-induced bounding volume hierarchy for ray tracing dynamic scenes. The Visual Computer. doi:10.1007/s00371-021-02205-3."
  - "Wald, I., Woop, S., Benthin, C., Johnson, G. S., Ernst, M. (2014). Embree: A Kernel Framework for Efficient CPU Ray Tracing. ACM Transactions on Graphics. embree.org/papers/2014-Siggraph-Embree.pdf."
  - "Wang, C. i sar. (2022). An Improved Ray Tracing Acceleration Algorithm Based on Bounding Volume Hierarchies. arXiv:2208.10008."
  - "Gambetta, G. Computer Graphics from Scratch. gabrielgambetta.com/computer-graphics-from-scratch/."
  - "Shirley, P., Black, T. D., Hollasch, S. Ray Tracing in One Weekend (serijal i kod). github.com/RayTracing/raytracing.github.io."
---

# Uvod u temu

Ray tracing je postupak generisanja slike u kojem se iz virtualne kamere kroz piksele
šalju zrake u trodimenzionalnu scenu. Program pronalazi najbližu površinu koju je pojedina
zraka pogodila i na osnovu materijala, osvjetljenja i položaja izvora svjetlosti određuje boju
piksela. Dodatne zrake omogućavaju računanje sjena i refleksija. Ovakav način renderovanja
može dati uvjerljive rezultate, ali pri većem broju objekata zahtijeva mnogo geometrijskih
proračuna [6, 7].

Najjednostavniji, takozvani naivni pristup, svaku zraku provjerava protiv svakog objekta
u sceni. Ako scena sadrži N geometrijskih primitiva, broj testova po zraci u najgorem slučaju
raste proporcionalno s N. Zbog toga se u praktičnim rendererima koriste akceleracijske
strukture koje izbjegavaju nepotrebne provjere [4].

# BVH i njegova uloga u ray tracingu

BVH (Bounding Volume Hierarchy) je hijerarhijsko stablo koje objekte organizuje u
prostorne grupe. Svaki čvor obuhvata određene primitive pomoću graničnog volumena,
najčešće AABB kutije (Axis-Aligned Bounding Box), čije su stranice paralelne
koordinatnim osama. Korijenski čvor predstavlja cijelu scenu, unutrašnji čvorovi manje
grupe, a listovi konkretne objekte. Ako zraka promaši AABB nekog čvora, mogu se
preskočiti svi objekti u njegovom podstablu. Tako se umjesto velikog broja skupih presjeka s
trouglovima izvodi manji broj jednostavnijih testova sa kutijama [7].

![Pojednostavljen prikaz binarnog BVH stabla.](bvh_stablo.png){width=13.9cm}

BVH nije zaseban algoritam za osvjetljenje, nego način da ray tracer brže pronađe
relevantnu geometriju. Iz literature se vidi da su važne dvije različite stvari: trošak izgradnje
stabla i brzina njegovog obilaska tokom renderovanja. Dobro stablo može zahtijevati više
vremena za pripremu, ali kasnije smanjiti broj testova presjeka. Zato u projektu planiramo
ove dvije faze mjeriti odvojeno [1, 3].

# Ostala literatura

## Gabriel Gambetta: Computer Graphics from Scratch [6]

Knjiga Gabriela Gambette pristupa računarskoj grafici od osnovnih matematičkih ideja i
ne pretpostavlja upotrebu gotovog game enginea. Posebno su korisna poglavlja o generisanju
zraka, presjecima s objektima, osvjetljenju, sjenama i refleksijama.

# Planirani koncept i tehnološki okvir

Planiramo napraviti samostalni C++ program koji će učitati ili definisati jednostavnu 3D
scenu, poslati zrake iz kamere i generisati konačnu statičnu sliku.

| Dio sistema          | Planirane tehnologije i namjena                                                     |
|----------------------|-------------------------------------------------------------------------------------|
| Osnovni program      | C++17 i CMake za prenosivu, optimizovanu izgradnju projekta.                        |
| CPU paralelizacija   | OpenMP za podjelu obrade piksela i zraka na više procesorskih niti.                 |
| Dodatna optimizacija | SIMD (SSE/AVX na odgovarajućem x86 hardveru), uz praćenje lokalnosti memorije.      |
| GPU opcija           | NVIDIA CUDA                                                                         |
| Prikaz i analiza     | PPM ili PNG slika; CSV podaci i Python grafikoni; Git za zajednički rad.            |

OpenMP omogućava direktno mjerenje skaliranja s brojem niti, a analiza SIMD-a i
memorijske lokalnosti povezuje projekat s načinom na koji procesor zaista izvršava kod.
