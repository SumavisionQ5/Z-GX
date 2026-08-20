# VC2 HLE - AVANCE Y DIAGNOSTICO EXACTO (2 dias de trabajo)

## LOGRO: HLE del BIOS funcional + modo hibrido
- bios_Init() en yabause.c: construye la zona de sistema (vectores, tabla funciones,
  interrupt handlers, slave proc). Portado de YabaSanshiro BiosInit.
- bios_HandleFunc() en sh2.c (~1099): 13 funciones del BIOS implementadas:
  0x04 PowerOnMemoryClear, 0x20 ChangeScuInterruptPriority, 0x40 SetScuInterrupt,
  0x44 SetSh2Interrupt, 0x48 ChangeSystemClock, 0x4C GetSemaphore, 0x4D ClearSemaphore,
  0x50 SetScuInterruptMask, 0x51 ChangeScuInterruptMask, 0x56 BUPInit, 0x27/0x37 CDINIT,
  + interrupt handlers 0x80-0x9F (BiosHandleScuInterrupt) y 0xA0 (Return).
- sh2_IllegalFull() en sh2.c (~1081): intercepta en el illegal handler del dynarec
  (como YabaSanshiro sh2int.c:346), solo rango bajo 0x200-0x4FF.
- compiler.c: SH2JIT_ILLEGAL modificado para llamar sh2_IllegalFull(curr_pc).
- Indice: (pc - 0x200) >> 2 (como YabaSanshiro).
- flag zgx_hle_bios=1 (yabause.c:689).

## MODO HIBRIDO (el que mas avanza)
SpeedySetup: SIEMPRE copia BIOS real + si HLE llama bios_Init() encima.
yabause.c:694 "{ copiar BIOS real }" ; :745 "} if(zgx_hle_bios){bios_Init();}"
Con esto VC2 avanza MUCHISIMO: dibuja loading 10seg, IRQ:16, GM:0x06009E30, alta res.

## DIAGNOSTICO EXACTO DEL CUELGUE DE VC2 (medido en Wii real)
1. VC2 arranca, dibuja pantalla Sega + loading (~10 seg, VDP1/2 con actividad).
2. En la transicion loading->juego, el master cae en un LOOP en 0x000002B0-0x2B4
   (codigo del BIOS: 2342 / 4610=JMP @R6 / 8ffc=BF/S -4 / 000b=RTS).
3. R6 = 0x00000019 (direccion INVALIDA). El JMP @R6 saltaria a 0x19 (basura).
4. El master termina en 0x06000678 (loop de error del BIOS: affe=BRA -2).
5. VDP cae a 0 (deja de dibujar). Slave nunca se activa (RUN:0).
6. IRQ:16 (procesa interrupciones), master con mascara 0xF en el handler de error
   -> el VBlank (nivel 0xF) no lo despierta (0xF > 0xF = false).

## PROXIMO PASO (retomar aca)
- Rastrear de donde sale R6=0x19 antes de 0x2B0. La funcion 0x280 (indice 0x20) es
  ChangeScuInterruptPriority - VC2 la llama. Ver si nuestra implementacion HLE de 0x20
  corrompe R6/R0, o si el flujo del BIOS real en 0x2A0-0x2B4 espera un estado que falta.
- El loop 0x2B0-0x2B4 espera T=1 (BF/S sale si T=1). Ver que setea T.
- Posible fix: interceptar la funcion 0x280 completa en HLE (no ejecutar el BIOS real).
- Alternativa: ver por que el slave no arranca (SSHON) - puede ser la causa raiz.

## INSTRUMENTACION EN OVERLAY (abajo, R+Z) - LIMPIAR ANTES DE RELEASE
sh2.c: _hle_calls, _hle_idx, _hle_pc, _hle_maxidx, _m_gamemax, _m_prev, _m_beforeerr,
_m_after2b0, _m_r6, _m_from2b0, zgx_mpc2, zgx_spc2, zgx_slcode, zgx_track_master,
_m_irqproc. yabause.c: zgx_ssrun. osd.c linea 262 (overlay debug).
