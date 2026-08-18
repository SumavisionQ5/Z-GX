# RESUMEN DEADLOCK MASTER-SLAVE (para pasarle a Gemini como texto, NO tocar nuestro repo)

## EL PROBLEMA
Juegos que arrancan pero se congelan: freeze pantalla negra + FPS clavados en 60
(el emulador corre, el juego no avanza). Ejemplo: VIRTUA COP 2 se congela justo
despues de la imagen de inicio. Otros: Galaxy Fight, Sonic 3D, Shienryu, Cotton Boomerang.
Es deadlock de sincronizacion entre los 2 SH2 (master y slave).

## CAUSA RAIZ (confirmada via changelog Kronos 2.5.0)
Kronos lo resolvio con "Implement SH2/SCU concurrent access on CPU-bus".
Es ARQUITECTURAL: acceso concurrente de los 2 SH2 + SCU al bus de memoria compartida.
NO es parche del handshake.

## POR QUE PASA
Master corre N ciclos, LUEGO slave corre N ciclos (alternados, no paralelo real).
Si el master escribe un flag que el slave espera, el slave no lo ve hasta su turno;
si el master ya se durmio esperando al slave, deadlock. Bucle: yabause.c YabauseEmulate.

## YA PROBADO Y DESCARTADO (no repetir):
1. Handshake FRT Input Capture (MINIT/SINIT) en ambos sentidos: NO arregla VC2.
2. Sync por ciclos exactos + salto de SLEEP (0x001B) tipo Kronos en Input Capture
   (igual que Kronos sh2core.c:2618/2657): NO arregla VC2. Conservado (no rompe nada).
3. Slave estilo HLE (VBR=0x06000400, PC de 0x06000250): ROMPE Dodonpachi/Blast Wind.
CONCLUSION: el deadlock NO pasa por Input Capture. Es otro mecanismo (polling de
memoria compartida / SCU DMA / interrupciones entre CPUs).

## EL FIX REAL (lo que falta)
Replicar "SH2/SCU concurrent access on CPU-bus" de Kronos: sincronizar los 2 SH2 en
CADA acceso a memoria compartida (HWRAM 0x06000000, LWRAM 0x00200000), no solo en
Input Capture. Cuando un SH2 lee/escribe direccion compartida, el otro se pone al dia.
Refs Kronos: sh2core.c (patron syncCycle = CurrentSH2->cycles - otroSH2->cycles),
memory.c:769+ (CPU bus). Buscar como sincroniza en accesos a memoria en general.

## REGLAS
- NO debug pesado (fopen/fwrite) en rutas calientes (sh2_GetPCAddr, sh2_Exec, Input
  Capture, StartSlave): desincroniza el timing.
- Probar que no se rompan: audio, Cotton 2, Daytona, Gekirindan.
- BIOS: Hi-Saturn 1.03 RF + QuickLoad activo.
- Base: Yabause/Seta GX con dynarec PowerPC. Estructura SH2: pc, cycles, r[16], flags.

=== HALLAZGO CLAVE (experimento del usuario: Yabause libretro en Wii) ===
El usuario compilo Yabause de RetroArch en Wii (interpretador, ~10fps) y SF Zero 3
SI CORRE ahi (en ZGX no). Comparamos el codigo (C:\Users\hotgl\saturn-wii\yabause):
- Su bucle master-slave es IGUAL al nuestro (MSH2 luego SSH2, alternados).
- Su InputCapture es MAS SIMPLE que el nuestro (solo setea flag + interrupt, sin
  ejecutar el otro SH2). AUN ASI corre SF Zero 3.
- Diferencia real: usa INTERPRETADOR (granularidad de 1 instruccion).

CONCLUSION DEFINITIVA: el deadlock es INHERENTE al dynarec de BLOQUES.
- Interpretador: ejecuta instruccion x instruccion -> cuando el master escribe un flag,
  el slave lo ve casi de inmediato -> se sincronizan naturalmente -> SF Zero 3 anda.
- Nuestro dynarec: ejecuta BLOQUES enteros -> el slave no ve el flag hasta terminar su
  bloque -> los 2 se esperan -> DEADLOCK.
Por eso Kronos lo arreglo con "concurrent bus access" = recuperar sincronizacion fina
en los accesos a memoria compartida.

=== OPCIONES DE FIX (para la sesion dedicada) ===
A) Bloques mas cortos cuando el slave esta activo: cuando IsSSH2Running, ejecutar los
   SH2 en trozos chicos (ej 32-64 ciclos) en vez de sh2cycles completos, alternando
   mas seguido. Recupera parte de la granularidad. Costo: mas lento pero puede destrabar.
   PROBAR PRIMERO (es el cambio mas simple, en YabauseEmulate).
B) Concurrent bus access tipo Kronos: sincronizar en cada acceso a HWRAM/LWRAM
   compartida. Mas complejo pero es el fix "correcto".
C) Detectar el patron de spin-loop (bf a si mismo) y forzar sync ahi.

Opcion A es la mas facil de probar y puede dar resultado rapido. Referencia: el bucle
esta en src/yabause.c YabauseEmulate (~400), variable sh2cycles.

=== DESCARTADO: Opcion A (chunking desde el bucle) NO funciona ===
Probamos ejecutar master/slave en trozos chicos (8, 32 ciclos) alternando desde
YabauseEmulate. RESULTADO: no destraba (VC2 y 5-6 juegos igual).
RAZON (confirmada en el assembly jit_code.s): el dynarec ejecuta BLOQUES COMPLETOS
y no se detiene a mitad. sh2_DrcExec arranca con cycles = -pedidos y cada bloque SUMA
sus ciclos; sale cuando cycles>=0. Pedir 32 ciclos pero un bloque dura 50 -> ejecuta
los 50 completos igual. Por eso el chunking desde afuera NO logra granularidad fina.
CONCLUSION: el fix DEBE ser a nivel del dynarec (concurrent bus access, Opcion B).
Nota: SF Zero 3 es MAL caso de prueba - REQUIERE cartucho RAM 4MB (2 problemas juntos).
Usar Virtua Cop 2 (no necesita cartucho) como caso limpio del deadlock.
