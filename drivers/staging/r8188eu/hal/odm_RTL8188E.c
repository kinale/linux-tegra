// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2007 - 2011 Realtek Corporation. */

#include "../include/odm_precomp.h"

void ODM_DIG_LowerBound_88E(struct odm_dm_struct *dm_odm)
{
	struct rtw_dig *pDM_DigTable = &dm_odm->DM_DigTable;

	if (dm_odm->AntDivType == CG_TRX_HW_ANTDIV)
		pDM_DigTable->rx_gain_range_min = (u8) pDM_DigTable->AntDiv_RSSI_max;
	/* If only one Entry connected */
}

static void odm_RX_HWAntDivInit(struct odm_dm_struct *dm_odm)
{
	u32	value32;

	if (*(dm_odm->mp_mode) == 1) {
		dm_odm->AntDivType = CGCS_RX_SW_ANTDIV;
		ODM_SetBBReg(dm_odm, ODM_REG_IGI_A_11N, BIT7, 0); /*  disable HW AntDiv */
		ODM_SetBBReg(dm_odm, ODM_REG_LNA_SWITCH_11N, BIT31, 1);  /*  1:CG, 0:CS */
		return;
	}

	/* MAC Setting */
	value32 = ODM_GetMACReg(dm_odm, ODM_REG_ANTSEL_PIN_11N, bMaskDWord);
	ODM_SetMACReg(dm_odm, ODM_REG_ANTSEL_PIN_11N, bMaskDWord, value32|(BIT23|BIT25)); /* Reg4C[25]=1, Reg4C[23]=1 for pin output */
	/* Pin Settings */
	ODM_SetBBReg(dm_odm, ODM_REG_PIN_CTRL_11N, BIT9|BIT8, 0);/* Reg870[8]=1'b0, Reg870[9]=1'b0	antsel antselb by HW */
	ODM_SetBBReg(dm_odm, ODM_REG_RX_ANT_CTRL_11N, BIT10, 0);	/* Reg864[10]=1'b0	antsel2 by HW */
	ODM_SetBBReg(dm_odm, ODM_REG_LNA_SWITCH_11N, BIT22, 1);	/* Regb2c[22]=1'b0	disable CS/CG switch */
	ODM_SetBBReg(dm_odm, ODM_REG_LNA_SWITCH_11N, BIT31, 1);	/* Regb2c[31]=1'b1	output at CG only */
	/* OFDM Settings */
	ODM_SetBBReg(dm_odm, ODM_REG_ANTDIV_PARA1_11N, bMaskDWord, 0x000000a0);
	/* CCK Settings */
	ODM_SetBBReg(dm_odm, ODM_REG_BB_PWR_SAV4_11N, BIT7, 1); /* Fix CCK PHY status report issue */
	ODM_SetBBReg(dm_odm, ODM_REG_CCK_ANTDIV_PARA2_11N, BIT4, 1); /* CCK complete HW AntDiv within 64 samples */
	ODM_UpdateRxIdleAnt_88E(dm_odm, MAIN_ANT);
	ODM_SetBBReg(dm_odm, ODM_REG_ANT_MAPPING1_11N, 0xFFFF, 0x0201);	/* antenna mapping table */
}

