# BUG "NEGRO / VIDEO NO SE VE" — Diagnóstico sesión video (VDP2 bitmap)

## HALLAZGO PRINCIPAL (grande)
Los "juegos negros" son DOS categorías distintas:
- Categoria A (deadlock): Cotton 2, SF Zero 3, Astal. Los CONTADORES SE DETIENEN
  al ponerse negro. Es el deadlock master-slave (ver NOTAS_BUG_NEGRO.md). El master
  cicla 0x4204->094E->0952, recibe NMI (lo saca a 0x55E delay loop) pero vuelve.
  A12 B0 (master manda 12 ICF al slave, slave nunca despierta al master). NO resuelto.
- Categoria B (VIDEO): Clockwork Knight, Shienryu, y los FMVs/intros de MUCHOS
  juegos (Street Fighter Alpha, etc). El juego CORRE (hay SONIDO, contadores SIGUEN).
  Solo el VIDEO no se dibuja. Mucho mas facil y prometedor.

## CAUSA DE CATEGORIA B: SGX_Vdp2DrawBitmap ESTABA VACIA
En el fork, sgx_vdp2.c la funcion SGX_Vdp2DrawBitmap() estaba VACIA { }.
En 0.3.2 tenia toda la implementacion. Por eso los juegos que usan capas VDP2 en
modo BITMAP quedaban NEGROS. Clockwork usa NBG0 modo bitmap:
  overlay: SE20 CA0042 (screen_enable=0x20 bit PRI_NGB0; CHCTLA=0x0042 ->
  color_fmt = (0x42>>4)&0x7 = 4 = 32bpp).

## LO QUE SE HIZO
Se porto DrawBitmap de 0.3.2. El fork renombro la API:
  - SGX_BeginVdp2Bitmap(fmt,w,h) 0.3.2 -> ya no existe. Fork: SGX_BeginVdp2Scroll(fmt,sz)
    fuerza textura CUADRADA sz x sz.
  - Fork tiene SGX_CellConverterSet(cellsize,bpp) (tiles) y
    SGX_SpriteConverterSet(width,bpp,align) (LINEAL).
Se creo helper SGX_BeginVdp2BitmapRect(fmt,w,h) rectangular en sgx_vdp2.c ~350.

## RESULTADO ACTUAL
DrawBitmap YA DIBUJA (antes negro). Clockwork muestra el video PERO con fallos:
  - CellConverterSet + ancho 512 + RGB5A3: 3 copias del video pero SE DISTINGUE.
  - SpriteConverterSet (LINEAL) + ancho grande: pantalla completa pero PIXELES
    DESORDENADOS. El converter lineal es EL CAMINO CORRECTO (bitmap es lineal) pero
    el WIDTH (stride) no es exacto.

## CLAVE PARA MANANA
1. El bitmap VDP2 es LINEAL como sprites VDP1. Usar SGX_SpriteConverterSet
   (indtex.c:198) NO CellConverterSet.
2. Ajustar el WIDTH (stride). color_fmt=4 (32bpp) leido como RGB5A3 (16bpp) = FACTOR 2.
   Width real del FMV Clockwork ~320 o 352, no 512.
3. color_fmt = (cell.char_ctl >> 4) & 0x7  (sgx_vdp2.c:339).
4. Para bitmap, cell.char_size NUNCA se setea (sgx_vdp2.c:340). Basura del frame previo.
5. Pipeline fork usa VTXFMT5 + indirect textures (SGX_Vdp2DrawCellSimple ~424 dibuja
   BIEN, es el modelo). La portada de 0.3.2 usa VTXFMT1. Quizas reescribir DrawBitmap
   al estilo VTXFMT5 del fork copiando DrawCellSimple.

## ARCHIVOS
- DrawBitmap WIP: NOTAS/drawbitmap_wip.txt
- Ref 0.3.2: curl -sL "https://raw.githubusercontent.com/fadedled/seta-gx/master/src/sgx/sgx_vdp2.c"

## DEBUG ACTIVO (quitar antes de commit)
- Overlay "SE%02X CA%04X" en osd.c:219 con MTX_IDENTITY (era 2X). chctla_dbg en
  sgx_vdp2.c:54 y SGX_Vdp2Draw.
- git checkout -- src/osd/osd.c  quita el overlay.
- DrawBitmap restaurada NO commiteada. Preservada en NOTAS/.
PENDIENTE FMV: cinemas se traban IRREGULAR (no parejo). FPS se mantiene 57-60 (NO es velocidad). En emu de PC van fluidos. Es SINCRONIZACION del decodificador/streaming CD, no rendimiento. Investigar timing de frames FMV.

