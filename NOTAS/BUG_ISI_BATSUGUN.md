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