static void odm_TRX_HWAntDivInit(struct odm_dm_struct *dm_odm)
{
	u32	value32;

	if (*(dm_odm->mp_mode) == 1) {
		dm_odm->AntDivType = CGCS_RX_SW_ANTDIV;
		ODM_SetBBReg(dm_odm, ODM_REG_IGI_A_11N, BIT7, 0); /*  disable HW AntDiv */
		ODM_SetBBReg(dm_odm, ODM_REG_RX_ANT_CTRL_11N, BIT5|BIT4|BIT3, 0); /* Default RX   (0/1) */
		return;
	}

	/* MAC Setting */
	value32 = ODM_GetMACReg(dm_odm, ODM_REG_ANTSEL_PIN_11N, bMaskDWord);
	ODM_SetMACReg(dm_odm, ODM_REG_ANTSEL_PIN_11N, bMaskDWord, value32|(BIT23|BIT25)); /* Reg4C[25]=1, Reg4C[23]=1 for pin output */
	/* Pin Settings */
	ODM_SetBBReg(dm_odm, ODM_REG_PIN_CTRL_11N, BIT9|BIT8, 0);/* Reg870[8]=1'b0, Reg870[9]=1'b0		antsel antselb by HW */
	ODM_SetBBReg(dm_odm, ODM_REG_RX_ANT_CTRL_11N, BIT10, 0);	/* Reg864[10]=1'b0	antsel2 by HW */
	ODM_SetBBReg(dm_odm, ODM_REG_LNA_SWITCH_11N, BIT22, 0);	/* Regb2c[22]=1'b0	disable CS/CG switch */
	ODM_SetBBReg(dm_odm, ODM_REG_LNA_SWITCH_11N, BIT31, 1);	/* Regb2c[31]=1'b1	output at CG only */
	/* OFDM Settings */
	ODM_SetBBReg(dm_odm, ODM_REG_ANTDIV_PARA1_11N, bMaskDWord, 0x000000a0);
	/* CCK Settings */
	ODM_SetBBReg(dm_odm, ODM_REG_BB_PWR_SAV4_11N, BIT7, 1); /* Fix CCK PHY status report issue */
	ODM_SetBBReg(dm_odm, ODM_REG_CCK_ANTDIV_PARA2_11N, BIT4, 1); /* CCK complete HW AntDiv within 64 samples */
	/* Tx Settings */
	ODM_SetBBReg(dm_odm, ODM_REG_TX_ANT_CTRL_11N, BIT21, 0); /* Reg80c[21]=1'b0		from TX Reg */
	ODM_UpdateRxIdleAnt_88E(dm_odm, MAIN_ANT);

	/* antenna mapping table */
	if (!dm_odm->bIsMPChip) { /* testchip */
		ODM_SetBBReg(dm_odm, ODM_REG_RX_DEFUALT_A_11N, BIT10|BIT9|BIT8, 1);	/* Reg858[10:8]=3'b001 */
		ODM_SetBBReg(dm_odm, ODM_REG_RX_DEFUALT_A_11N, BIT13|BIT12|BIT11, 2);	/* Reg858[13:11]=3'b010 */
	} else { /* MPchip */
		ODM_SetBBReg(dm_odm, ODM_REG_ANT_MAPPING1_11N, bMaskDWord, 0x0201);	/* Reg914=3'b010, Reg915=3'b001 */
	}
}