=== FIX VIRTUA COP (VDP1 Scissor + Z) - de rama Gemini ===
En src/sgx/sgx_vdp1.c:
- Lineas 217, 586, 589, 994: GX_SetScissor(0, 0, 640, 480) (desactiva recorte)
- Linea 592: GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_DISABLE) (desactiva test Z)
Resultado: Virtua Cop muestra enemigos y fondo. Probado 3 juegos mas, no rompio nada.
PENDIENTE: capa VERDE del VDP2 quedo de fondo, tapa piso y cielo (se ven verdes).
  Es back screen o capa VDP2 que se dibuja verde solido. Investigar.

=== HALLAZGO: piso verde Virtua Cop = CF4 (mismo pendiente) ===
Virtua Cop usa NBG0+NBG1+NBG2 (screen_enable=56). El PISO es NBG1 en modo
BITMAP CF4 (32bpp). El piso sale VERDE (deberia ser gris carretera) = MISMO
problema del tinte verde de los videos CF4. Un solo fix (layout/color RGBA8 32bpp)
arregla videos CF4 Y el piso de Virtua Cop.
Toggle debug R+X cicla _skip_layer (0=todo 1=sinNBG0 2=sinNBG1 3=sinNBG2) en sgx_vdp2.c.
Confirmado: skip NBG1 quita el piso verde.
CAUSA RGBA8: GX usa tiles 4x4 entrelazados (AR luego GB), Saturn es lineal.

=== CF4 GEOMETRIA RESUELTA (falta solo color) ===
CF4 (32bpp) ahora con GEOMETRIA PERFECTA:
- case 4: fmt = GX_TF_RGB565 (NO RGBA8). bpp_id SPRITE_16BPP.
- bm_width <<= 1 para CF4 (compensa lectura).
- matriz vdp2mtx[0][0] = bm_width>>1 para CF4 (corrige ancho).
Resultado: alto/ancho perfecto, letras legibles centradas, SIN lineas ni rejilla.
FALTA SOLO COLOR: sale rosa/azul plano (deberia color real). RGB565 toma 2 de los
4 bytes del pixel 32bpp -> pierde componentes. Ajustar orden/lectura de bytes de color.
REFERENCIA futura: PicoDrive (RetroArch Wii) corre 32X 60fps con dynarec SH2 - util
para dynarec SH2 y timing FMV (NO para video Saturn, hardware distinto).

=== EXPERIMENTO CD TIMING (descartado) ===
Probado bajar _periodictiming 2x de 20000 a 15000: el video va MAS RAPIDO pero con
los MISMOS tirones. Conclusion: el jitter NO es la velocidad de lectura del CD.
El problema es la REGULARIDAD de la presentacion de frames (pipeline de video/bitmap),
no el streaming del CD. Descartado el CD timing. Revertido a 20000.
Proxima vez: investigar el pipeline de presentacion del bitmap (SGX_Vdp2DrawBitmap)
y como se sincroniza la actualizacion del frame de video con el VBlank.

=== CF4 (piso/cielo verde Virtua Cop) - DIAGNOSTICO COMPLETO (Sep 5) ===
RUTA REAL: el piso de VC es NBG1 CELDA (no bitmap) color_fmt 4 (32bpp).
Pasa por SGX_Vdp2DrawCellSimple() case 4 (sgx_vdp2.c:462): usa GX_TF_RGBA8 SIN converter.
El tile tiene TEXTURA (geometria OK) pero color VERDE porque GX lee el RGBA8 con su
layout entrelazado (tiles 4x4, planos AR y GB separados) sobre datos LINEALES del Saturn
(0x??RRGGBB). Los bytes de color quedan mezclados -> verde.

INTENTOS QUE NO FUNCIONARON:
- Conversion CF4 en SGX_Vdp2DrawBitmap: NO es la ruta (el piso es celda, no bitmap).
- Cambiar case 4 a RGB5A3 + converter 16bpp: nada cambia (el converter 16bpp no reordena bien el 32bpp).
- Conversion por CPU tile-por-tile: DESCARTADA, mataria el rendimiento (VC corre 60fps, caeria a 30 o menos).

POR QUE ES DIFICIL: el RGBA8 de GX tiene layout de DOBLE PLANO (32 bytes AR + 32 bytes GB
por tile 4x4), distinto a los formatos de 1 plano (4/8/16bpp) que el sistema de indirect
texture (indtex.c __indtex4/8/16bppGen) sabe manejar. Una indirect texture 32bpp NO se
deriva del 16bpp (no es "doble ancho") - requiere manejar los 2 planos. Rediseño grande
del converter. Es el fix mas dificil del renderer.

FIX REAL PENDIENTE: extender el sistema de indirect texture (indtex.c) para soportar el
layout de doble plano de GX_TF_RGBA8, o encontrar otro formato GX que lea el 32bpp lineal
del Saturn sin costo de CPU. El piso verde es JUGABLE (no impide jugar), baja prioridad.
