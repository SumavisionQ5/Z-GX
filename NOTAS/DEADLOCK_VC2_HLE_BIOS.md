# DEADLOCK VC2 - DIAGNOSTICO COMPLETO + INFRAESTRUCTURA BIOS HLE

## DATOS DUROS CAPTURADOS (instrumentacion en Wii real)
- Master trabado en PC=0x06000952 (codigo del BIOS copiado a HWRAM por SpeedySetup,
  rango 0x06000220-0x06000B00 <- de 0x00000820). Lee 0x06000354 en spin (31500+ veces).
- Slave en PC=0x060046B4. NO spinnea leyendo HWRAM (SLrd=0). Hace otra cosa.
- 0x06000354 = variable que BiosBUPInit escribe (Kronos bios.c:794 = R[5]).
- BIOS HLE handler NUNCA se dispara en VC2 (BIOS:0). El master se traba en el spin de
  0x06000354 ANTES de llamar cualquier funcion del BIOS.

## CAUSA RAIZ (confirmada)
El master ejecuta codigo REAL del BIOS (copiado a HWRAM) que lee 0x06000354 esperando
un valor de inicializacion. Nuestro QuickLoad copia el CODIGO del BIOS pero NO inicializa
los DATOS de la zona de sistema (0x06000300-0x06000358) de forma coherente.
Kronos inicializa toda esa zona en bios.c:144-171 (tabla de punteros a funciones +
slave proc). Nosotros solo llegamos hasta 0x0600032C en SpeedySetup.

## POR QUE NO SE PUDO ARREGLAR CON PARCHES
- Inicializar la tabla 0x06000300-0x06000358 como Kronos ROMPE el arranque de TODOS
  los juegos (pantalla negra). Direcciones que rompen: 0x06000340 y otras.
  Seguras: 0x06000330, 0x06000334, 0x06000354. (aisladas una por una en Wii real)
- Poner solo 0x06000354=0 NO destraba VC2 (espera un valor no-cero, un puntero a tabla).
- La zona de datos y el codigo del BIOS estan acoplados: parchear datos sueltos rompe
  la coherencia que el codigo del BIOS espera.

## INFRAESTRUCTURA YA IMPLEMENTADA (funciona, conservar)
1. bios_HandleFunc() en sh2.c (~1077): handler HLE con:
   - case 0x4C (0x06000330): BiosGetSemaphore (test-and-set en 0x06000B00+R4)
   - case 0x4D (0x06000334): BiosClearSemaphore
   - case 0x56 (0x06000358): BiosBUPInit (escribe 0x06000354=R5 + tabla)
   Portado de Kronos/Yabause. Compila, no rompe. Backups: sh2.c.antes_bioshle
2. Interceptacion en el DYNAREC (compiler.c ~1436, _jit_GenBlock): si el PC entra en
   0x06000200-0x060003FF, genera un bloque que llama a bios_HandleFunc en vez de
   compilar. USA PPCC_BL(bios_HandleFunc). Backup: compiler.c.antes_bioshle
   ESTA ES LA PIEZA CLAVE: permite HLE del BIOS con dynarec (Kronos usa interprete).

## CAMINO REAL PARA RESOLVERLO (sesion dedicada)
Portar el HLE del BIOS COMPLETO y COHERENTE de Kronos (bios.c BiosInit, ~lineas 88-172):
- La rutina BiosInit de Kronos inicializa TODA la zona de forma coherente: vectores,
  interrupt handlers (0x06000600 con opcodes reales rte/nop), tabla de funciones,
  slave proc, Y los datos de sistema. Todo junto, no suelto.
- Hay que reemplazar/complementar YabauseSpeedySetup con el BiosInit completo de Kronos,
  y verificar que NO rompa los juegos que ya andan (probar Cotton/Gekirindan cada paso).
- El handler bios_HandleFunc ya esta listo para recibir las llamadas.
- Referencia exacta: ~/kronos/yabause/src/bios.c funcion BiosInit (88-174) y
  BiosHandleFunc (1697+). Yabause retroarch: /c/Users/hotgl/saturn-wii/.../bios.c

## ESTADO ACTUAL
- yabause.c: revertido (sin tabla que rompe). Solo el codigo original de SpeedySetup.
- sh2.c: tiene bios_HandleFunc (semaforos + BUPInit).
- compiler.c: tiene la interceptacion del dynarec.
- TODO compila y los juegos que andaban siguen andando (Cotton confirmado).
- La interceptacion no se dispara en VC2 porque se traba antes (en el spin del codigo BIOS).