static void odm_FastAntTrainingInit(struct odm_dm_struct *dm_odm)
{
	u32	value32, i;
	struct fast_ant_train *dm_fat_tbl = &dm_odm->DM_FatTable;
	u32	AntCombination = 2;

	if (*(dm_odm->mp_mode) == 1)
		return;

	for (i = 0; i < 6; i++) {
		dm_fat_tbl->Bssid[i] = 0;
		dm_fat_tbl->antSumRSSI[i] = 0;
		dm_fat_tbl->antRSSIcnt[i] = 0;
		dm_fat_tbl->antAveRSSI[i] = 0;
	}
	dm_fat_tbl->TrainIdx = 0;
	dm_fat_tbl->FAT_State = FAT_NORMAL_STATE;

	/* MAC Setting */
	value32 = ODM_GetMACReg(dm_odm, 0x4c, bMaskDWord);
	ODM_SetMACReg(dm_odm, 0x4c, bMaskDWord, value32|(BIT23|BIT25)); /* Reg4C[25]=1, Reg4C[23]=1 for pin output */
	value32 = ODM_GetMACReg(dm_odm,  0x7B4, bMaskDWord);
	ODM_SetMACReg(dm_odm, 0x7b4, bMaskDWord, value32|(BIT16|BIT17)); /* Reg7B4[16]=1 enable antenna training, Reg7B4[17]=1 enable A2 match */

	/* Match MAC ADDR */
	ODM_SetMACReg(dm_odm, 0x7b4, 0xFFFF, 0);
	ODM_SetMACReg(dm_odm, 0x7b0, bMaskDWord, 0);

	ODM_SetBBReg(dm_odm, 0x870, BIT9|BIT8, 0);/* Reg870[8]=1'b0, Reg870[9]=1'b0		antsel antselb by HW */
	ODM_SetBBReg(dm_odm, 0x864, BIT10, 0);	/* Reg864[10]=1'b0	antsel2 by HW */
	ODM_SetBBReg(dm_odm, 0xb2c, BIT22, 0);	/* Regb2c[22]=1'b0	disable CS/CG switch */
	ODM_SetBBReg(dm_odm, 0xb2c, BIT31, 1);	/* Regb2c[31]=1'b1	output at CG only */
	ODM_SetBBReg(dm_odm, 0xca4, bMaskDWord, 0x000000a0);

	/* antenna mapping table */
	if (AntCombination == 2) {
		if (!dm_odm->bIsMPChip) { /* testchip */
			ODM_SetBBReg(dm_odm, 0x858, BIT10|BIT9|BIT8, 1);	/* Reg858[10:8]=3'b001 */
			ODM_SetBBReg(dm_odm, 0x858, BIT13|BIT12|BIT11, 2);	/* Reg858[13:11]=3'b010 */
		} else { /* MPchip */
			ODM_SetBBReg(dm_odm, 0x914, bMaskByte0, 1);
			ODM_SetBBReg(dm_odm, 0x914, bMaskByte1, 2);
		}
	} else if (AntCombination == 7) {
		if (!dm_odm->bIsMPChip) { /* testchip */
			ODM_SetBBReg(dm_odm, 0x858, BIT10|BIT9|BIT8, 0);	/* Reg858[10:8]=3'b000 */
			ODM_SetBBReg(dm_odm, 0x858, BIT13|BIT12|BIT11, 1);	/* Reg858[13:11]=3'b001 */
			ODM_SetBBReg(dm_odm, 0x878, BIT16, 0);
			ODM_SetBBReg(dm_odm, 0x858, BIT15|BIT14, 2);	/* Reg878[0],Reg858[14:15])=3'b010 */
			ODM_SetBBReg(dm_odm, 0x878, BIT19|BIT18|BIT17, 3);/* Reg878[3:1]=3b'011 */
			ODM_SetBBReg(dm_odm, 0x878, BIT22|BIT21|BIT20, 4);/* Reg878[6:4]=3b'100 */
			ODM_SetBBReg(dm_odm, 0x878, BIT25|BIT24|BIT23, 5);/* Reg878[9:7]=3b'101 */
			ODM_SetBBReg(dm_odm, 0x878, BIT28|BIT27|BIT26, 6);/* Reg878[12:10]=3b'110 */
			ODM_SetBBReg(dm_odm, 0x878, BIT31|BIT30|BIT29, 7);/* Reg878[15:13]=3b'111 */
		} else { /* MPchip */
			ODM_SetBBReg(dm_odm, 0x914, bMaskByte0, 0);
			ODM_SetBBReg(dm_odm, 0x914, bMaskByte1, 1);
			ODM_SetBBReg(dm_odm, 0x914, bMaskByte2, 2);
			ODM_SetBBReg(dm_odm, 0x914, bMaskByte3, 3);
			ODM_SetBBReg(dm_odm, 0x918, bMaskByte0, 4);
			ODM_SetBBReg(dm_odm, 0x918, bMaskByte1, 5);
			ODM_SetBBReg(dm_odm, 0x918, bMaskByte2, 6);
			ODM_SetBBReg(dm_odm, 0x918, bMaskByte3, 7);
		}
	}

	/* Default Ant Setting when no fast training */
	ODM_SetBBReg(dm_odm, 0x80c, BIT21, 1); /* Reg80c[21]=1'b1		from TX Info */
	ODM_SetBBReg(dm_odm, 0x864, BIT5|BIT4|BIT3, 0);	/* Default RX */
	ODM_SetBBReg(dm_odm, 0x864, BIT8|BIT7|BIT6, 1);	/* Optional RX */

	/* Enter Traing state */
	ODM_SetBBReg(dm_odm, 0x864, BIT2|BIT1|BIT0, (AntCombination-1));	/* Reg864[2:0]=3'd6	ant combination=reg864[2:0]+1 */
	ODM_SetBBReg(dm_odm, 0xc50, BIT7, 1);	/* RegC50[7]=1'b1		enable HW AntDiv */
}

