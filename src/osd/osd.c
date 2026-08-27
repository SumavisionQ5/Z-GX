


#include "osd.h"
#include "gui.h"
#include "../yabause.h"
#include "../sgx/sgx.h"
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>



struct MsgQueue {
	u32 count;
	struct Msg {
		u32 msg;
		u32 m_len;
		u16 x;
		u16 y;
		u32 color;
	} queue[MAX_MESSAGES];
} osd = {0};


GXTexObj osd_tobj;

#define OSD_TEX_W	256
#define OSD_TEX_H	64


u8 msg_buffer[0x800];
u32 msg_index = 0;
u64 cycle_data[8];

extern yabsys_struct yabsys;



void osd_CyclesSet(u32 indx, u64 cycles)
{
	cycle_data[indx] = (cycles * 100) / yabsys.OneFrameTime;
}


void osd_MsgAdd(u32 x, u32 y, u32 color, char *msg)
{
	u32 len = 0;

	if (!msg || osd.count >= MAX_MESSAGES) {
		return;
	}

	osd.queue[osd.count].x = x;
	osd.queue[osd.count].y = y;
	osd.queue[osd.count].color = color;
	osd.queue[osd.count].msg = msg_index;

	//Copy the string to the message buffer:
	while ((msg_buffer[msg_index] = (*msg))) {
		msg_index = (msg_index + 1) & 0x7FF;
		++len;
		++msg;
	}
	msg_index = (msg_index + 1) & 0x7FF;

	osd.queue[osd.count].m_len = len;
	++osd.count;
}


void osd_MsgShow(void)
{
	/*Show messages*/
	if (!osd.count) {
		return;
	}


	//GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	/*Reserve GX_VTXFMT7 for OSD*/
	GX_SetVtxAttrFmt(GX_VTXFMT7, GX_VA_POS, GX_POS_XY, GX_U16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT7, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GX_SetNumChans(0);
	GX_SetNumTexGens(1);
	GX_SetNumTevStages(1);

	GX_LoadTexObjPreloaded(&gui_tobj, &gui_treg, GX_TEXMAP0);

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_KONST, GX_CC_TEXC, GX_CC_ZERO);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);

	GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);

	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	for (u32 i = 0; i < osd.count; ++i) {
		u32 c = osd.queue[i].msg;
		u32 x = osd.queue[i].x;
		u32 y = osd.queue[i].y;

		GX_SetTevKColor(GX_KCOLOR0, *((GXColor*) &osd.queue[i].color));

		GX_Begin(GX_QUADS, GX_VTXFMT7, 4 * osd.queue[i].m_len);
		/*not while m_ptr, must use index for this because of circular buffering*/
		for (u32 j = 0; j < osd.queue[i].m_len; ++j) {
			//XXX: Use a texCoordGen for this.
			f32 chr_x = (f32) ((msg_buffer[c]) & 0x1F) * 0.03125;
			f32 chr_y = (f32) (((msg_buffer[c]) >> 5) & 0x3) * 0.125;
			GX_Position2u16(x , y);					// Top Left
			GX_TexCoord2f32(chr_x, chr_y);
			GX_Position2u16(x + 8, y);			// Top Right
			GX_TexCoord2f32(chr_x + 0.03125, chr_y);
			GX_Position2u16(x + 8, y + 8);	// Bottom Right
			GX_TexCoord2f32(chr_x + 0.03125, chr_y + 0.125);
			GX_Position2u16(x, y + 8);			// Bottom Left
			GX_TexCoord2f32(chr_x, chr_y + 0.125);
			x += 8;
			c = (c + 1) & 0x7FF;
		}
		GX_End();
	}

	GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	osd.count = 0;
}
#define MAX_PROF_COUNTERS	16

u32 system_cycles;
struct ProfCounter {
	char *name;
	u32	is_active;
	u32	value;
} prof_counters[MAX_PROF_COUNTERS];

void osd_ProfInit(u32 sys_cycles)
{
	system_cycles = sys_cycles;
}


void osd_ProfAddCounter(u32 indx, char *name)
{
	prof_counters[indx].name = name;
	prof_counters[indx].is_active = 1;
}


void osd_ProfAddTime(u32 indx, u32 ticks)
{
	if (prof_counters[indx].is_active) {
		prof_counters[indx].value += ticks;
	}
}
extern u32 preloadtex_addr;

static void __osd_SetTev(void)
{
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetCurrentMtx(MTX_IDENTITY);
	GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, 8, 8);

	GX_SetScissor(0, 0, 640, 480);
	GX_SetNumChans(0);
	GX_SetNumTexGens(1);
	GX_SetNumTevStages(1);

	GX_LoadTexObjPreloaded(&gui_tobj, &gui_treg, GX_TEXMAP0);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_KONST, GX_CC_TEXC, GX_CC_ZERO);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CC_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);

	GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_1);
	GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
	GX_SetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_1_2);

	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
}

static void __osd_DrawText(u32 x, u32 y, char *text, u32 numc)
{
	GX_Begin(GX_QUADS, GX_VTXFMT1, 4 * numc);
	for (u32 j = 0; j < numc; ++j) {
		u32 chr_x = ((text[j]) & 0x1F);
		u32 chr_y = (((text[j]) >> 5) & 0x3);
		GX_Position2s16(x , y);					// Top Left
		GX_TexCoord2u16(chr_x, chr_y);
		GX_Position2s16(x + 8, y);			// Top Right
		GX_TexCoord2u16(chr_x + 1, chr_y);
		GX_Position2s16(x + 8, y + 8);	// Bottom Right
		GX_TexCoord2u16(chr_x + 1, chr_y + 1);
		GX_Position2s16(x, y + 8);			// Bottom Left
		GX_TexCoord2u16(chr_x, chr_y + 1);
		x += 8;
	}
	GX_End();
}

