# PROBLEMA: entorno de compilacion roto tras update de Windows

## SINTOMA
El codigo NO cambio pero dejo de compilar. El compilador (o toolchain) cambio de
comportamiento y ahora rechaza cosas que antes toleraba. El seta-gx.dol quedo viejo
(no se regeneraba) - por eso las pruebas "no cambiaban nada" (era un dol viejo).

## CAUSA RAIZ
Hay DOS devkitPPC instalados:
- /c/devkitPPC_r26 -> VIEJO (GCC 4.6.3). DEVKITPPC apunta aca.
- /opt/devkitpro/devkitPPC -> NUEVO (GCC 15.2.0).
Las librerias (libogc2, portlibs) parecen ser nuevas. Mezcla viejo+nuevo = conflictos.

## FIXES YA APLICADOS (funcionan, conservar):
1. Makefile: agregado -std=gnu99 a CFLAGS (commit 3523171). Sin esto: error "for loop C99".
2. Makefile: agregado PORTLIBS := $(DEVKITPRO)/portlibs/ppc $(DEVKITPRO)/portlibs/wii
   (linea 73). Sin esto: linker "cannot find -lpng16 -lz -lchdr" etc.
3. cdbase.c: quitado typedef ChdInfo duplicado (~1158). Backup: NOTAS/cdbase.c.antes_dupfix
4. cdbase.c: scandir/alphasort -> reemplazado por opendir/readdir (no estan en newlib).

## FIXES PENDIENTES (cascada de redefiniciones):
- gui/gui.h: redefinition de GuiLabel_t, GuiItems_t, GuiAnim, gui_Draw, gui_SetMessage.
  (structs/funciones definidas 2 veces - el compilador nuevo ya no lo tolera).
- Probablemente MAS archivos con el mismo patron.

## SOLUCION REAL (recomendada, hacer con calma - NO en sesion de emulacion):
OPCION 1 (mejor): restaurar un entorno consistente.
  - Si hay backup del devkitPPC r26 + librerias viejas que funcionaban -> restaurar.
  - O reinstalar devkitPPC limpio con pacman (dkp-pacman) y migrar al GCC nuevo
    (arreglar todas las redefiniciones - son varias pero mecanicas).
OPCION 2 (parche): seguir arreglando redefiniciones archivo por archivo con el
  toolchain viejo r26 + PORTLIBS arreglado. Tedioso, puede haber muchas.

## NOTA IMPORTANTE
El codigo del emulador esta BIEN. Es SOLO el entorno. Una vez que compile, todo
el trabajo (QuickLoad, BIOS Hi-Saturn, sync Kronos) sigue intacto.
Las pruebas del deadlock de VC2 de la ultima sesion NO valieron porque el dol era viejo.