void ODM_AntennaDiversityInit_88E(struct odm_dm_struct *dm_odm)
{
	if (dm_odm->SupportICType != ODM_RTL8188E)
		return;

	if (dm_odm->AntDivType == CGCS_RX_HW_ANTDIV)
		odm_RX_HWAntDivInit(dm_odm);
	else if (dm_odm->AntDivType == CG_TRX_HW_ANTDIV)
		odm_TRX_HWAntDivInit(dm_odm);
	else if (dm_odm->AntDivType == CG_TRX_SMART_ANTDIV)
		odm_FastAntTrainingInit(dm_odm);
}

void ODM_UpdateRxIdleAnt_88E(struct odm_dm_struct *dm_odm, u8 Ant)
{
	struct fast_ant_train *dm_fat_tbl = &dm_odm->DM_FatTable;
	u32	DefaultAnt, OptionalAnt;

	if (dm_fat_tbl->RxIdleAnt != Ant) {
		if (Ant == MAIN_ANT) {
			DefaultAnt = (dm_odm->AntDivType == CG_TRX_HW_ANTDIV) ? MAIN_ANT_CG_TRX : MAIN_ANT_CGCS_RX;
			OptionalAnt = (dm_odm->AntDivType == CG_TRX_HW_ANTDIV) ? AUX_ANT_CG_TRX : AUX_ANT_CGCS_RX;
		} else {
			DefaultAnt = (dm_odm->AntDivType == CG_TRX_HW_ANTDIV) ? AUX_ANT_CG_TRX : AUX_ANT_CGCS_RX;
			OptionalAnt = (dm_odm->AntDivType == CG_TRX_HW_ANTDIV) ? MAIN_ANT_CG_TRX : MAIN_ANT_CGCS_RX;
		}

		if (dm_odm->AntDivType == CG_TRX_HW_ANTDIV) {
			ODM_SetBBReg(dm_odm, ODM_REG_RX_ANT_CTRL_11N, BIT5|BIT4|BIT3, DefaultAnt);	/* Default RX */
			ODM_SetBBReg(dm_odm, ODM_REG_RX_ANT_CTRL_11N, BIT8|BIT7|BIT6, OptionalAnt);		/* Optional RX */
			ODM_SetBBReg(dm_odm, ODM_REG_ANTSEL_CTRL_11N, BIT14|BIT13|BIT12, DefaultAnt);	/* Default TX */
			ODM_SetMACReg(dm_odm, ODM_REG_RESP_TX_11N, BIT6|BIT7, DefaultAnt);	/* Resp Tx */
		} else if (dm_odm->AntDivType == CGCS_RX_HW_ANTDIV) {
			ODM_SetBBReg(dm_odm, ODM_REG_RX_ANT_CTRL_11N, BIT5|BIT4|BIT3, DefaultAnt);	/* Default RX */
			ODM_SetBBReg(dm_odm, ODM_REG_RX_ANT_CTRL_11N, BIT8|BIT7|BIT6, OptionalAnt);		/* Optional RX */
		}
	}
	dm_fat_tbl->RxIdleAnt = Ant;
	if (Ant != MAIN_ANT)
		pr_info("RxIdleAnt=AUX_ANT\n");
}

static void odm_UpdateTxAnt_88E(struct odm_dm_struct *dm_odm, u8 Ant, u32 MacId)
{
	struct fast_ant_train *dm_fat_tbl = &dm_odm->DM_FatTable;
	u8	TargetAnt;

	if (Ant == MAIN_ANT)
		TargetAnt = MAIN_ANT_CG_TRX;
	else
		TargetAnt = AUX_ANT_CG_TRX;
	dm_fat_tbl->antsel_a[MacId] = TargetAnt&BIT0;
	dm_fat_tbl->antsel_b[MacId] = (TargetAnt&BIT1)>>1;
	dm_fat_tbl->antsel_c[MacId] = (TargetAnt&BIT2)>>2;
}

