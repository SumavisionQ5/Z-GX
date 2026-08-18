# ENTORNO ROTO - DIAGNOSTICO FINAL (tras update de Windows/devkitPro)

## LA SITUACION REAL (confirmada probando los 3 toolchains)
Hay 3 toolchains instalados:
- /c/devkitPPC_r26 (GCC 4.6.3) - MUY viejo
- /c/devkitPPC_r29 (GCC 6.3.0) - intermedio
- /opt/devkitpro/devkitPPC (GCC 15.2.0) - nuevo (lo metio el update)

La libogc2 actual (/opt/devkitpro/libogc2, Jul 11) fue compilada con GCC 15.2.0.
=> SOLO el GCC 15.2.0 la linkea bien. Los viejos (r26/r29) dan:
   - r26: "dwarf version" + "relocation truncated" (incompatible)
   - r29: "unknown floating point ABI 9" + "cannot find _start" (ABI incompatible,
     .elf sale casi vacio 16KB en vez de 1.5MB)

## EL DILEMA
- GCC 15.2.0: linkea la libogc2 PERO rompe el codigo por UB (undefined behavior que
  el compilador viejo toleraba). Sintomas: QuickLoad no carga juegos (van al media
  player), lista de juegos vacia. Hay que cazar los UB uno por uno.
- r26/r29: toleran el codigo PERO no linkean la libogc2 actual (ABI/dwarf).

## FIXES DE CODIGO YA APLICADOS (para compilar con GCC15, conservar):
1. Makefile: -std=gnu99 + PORTLIBS + flags -fno-strict-aliasing -fwrapv
   -fno-aggressive-loop-optimizations (mitigan algunos UB)
2. cdbase.c: quitado ChdInfo duplicado; scandir->opendir/readdir
3. gui.h (osd/ y gui/): guard comun __GUI_SHARED__ para enum+structs compartidas
4. sgx_vdp1.c: case 1 envuelto en llaves (label+declaracion)
5. yabause.c QuickLoad: cast a u32 en shifts de size/addr (UB signed overflow)
   -> OJO: este cast puede haber causado "sin juegos", REVISAR. Backup: yabause.c.antes_ubfix

## LAS 2 SALIDAS REALES (para sesion dedicada / con cabeza fresca):
### SALIDA A: GCC 15.2.0 + cazar los UB
- Toolchain y libogc coinciden (consistente).
- Arreglar UB: QuickLoad (revisar el cast), lista de juegos (games_LoadList), y los
  que aparezcan. Trabajo disperso pero "hacia adelante".

### SALIDA B: recompilar libogc2 con r29 (GCC 6.3.0)
- Tenes el SOURCE en /c/Users/hotgl/libogc2 (tiene Makefile, PKGBUILD, n64.h).
- Compilar esa libogc2 con el r29 -> quedaria libogc2+toolchain consistentes en 6.3.0
  (que TOLERA tu codigo). Es la mas limpia si funciona.
- Comando aprox: cd /c/Users/hotgl/libogc2 && export DEVKITPPC=/c/devkitPPC_r29 && make
  luego instalar en /opt/devkitpro/libogc2 (o apuntar el proyecto ahi).

## RECOMENDACION
Salida B (recompilar libogc2 con r29) es la mas limpia: todo consistente en GCC 6.3.0
que tolera el codigo, sin cazar UB. Requiere compilar libogc2 (tenes el source).
Si falla, Salida A (GCC15 + cazar UB).

## IMPORTANTE
- El CODIGO del emulador esta BIEN. Es SOLO el entorno.
- El .dol que funcionaba (viejo, en la SD antes de hoy) sigue existiendo si se necesita.
- Todo el trabajo (QuickLoad, BIOS Hi-Saturn, sync Kronos) esta commiteado e intacto.