void osd_FPSDraw(u32 fps)
{
	char tstr[64];
	__osd_SetTev();
	GX_SetCurrentMtx(MTX_IDENTITY);

	{ extern u32 snd_muted; u32 numc; numc = sprintf(tstr, "%2d", fps);
	GX_SetTevKColor(GX_KCOLOR0, (GXColor) {0xBB, 0xFF, 0xCC, 0xFF});
	__osd_DrawText(0, 0, tstr, numc); }

	GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, 8, 1);
	{ extern u32 snd_muted; GX_SetDispCopySrc(0, 0, 1*16, 10); }
	SVI_CopyXFB(32, 24);
}


void osd_ProfDraw(void)
{
	char tstr[64];
	u32 y = 4, x = 4;
	u32 total = 0;
	static u32 accum[MAX_PROF_COUNTERS] = {0};
	static u32 shown[MAX_PROF_COUNTERS] = {0};
	static u32 fcount = 0;
	fcount++;
	__osd_SetTev();
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0xFF, 0xFF, 0xFF, 0xFF});
	for (u32 i = 0; i < MAX_PROF_COUNTERS; ++i) {
		accum[i] += (u32) ticks_to_microsecs(prof_counters[i].value);
		prof_counters[i].value = 0;
	}
	if (fcount >= 60) {
		for (u32 i = 0; i < MAX_PROF_COUNTERS; ++i) { shown[i] = accum[i] / 60; accum[i] = 0; }
		fcount = 0;
	}
	for (u32 i = 0; i < MAX_PROF_COUNTERS; ++i) {
		if (prof_counters[i].is_active) {
			x = 8;
			total += shown[i];
			u32 numc = sprintf(tstr, "%5s:%6d", prof_counters[i].name, shown[i]);
			__osd_DrawText(x, y, tstr, numc);
			y += 8;
		}
	}
	x = 8;
	u32 numc = sprintf(tstr, "%5s:%6d", "TOTAL", total);
	__osd_DrawText(x, y, tstr, numc);
	y += 8;
	{ extern u32 drc_comp_count, drc_inval_count, drc_flush_count, drc_idle_count;
	   { extern unsigned zgx_mpc2(void), zgx_spc2(void), zgx_mflags(void), zgx_sflags(void);
	     numc = sprintf(tstr, "MPC:%08X F:%X", zgx_mpc2(), zgx_mflags()); __osd_DrawText(x, y, tstr, numc); y += 8;
	     numc = sprintf(tstr, "SPC:%08X F:%X", zgx_spc2(), zgx_sflags()); __osd_DrawText(x, y, tstr, numc); y += 8;
	     numc = sprintf(tstr, "MCYC:%d", (int)({extern unsigned zgx_mcyc(void); zgx_mcyc();})); }
	{ extern unsigned zgx_mop(void), zgx_sop(void), zgx_mop2(void), zgx_sop2(void);
	  extern unsigned zgx_m4(void), zgx_m6(void), zgx_m8(void), zgx_mr0(void);
	  y += 8; numc = sprintf(tstr, "M:%04X %04X %04X %04X %04X T:%X", ({extern unsigned zgx_mE(void);zgx_mE();}), ({extern unsigned zgx_mC(void);zgx_mC();}), ({extern unsigned zgx_mA(void);zgx_mA();}), ({extern unsigned zgx_m8(void);zgx_m8();}), ({extern unsigned zgx_m6(void);zgx_m6();}), ({extern unsigned zgx_mt(void);zgx_mt();})); __osd_DrawText(x, y, tstr, numc);
	  y += 8; numc = sprintf(tstr, "MSR:%X IMS:%X IST:%X IRQP:%u", ({extern unsigned zgx_msr(void); zgx_msr();}), ({extern unsigned zgx_scuims(void); zgx_scuims();}), ({extern unsigned zgx_scuist(void); zgx_scuist();}), ({extern unsigned zgx_mirqproc(void); zgx_mirqproc();})); __osd_DrawText(x, y, tstr, numc);
	  extern unsigned zgx_s4(void), zgx_s6(void), zgx_s8(void), zgx_sr0(void), zgx_sr2(void);
	  y += 8; numc = sprintf(tstr, "S:%04X %04X %04X %04X", zgx_s8(), zgx_s6(), zgx_s4(), zgx_sop2()); __osd_DrawText(x, y, tstr, numc);
	  y += 8; numc = sprintf(tstr, "SBF:%04X R2:%08X", zgx_sop(), zgx_sr2()); __osd_DrawText(x, y, tstr, numc); }
	  __osd_DrawText(x, y, tstr, numc);
	  drc_comp_count=0; drc_inval_count=0; drc_flush_count=0; drc_idle_count=0; }
	{ extern u32 cfmt_dbg, bmw_dbg, bmconv_dbg, vdp2_disp_w, screen_enable; 
		y += 8; numc = sprintf(tstr, "CF:%u", cfmt_dbg); __osd_DrawText(x, y, tstr, numc); 
		y += 8; numc = sprintf(tstr, "BW:%u", bmw_dbg); __osd_DrawText(x, y, tstr, numc); 
		y += 8; numc = sprintf(tstr, "SE:%u", screen_enable); __osd_DrawText(x, y, tstr, numc);
		y += 8; numc = sprintf(tstr, "DW:%u", vdp2_disp_w); __osd_DrawText(x, y, tstr, numc); }
	GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, 8, 1);
	GX_SetDispCopySrc(0, 0, (40*8), y+12);
	SVI_CopyXFB(32, 320);
}
