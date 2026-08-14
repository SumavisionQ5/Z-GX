# AVANCE GRANDE: QuickLoad destraba juegos negros (deadlock del BIOS)

## QUE PASO
Activar el QuickLoad (yabause.c:219 usequickload=1, commit 8f61faf) DESTRABA
gran cantidad de juegos que se quedaban negros tras el logo SEGA.
El QuickLoad hace HLE boot (carga directa del juego saltando la EJECUCION del BIOS),
por lo que esquiva el deadlock master-slave que ocurria DURANTE el arranque del BIOS.

## CLAVE: la instrumentacion lo tapaba
Con la captura de debug pesada en sh2_GetPCAddr (fopen/fwrite en caliente) el timing
se desincronizaba y los juegos seguian negros. Al QUITAR esa instrumentacion, el
QuickLoad funciona y los juegos arrancan. Leccion: NO poner debug pesado en rutas
calientes (sh2_GetPCAddr, StartSlave) - altera el timing master-slave.

## JUEGOS QUE AHORA FUNCIONAN (antes negros, un mes+ de intentos en Cotton 2)
- Cotton 2 (PERFECTO - llevaba un mes)
- Magic Knight Rayearth
- Clockwork Knight 2
- Gekirindan (perfecto)
- Groove on Fight (perfecto)
- Shippuu Mahou Daisakusen (perfecto)

## JUEGOS QUE AVANZARON (antes negro total, ahora arrancan/muestran)
- Capcom Generation (llega a "now loading")
- Dynamite Deka (gameplay con fallos graficos: agrandado, faltan capas)
- Galaxy Fight (llega a start; freeze negro 60fps al dar start)
- Highway 2000 (corre, se escucha, queda en loading)
- Shienryu (muestra intro completa, luego freeze negro 60fps)
- Sonic 3D Blast (titulo, luego negro 60fps)
- Sonic Jam (funciona, fallos graficos)

## SIGUEN NEGROS (para investigar)
- Cotton Boomerang (muestra lineas, freeze)
- X-Men vs Street Fighter, X-Men CotA
- Street Fighter Zero 3, Marvel Super Heroes vs SF
- (mas de 1000 juegos, muchos sin probar aun)

## PATRON DE LO QUE QUEDA
Varios llegan al gameplay/titulo y hacen "freeze negro con FPS 60" = el deadlock
master-slave persiste en un 2do punto (durante gameplay, no en el BIOS). El QuickLoad
esquivo el 1er deadlock (BIOS); queda el 2do (gameplay) - sigue siendo el bug
arquitectural de sincronizacion master-slave (ver NOTAS_BUG_NEGRO.md).

## NOTA
selectedcart quedo en 7 (original). El QuickLoad NO rompe juegos que ya andaban.
Fallos graficos (Dynamite Deka, Sonic Jam) son tema aparte (VDP/capas), no el negro.
