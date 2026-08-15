# FIX GUARDADO BACKUP RAM (partidas internas del Saturn)

## PROBLEMA
El guardado interno de cada juego (backup RAM del Saturn) no funcionaba desde la
primera version de ZGX. La carpeta saves quedaba vacia aunque el juego dijera
"guardado exitosamente". Y si habia un .bup, al cargar daba error DSI (crash).

## DIAGNOSTICO (con debug temporal)
- La RUTA estaba bien: saves_dir=[sd:/ZGX/saves], bupfilename=[sd:/ZGX/saves/<itemnum>.bup]. NO era la reorganizacion de carpetas.
- SaveBackupRam se llamaba pero last_block=2 SIEMPRE -> cortaba con return -1 sin escribir.
  Causa: BupLastUsedBlock() (memory.c:837) nunca detecta bloques usados (bug de
  precedencia/logica en la condicion linea 844) -> devuelve 1 -> save abortado.
- LoadBackupRam tenia fread(&bup_savefile,...) con & de mas (memory.c:887): escribia
  sobre el PUNTERO en vez del buffer -> corrompia memoria -> DSI al cargar.

## FIX APLICADO (src/memory.c)
1. SaveBackupRam (~913): quitado el corte last_block==2, forzado last_block=256
   para guardar toda la backup RAM sin depender del BupLastUsedBlock roto.
2. LoadBackupRam (~887): fread(&bup_savefile,...) -> fread(bup_savefile,...) (quitado &).

## AUTOSAVE (bonus, src/vdp2.c Vdp2VBlankIN ~284)
Guarda la backup RAM 60 frames (1seg) despues de la ultima escritura (flag
bup_ram_written). Asi persiste sin depender de salir por el menu (util si apagas la Wii).

## RESULTADO
- El .bup se crea en sd:/ZGX/saves/. Battle Garegga guarda y carga OK (probado 2 veces).
- Ya no dan DSI en cadena.

## PENDIENTE
- Batsugun: da DSI si se carga como PRIMER juego tras encender; anda si antes cargas
  otro juego. Parece problema de inicializacion (backup RAM/buffer con basura la 1ra vez).
- Confirmar guardado en SD y USB (el amigo va a probar).
