# VC2 DEADLOCK - DIAGNOSTICO FINAL (no volver a medir esto)

## ESTADO EXACTO CUANDO SE TRABA (medido en Wii con overlay R+Z)
Virtua Cop 2 se traba despues de la imagen de inicio. VDP1/VDP2 en 0 (pantalla negra), FPS 60.
NADA se mueve (deadlock total, 100% congelado).

### MASTER (MSH2)
- PC clavado en 0x06000952. Flags F=0 (NO idle, NO sleeping: ejecuta pero gira).
- Spin-loop: instrucciones antes del PC = 002B 0009 9001 400E, en PC 8BFE (BF a si mismo).
  - 9001 = MOV.W @(disp,PC),R0  -> R0 = 0x000000F0
  - 400E = LDC R0,SR            -> pone SR = 0xF0 (mascara de interrupcion = 15, MAXIMA: bloquea IRQs)
  - 8BFE = BF a si mismo (spin cerrado)
- R0 = 0x000000F0.
- MCYC se mueve muy rapido (gira sin avanzar el PC).

### SLAVE (SSH2)
- PC clavado en 0x060046B4. Flags F=0x8000 (solo SLAVE: NO idle, NO sleeping: ejecuta pero gira).
- Instruccion en PC = 6020 = MOV.B @R2,R0 -> lee un BYTE de la direccion en R2.
- Instruccion anterior = 211A.
- **R2 = 0x060C801F** <- DIRECCION CLAVE: el slave pollea este byte en HWRAM compartida,
  esperando que el MASTER lo escriba. El master nunca llega a escribirlo -> deadlock.

## INTERPRETACION
- El slave espera que el master escriba el flag en 0x060C801F (HWRAM 0x06000000-0x060FFFFF).
- El master esta en su propio spin habiendo puesto la mascara de IRQ al maximo (SR=0xF0).
- Los 2 corren en bloques alternados grandes (dynarec) -> ninguno ve el cambio del otro
  en el momento justo -> deadlock total. Es el problema de granularidad conocido.

## YA DESCARTADO (NO REPETIR - ver DEADLOCK_MASTER_SLAVE.md)
- Chunking desde el bucle YabauseEmulate: el dynarec ejecuta bloques completos igual.
- Sync por Input Capture (MINIT/SINIT): no es por ahi.
- Slave estilo HLE: rompe otros juegos.

## CAMINO 1 A PROBAR (sync dirigido a HWRAM)
Cuando el slave lee repetidamente 0x060C801F (o zona HWRAM) sin cambio en un spin,
quemar su slice / ceder al master para que avance y escriba el flag. Analogo al
IDLE SKIP del Input Capture (sh2.c ~507) pero para lectura de HWRAM compartida.
Referencia interpretador (Yabause libretro) SI corre VC2 por granularidad de 1 instruccion.

## SLAVE IRQ (Camino 2 si falla el 1)
scu.c:1927-1938 manda IRQs al slave (vectores 0x41/0x43/0x40). Revisar si VC2 espera
un IRQ que no llega en el momento justo.
