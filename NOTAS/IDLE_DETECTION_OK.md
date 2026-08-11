# IDLE DETECTION EN DYNAREC - MEJORA DE RENDIMIENTO (FUNCIONA)

## RESULTADO
- Batsugun: 49 -> 60fps clavados
- Battle Garegga: 49 -> 60fps clavados
- Sega Rally: 48 -> 54/57fps
- Daytona y demas: sin cambios, no se rompio nada.
Tecnica inspirada en PicoDrive (idle/poll loop detection).

## QUE SE HIZO (src/sh2/drc/compiler.c)
1. Deteccion en _jit_GenIFCBlock: un bloque es IDLE si:
   - termina en branch condicional BF/BT
   - el desplazamiento es hacia ATRAS (idle_disp >= 0x80, bit de signo)
   - NO escribe a memoria (has_mem_write = 0, se checan todos los stores IFC_MOVxS)
   Variables: has_mem_write, idle_disp, idle_is_branch (~linea 1167).
   Deteccion de stores y branch antes de ifc_array (~1389).
   Campo block_data.is_idle (struct ~1150), seteado ~1405, cuenta drc_idle_count.
2. Salto de ciclos en macro SH2JIT_BF (~922): si block_data.is_idle y el branch
   se toma (sigue en loop), en vez de girar SALE del bloque devolviendo 128 ciclos
   (PPCC_ADDI(3,0,128) + jit_endblock_test). Asi el scheduler avanza al proximo
   evento en vez de quemar el loop miles de veces.

## PENDIENTE / MEJORAS FUTURAS
- Solo se modifico SH2JIT_BF. Falta SH2JIT_BT (~981) para cubrir mas idle loops
  (los que usan BT en vez de BF). Podria mejorar Sega Rally que quedo en 54/57.
- El valor 128 ciclos es fijo; podria ajustarse al slice real para mas precision.
- Backup en NOTAS/compiler.c.backup y sh2.c.backup por si acaso.
