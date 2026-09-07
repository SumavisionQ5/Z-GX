# ESTADO ACTUAL DEL PROYECTO (para retomar rapido)

## ENTORNO (RESUELTO)
- Compila con: export DEVKITPPC=/opt/devkitpro/devkitPPC (GCC 15.2.0) + libogc2.
- IMPORTANTE: setear DEVKITPPC al nuevo ANTES de compilar (el default puede ser viejo).
- make 2>&1 | grep -iE "error:|output" | tail
- cp seta-gx.dol /e/apps/ZGX/boot.dol
- CUIDADO con GCC15: es agresivo con undefined behavior. Cambios en punteros/shifts/
  QuickLoad pueden romper cosas (el cast en QuickLoad rompio la lista de juegos).
  Probar cambios de a poco.

## LO QUE FUNCIONA (confirmado con GCC15)
- Emulador arranca, menu, lista de juegos OK.
- BIOS Hi-Saturn 1.03 RF + QuickLoad: destraba muchos negros (Cotton 2, Gekirindan,
  Groove on Fight, Shippuu, Magic Knight Rayearth, Clockwork Knight 2 - perfectos).
- Guardado de partidas (backup RAM) SD y USB.
- Audio OK. FMV andan (con tirones, tema aparte).

## FRENTES PENDIENTES (elegir en la proxima sesion)
1. DEADLOCK MASTER-SLAVE (VC2, Galaxy Fight, Sonic 3D freeze negro 60fps).
   - Es el bug ARQUITECTURAL dificil. Ver NOTAS/DEADLOCK_MASTER_SLAVE.md
   - YA descartado: Input Capture, chunking desde bucle, idle-loop timing.
   - Causa: granularidad del dynarec (bloques completos). Fix real = concurrent bus
     access a nivel dynarec (compiler.c). Necesita SESION DEDICADA.
   - Caso limpio: Virtua Cop 2 (no necesita cartucho).
2. FALLOS GRAFICOS: Dynamite Deka (agrandado, faltan capas), Sonic Jam (VDP/capas).
3. FMV a tirones: es la presentacion del bitmap, NO el CD timing (ya descartado).
   Ver NOTAS/BUG_VIDEO_BITMAP.md. Investigar SGX_Vdp2DrawBitmap.
4. DSI Batsugun/Hyper Duel/Elevator (misma raiz que el deadlock).
5. Mas juegos negros por probar (>1000 juegos).

## NOTA
No subir a GitHub por ahora (tema de terceros atribuyendose el trabajo).
Todo commiteado LOCAL (seguro en disco). 8 commits locales sin subir.
