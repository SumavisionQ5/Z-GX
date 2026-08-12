# INTEGRACION Z-GX CON WIIFLOW

## Archivos y pasos

1. ZGX.ini -> copiar a: sd:/wiiflow/plugins/ZGX.ini

2. El boot.dol de Z-GX -> copiar y RENOMBRAR a: sd:/wiiflow/plugins/ZGX.dol
   (el .ini apunta a dolfile=wiiflow/plugins/ZGX.dol)

3. Registrar el magic number en: sd:/wiiflow/plugins_data/platform.ini
   Agregar bajo la seccion correspondiente:
   5A47585F=Saturn
   (5A47585F = "ZGX_" en hex, es el magic del .ini)

4. Covers: poner caratulas en sd:/wiiflow/boxcovers/Saturn/
   (coverfolder=Saturn en el .ini)

5. Juegos: en sd:/ZGX/games/ (romdir del .ini), extensiones .cue o .chd

## Formato de argumentos (ya implementado en Z-GX)
WiiFlow envia: arguments={device}:/{path}|{name}
- argv[1] = sd:/ZGX/games  (device + path)
- argv[2] = juego.chd       (name)
Z-GX (main.c ~434) los une: sprintf(isofilename, "%s/%s", argv[1], argv[2])
= sd:/ZGX/games/juego.chd

## Requisitos que Z-GX YA cumple
- Autoboot desde argumentos (argc > 2): SI (main.c:432)
- Deteccion sd/usb desde el path: SI (main.c:436-440)
- bios en <device>/ZGX/bios/bios.bin, saves en <device>/ZGX/saves

## PENDIENTE / A PROBAR
- Retorno a WiiFlow al salir del juego (el {loader}). WiiStation usa solo
  {device}:/{path}|{name} sin loader, asi que quiza no haga falta. Si al salir
  no vuelve a WiiFlow, investigar como agregan el retorno WiiSX/WiiStation.
- Confirmar que el magic number no choque con otro plugin (cambiar si hace falta).
- Si WiiFlow pasa el path distinto (todo en argv[1]), ajustar main.c.
