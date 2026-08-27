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

## ACTUALIZACION (sesion Aug 27) - DIAGNOSTICO MAS PROFUNDO
- Fix slave (ceder en spin HWRAM, sh2_Read8): el SLAVE avanza de 0x060046B4 a 0x06004DCC.
  No destraba VC2 pero el slave progresa. Umbral probado 64 y 8, mismo resultado.
- Fix nivel 15 en sh2_HandleInterrupt (level >= 15): aplicado, no destraba.
- MASTER: IRQP (interrupciones procesadas) SUBE SIN PARAR (miles). El master SI recibe
  VBlanks, ejecuta el handler, y VUELVE al spin 0x06000952. NO espera un IRQ.
- MSR del master = 0xF0 constante (mascara 15). IMS SCU=0xFFFFF15C (VBlank NO enmascarado,
  bit0=0). IST=0x28xx. El VBlank llega bien.
- Bloque del master (de viejo a nuevo): 62F2 61F6 60F6 022B 0009 9001 400E 8BFE
  - 60F6/61F6/62F2 = MOV.L @R15+,Rn (restaurar registros de la pila = epilogo/retorno)
  - 9001 = MOV.W @(2,PC),R0 -> R0=0xF0
  - 400E = LDC R0,SR -> SR=0xF0 (mascara 15)
  - 8BFE = BF a si mismo (spin)
  - **T bit = 0** -> el BF (branch if false) siempre salta -> spin infinito.
- El master gira en BF esperando T=1, pero NADA en el loop cambia T. Solo una interrupcion
  o un cambio de flujo lo sacaria. El handler del VBlank vuelve con T=0.

## HIPOTESIS ACTUAL (para proxima sesion)
El master ejecuta un epilogo (restaura registros), deshabilita IRQs (SR=0xF0) y gira en BF
esperando T=1. El handler del VBlank no cambia T ni el flujo. Puede ser:
1. El master espera que el handler del VBlank haga algo que no hace (revisar el handler en VBR+0x40*4).
2. El master espera un valor en memoria que el slave escribe (el slave llega a 0x06004DCC pero no completa).
3. Bug en como el dynarec maneja el RTE o el LDC SR en este contexto.
SF03 es un caso DISTINTO (IRQP fijo, master deja de recibir IRQs) - no mezclar con VC2.