void ODM_SetTxAntByTxInfo_88E(struct odm_dm_struct *dm_odm, u8 *pDesc, u8 macId)
{
	struct fast_ant_train *dm_fat_tbl = &dm_odm->DM_FatTable;

	if ((dm_odm->AntDivType == CG_TRX_HW_ANTDIV) || (dm_odm->AntDivType == CG_TRX_SMART_ANTDIV)) {
		SET_TX_DESC_ANTSEL_A_88E(pDesc, dm_fat_tbl->antsel_a[macId]);
		SET_TX_DESC_ANTSEL_B_88E(pDesc, dm_fat_tbl->antsel_b[macId]);
		SET_TX_DESC_ANTSEL_C_88E(pDesc, dm_fat_tbl->antsel_c[macId]);
	}
}

void ODM_AntselStatistics_88E(struct odm_dm_struct *dm_odm, u8 antsel_tr_mux, u32 MacId, u8 RxPWDBAll)
{
	struct fast_ant_train *dm_fat_tbl = &dm_odm->DM_FatTable;
	if (dm_odm->AntDivType == CG_TRX_HW_ANTDIV) {
		if (antsel_tr_mux == MAIN_ANT_CG_TRX) {
			dm_fat_tbl->MainAnt_Sum[MacId] += RxPWDBAll;
			dm_fat_tbl->MainAnt_Cnt[MacId]++;
		} else {
			dm_fat_tbl->AuxAnt_Sum[MacId] += RxPWDBAll;
			dm_fat_tbl->AuxAnt_Cnt[MacId]++;
		}
	} else if (dm_odm->AntDivType == CGCS_RX_HW_ANTDIV) {
		if (antsel_tr_mux == MAIN_ANT_CGCS_RX) {
			dm_fat_tbl->MainAnt_Sum[MacId] += RxPWDBAll;
			dm_fat_tbl->MainAnt_Cnt[MacId]++;
		} else {
			dm_fat_tbl->AuxAnt_Sum[MacId] += RxPWDBAll;
			dm_fat_tbl->AuxAnt_Cnt[MacId]++;
		}
	}
}

static void odm_HWAntDiv(struct odm_dm_struct *dm_odm)
{
	u32	i, MinRSSI = 0xFF, AntDivMaxRSSI = 0, MaxRSSI = 0, LocalMinRSSI, LocalMaxRSSI;
	u32	Main_RSSI, Aux_RSSI;
	u8	RxIdleAnt = 0, TargetAnt = 7;
	struct fast_ant_train *dm_fat_tbl = &dm_odm->DM_FatTable;
	struct rtw_dig *pDM_DigTable = &dm_odm->DM_DigTable;
	struct sta_info *pEntry;

	for (i = 0; i < ODM_ASSOCIATE_ENTRY_NUM; i++) {
		pEntry = dm_odm->pODM_StaInfo[i];
		if (IS_STA_VALID(pEntry)) {
			/* 2 Caculate RSSI per Antenna */
			Main_RSSI = (dm_fat_tbl->MainAnt_Cnt[i] != 0) ? (dm_fat_tbl->MainAnt_Sum[i]/dm_fat_tbl->MainAnt_Cnt[i]) : 0;
			Aux_RSSI = (dm_fat_tbl->AuxAnt_Cnt[i] != 0) ? (dm_fat_tbl->AuxAnt_Sum[i]/dm_fat_tbl->AuxAnt_Cnt[i]) : 0;
			TargetAnt = (Main_RSSI >= Aux_RSSI) ? MAIN_ANT : AUX_ANT;
			/* 2 Select MaxRSSI for DIG */
			LocalMaxRSSI = (Main_RSSI > Aux_RSSI) ? Main_RSSI : Aux_RSSI;
			if ((LocalMaxRSSI > AntDivMaxRSSI) && (LocalMaxRSSI < 40))
				AntDivMaxRSSI = LocalMaxRSSI;
			if (LocalMaxRSSI > MaxRSSI)
				MaxRSSI = LocalMaxRSSI;

			/* 2 Select RX Idle Antenna */
			if ((dm_fat_tbl->RxIdleAnt == MAIN_ANT) && (Main_RSSI == 0))
				Main_RSSI = Aux_RSSI;
			else if ((dm_fat_tbl->RxIdleAnt == AUX_ANT) && (Aux_RSSI == 0))
				Aux_RSSI = Main_RSSI;

			LocalMinRSSI = (Main_RSSI > Aux_RSSI) ? Aux_RSSI : Main_RSSI;
			if (LocalMinRSSI < MinRSSI) {
				MinRSSI = LocalMinRSSI;
				RxIdleAnt = TargetAnt;
			}
			/* 2 Select TRX Antenna */
			if (dm_odm->AntDivType == CG_TRX_HW_ANTDIV)
				odm_UpdateTxAnt_88E(dm_odm, TargetAnt, i);
		}
		dm_fat_tbl->MainAnt_Sum[i] = 0;
		dm_fat_tbl->AuxAnt_Sum[i] = 0;
		dm_fat_tbl->MainAnt_Cnt[i] = 0;
		dm_fat_tbl->AuxAnt_Cnt[i] = 0;
	}

	/* 2 Set RX Idle Antenna */
	ODM_UpdateRxIdleAnt_88E(dm_odm, RxIdleAnt);

	pDM_DigTable->AntDiv_RSSI_max = AntDivMaxRSSI;
	pDM_DigTable->RSSI_max = MaxRSSI;
}

