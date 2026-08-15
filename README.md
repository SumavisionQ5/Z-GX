# Z-GX

**Z-GX** es un fork del emulador experimental de Sega Saturn para Wii **Seta GX** (de Evoca/fadedled), mantenido por **CheloRetro**.

Seta GX esta basado en el port original de Yabause para Wii, fuertemente modificado. Algunas mejoras (soporte CHD y emulacion del SCU) vienen del Yaba Sanshiro de devmiyax. La aceleracion de video usa la GPU Hollywood.

Licenciado bajo GNU GPL v2: http://www.gnu.org/licenses/gpl-2.0.txt

DESCARGO: Proyecto experimental, no pulido en estabilidad. Es esperable encontrar bugs.

## Mejoras de este fork (Z-GX)

- Vsync a 60fps.
- Modo 240p para CRT.
- Soporte del cartucho de RAM de 4MB (KOF, Street Fighter Alpha, etc.).
- Deteccion de caratulas (PNG).
- Menu mejorado: Izq/Der salta por pagina, mantener Arriba/Abajo hace scroll continuo, soporte de subcarpetas.
- Estructura de carpetas unificada bajo sd:/ZGX/.

## Instalacion

Copia la carpeta apps/ZGX a la carpeta apps de tu SD/USB. Estructura:

    sd:/ZGX/games/         los juegos (.chd, .cue, o subcarpetas)
    sd:/ZGX/bios/bios.bin  el BIOS
    sd:/ZGX/saves/         guardados
    sd:/ZGX/art/           caratulas (PNG con el nombre exacto del juego 128x192)
    sd:/apps/ZGX/          boot.dol, icon.png, meta.xml

BIOS NO INCLUIDO: coloca tu BIOS en sd:/ZGX/bios/bios.bin (recomendado region-libre v1.00).

## Creditos

- CheloRetro - Fork Z-GX
- NiuuS - Logo de Z-GX, pruebas y administracion del proyecto
- Evoca (fadedled) - Seta GX, base de este fork
- Yabause Team - Codigo fuente original
- devmiyax - Yaba Sanshiro
- Extrems - Ayuda con GX y libogc2
- emu_kidid y Pcercuei - WiiSX
- tueidj - Codigo de memoria virtual
