# BUG ISI: Batsugun, Elevator Action 2, Hyper Duel (y quiza mas)

## SINTOMA
DSI/ISI al cargar estos juegos directo. A veces andan tras cargar otro juego
(intermitente, 2-3 intentos). Tambien ISI al salir. Persiste apagando la Wii.
Batsugun SIEMPRE fue problematico. "Hubo un tiempo que cargaba" (alguna version vieja).

## DATOS DEL CRASH (claves)
- Batsugun: ISI, SRR0 = 0x20002000, DAR = 0
- Elevator Action: ISI, SRR0 = 0x30003000, DAR = 0
- Hyper Duel: Program exception, SRR0 = 0x800fc110 (dir real de codigo, distinto caso)
- Al salir: Program exception.

## DIAGNOSTICO
- NO es el idle skip: se desactivo (is_idle=0) y el ISI persiste. Idle skip absuelto.
- ISI = el PowerPC intento EJECUTAR en direccion invalida.
- SRR0 0x20002000 / 0x30003000 son direcciones del SATURN (espacio 0x20000000/0x30000000)
  que el SH2 puso como PC. El dynarec (jit_endblock_test -> HashGet -> _jit_GenBlock)
  intenta compilar codigo en esa direccion basura -> lee/ejecuta basura -> ISI.
- Es un problema de MAPEO/traduccion de esas direcciones para ejecucion, o el juego
  salta ahi esperando algo no emulado. NO es el dynarec en si.
- Hyper Duel (SRR0=800fc110) es dir real -> caso distinto, investigar aparte.

## FLUJO DEL DYNAREC (jit_code.s jit_endblock_test ~71)
Toma PC del SH2 (reg 17) -> HashGet busca bloque -> si no, _jit_GenBlock compila en
esa dir -> salta. Si el PC es 0x20002000 (basura/no ejecutable), compila basura -> ISI.

## PARA INVESTIGAR (sesion dedicada)
- Trazar QUE hace el SH2 antes de saltar a 0x20002000 (que instruccion pone ese PC).
- Ver si 0x20000000/0x30000000 deberian mapear a algo (espejo de WRAM? cartucho?).
- Comparar con emulador que corra estos juegos (Kronos/Yaba Sanshiro/mednafen).
- Posible validacion en el dynarec: si PC cae fuera de rangos ejecutables validos,
  manejar el caso en vez de compilar basura.
- Estos 3 juegos comparten algo (mapper? tecnica?) - buscar patron comun.

## NOTA
No arriesgar el estado actual (rendimiento + guardado funcionan). Este bug es viejo
y acotado a ciertos juegos. Requiere analisis dedicado del mapeo de memoria del SH2.

=== SESION ADICIONAL: mas descartes (el bug persiste) ===
DATO NUEVO CLAVE: el ISI ocurre AL DAR START (transicion titulo->gameplay), NO al
cargar. La intro/titulo se ve bien en los 3 juegos. Crashea al pasar a gameplay.
Mismo sintoma que commit 59b2cbe (Fighting Vipers "ISI al dar Start" - era el dynarec).
PISTA FUERTE: commit 959afd7 documenta "corrupcion de heap con Blast Wind/Batsugun/
Taromaru (crash al salir con punto movil), pendiente de investigar". ES CORRUPCION DE HEAP.

DESCARTADO HOY (no era ninguno):
- Idle skip (is_idle=0 y persiste).
- Tipo de cartucho (selectedcart 7->0, KOF sigue OK, Batsugun sigue ISI).
- Fix NULL en cs0_getPCAddr (CS0 default) - no cambio.
- Tamano buffer dynarec (DRC_CODE_SIZE 1M->2M) - no cambio.
- Reset de paginas de invalidacion (drc_ResetPages agregado en sh2_Init) - CONSERVADO
  (buena practica: limpia drc_code_pages/heat/dirty al cargar juego) pero NO resolvio.
- Log en sh2_GetPCAddr default/CS0: NO se genera archivo -> el PC del SH2 es VALIDO
  (BIOS/LWRAM/HWRAM), el crash NO pasa por ahi. El ISI es en el codigo compilado.
- SCU DMA SI notifica al dynarec (sh2_WriteNotify en scu.c). SH2 DMA tambien (sh2.c:445).

CARACTERISTICAS: intermitente, anda tras cargar otro juego (estado de heap dependiente).
Juegos afectados: Batsugun, Hyper Duel, Elevator Action 2, Blast Wind, Taromaru (y +).

PLAN PROXIMA SESION (dedicada):
- Es corrupcion de heap: instrumentar guardas de memoria alrededor de estructuras del
  dynarec (drc_code, drc_blocks, block_data) y del heap, detectar QUIEN escribe fuera.
- Ver que hacen estos juegos en comun al dar Start (DMA grande? patron de escritura?).
- Comparar: por que "anda tras otro juego" - que estado del heap/dynarec deja el 1er juego.
- El commit 59b2cbe (check buffer lleno) arreglo un caso; buscar otro punto similar.