void ODM_AntennaDiversity_88E(struct odm_dm_struct *dm_odm)
{
	struct fast_ant_train *dm_fat_tbl = &dm_odm->DM_FatTable;
	if ((dm_odm->SupportICType != ODM_RTL8188E) || (!(dm_odm->SupportAbility & ODM_BB_ANT_DIV)))
		return;
	if (!dm_odm->bLinked) {
		if (dm_fat_tbl->bBecomeLinked) {
			ODM_SetBBReg(dm_odm, ODM_REG_IGI_A_11N, BIT7, 0);	/* RegC50[7]=1'b1		enable HW AntDiv */
			ODM_SetBBReg(dm_odm, ODM_REG_CCK_ANTDIV_PARA1_11N, BIT15, 0); /* Enable CCK AntDiv */
			if (dm_odm->AntDivType == CG_TRX_HW_ANTDIV)
				ODM_SetBBReg(dm_odm, ODM_REG_TX_ANT_CTRL_11N, BIT21, 0); /* Reg80c[21]=1'b0		from TX Reg */
			dm_fat_tbl->bBecomeLinked = dm_odm->bLinked;
		}
		return;
	} else {
		if (!dm_fat_tbl->bBecomeLinked) {
			/* Because HW AntDiv is disabled before Link, we enable HW AntDiv after link */
			ODM_SetBBReg(dm_odm, ODM_REG_IGI_A_11N, BIT7, 1);	/* RegC50[7]=1'b1		enable HW AntDiv */
			ODM_SetBBReg(dm_odm, ODM_REG_CCK_ANTDIV_PARA1_11N, BIT15, 1); /* Enable CCK AntDiv */
			if (dm_odm->AntDivType == CG_TRX_HW_ANTDIV)
				ODM_SetBBReg(dm_odm, ODM_REG_TX_ANT_CTRL_11N, BIT21, 1); /* Reg80c[21]=1'b1		from TX Info */
			dm_fat_tbl->bBecomeLinked = dm_odm->bLinked;
		}
	}
	if ((dm_odm->AntDivType == CG_TRX_HW_ANTDIV) || (dm_odm->AntDivType == CGCS_RX_HW_ANTDIV))
		odm_HWAntDiv(dm_odm);
}

/* 3============================================================ */
/* 3 Dynamic Primary CCA */
/* 3============================================================ */

void odm_PrimaryCCA_Init(struct odm_dm_struct *dm_odm)
{
	struct dyn_primary_cca *PrimaryCCA = &(dm_odm->DM_PriCCA);

	PrimaryCCA->DupRTS_flag = 0;
	PrimaryCCA->intf_flag = 0;
	PrimaryCCA->intf_type = 0;
	PrimaryCCA->Monitor_flag = 0;
	PrimaryCCA->PriCCA_flag = 0;
}

bool ODM_DynamicPrimaryCCA_DupRTS(struct odm_dm_struct *dm_odm)
{
	struct dyn_primary_cca *PrimaryCCA = &(dm_odm->DM_PriCCA);

	return	PrimaryCCA->DupRTS_flag;
}
