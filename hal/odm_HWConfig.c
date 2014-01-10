/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/

//============================================================
// include files
//============================================================

#include "odm_precomp.h"

#define READ_AND_CONFIG     READ_AND_CONFIG_MP

#define READ_AND_CONFIG_MP(ic, txt) (ODM_ReadAndConfig##txt##ic(pDM_Odm))
#define READ_AND_CONFIG_TC(ic, txt) (ODM_ReadAndConfig_TC##txt##ic(pDM_Odm))

static u8 odm_QueryRxPwrPercentage(s8 AntPower)
{
	if ((AntPower <= -100) || (AntPower >= 20))
		return	0;
	else if (AntPower >= 0)
		return	100;
	else
		return	100 + AntPower;
}

//
// 2012/01/12 MH MOve some signal strength smooth method to MP HAL layer.
// IF other SW team do not support the feature, remove this section.??
//
static s32
odm_SignalScaleMapping_92CSeries_patch_RT_CID_819x_Lenovo(
	PDM_ODM_T pDM_Odm,
	s32 CurrSig
)
{
	s32 RetSig;
	return RetSig;
}

static s32
odm_SignalScaleMapping_92CSeries_patch_RT_CID_819x_Netcore(
 PDM_ODM_T pDM_Odm,
	s32 CurrSig
)
{
	s32 RetSig;
	return RetSig;
}


static s32
odm_SignalScaleMapping_92CSeries(
	PDM_ODM_T pDM_Odm,
	s32 CurrSig
)
{
	s32 RetSig;

	if((pDM_Odm->SupportInterface  == ODM_ITRF_USB) || (pDM_Odm->SupportInterface  == ODM_ITRF_SDIO) ) {
		if(CurrSig >= 51 && CurrSig <= 100)
			RetSig = 100;
		else if(CurrSig >= 41 && CurrSig <= 50)
			RetSig = 80 + ((CurrSig - 40)*2);
		else if(CurrSig >= 31 && CurrSig <= 40)
			RetSig = 66 + (CurrSig - 30);
		else if(CurrSig >= 21 && CurrSig <= 30)
			RetSig = 54 + (CurrSig - 20);
		else if(CurrSig >= 10 && CurrSig <= 20)
			RetSig = 42 + (((CurrSig - 10) * 2) / 3);
		else if(CurrSig >= 5 && CurrSig <= 9)
			RetSig = 22 + (((CurrSig - 5) * 3) / 2);
		else if(CurrSig >= 1 && CurrSig <= 4)
			RetSig = 6 + (((CurrSig - 1) * 3) / 2);
		else
			RetSig = CurrSig;
	}
	return RetSig;
}

static s32 odm_SignalScaleMapping(PDM_ODM_T pDM_Odm, s32 CurrSig)
{
	return odm_SignalScaleMapping_92CSeries(pDM_Odm,CurrSig);
}

//pMgntInfo->CustomerID == RT_CID_819x_Lenovo
static u8 odm_SQ_process_patch_RT_CID_819x_Lenovo(
 PDM_ODM_T	pDM_Odm,
 u8		isCCKrate,
 u8		PWDB_ALL,
 u8		path,
 u8		RSSI
)
{
	u8	SQ;
	return SQ;
}

static u8
odm_EVMdbToPercentage(
	s8 Value
    )
{
	//
	// -33dB~0dB to 0%~99%
	//
	s8 ret_val;

	ret_val = Value;
	//ret_val /= 2;

	//ODM_RTPRINT(FRX, RX_PHY_SQ, ("EVMdbToPercentage92C Value=%d / %x \n", ret_val, ret_val));

	if(ret_val >= 0)
		ret_val = 0;
	if(ret_val <= -33)
		ret_val = -33;

	ret_val = 0 - ret_val;
	ret_val*=3;

	if(ret_val == 99)
		ret_val = 100;

	return(ret_val);
}



static void odm_RxPhyStatus92CSeries_Parsing(
	PDM_ODM_T					pDM_Odm,
	PODM_PHY_INFO_T			pPhyInfo,
	u8 *						pPhyStatus,
	PODM_PACKET_INFO_T			pPktinfo
	)
{
	SWAT_T				*pDM_SWAT_Table = &pDM_Odm->DM_SWAT_Table;
	u8				i, Max_spatial_stream;
	s8				rx_pwr[4], rx_pwr_all=0;
	u8				EVM, PWDB_ALL = 0, PWDB_ALL_BT;
	u8				RSSI, total_rssi=0;
	u8				isCCKrate=0;
	u8				rf_rx_num = 0;
	u8				cck_highpwr = 0;
	u8				LNA_idx, VGA_idx;

	PPHY_STATUS_RPT_8192CD_T pPhyStaRpt = (PPHY_STATUS_RPT_8192CD_T)pPhyStatus;

	isCCKrate = (pPktinfo->Rate <= DESC92C_RATE11M) ? true : false;

	pPhyInfo->RxMIMOSignalQuality[ODM_RF_PATH_A] = -1;
	pPhyInfo->RxMIMOSignalQuality[ODM_RF_PATH_B] = -1;


	if(isCCKrate) {
		u8 report;
		u8 cck_agc_rpt;

		pDM_Odm->PhyDbgInfo.NumQryPhyStatusCCK++;
		//
		// (1)Hardware does not provide RSSI for CCK
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//

		//if(pHalData->eRFPowerState == eRfOn)
			cck_highpwr = pDM_Odm->bCckHighPower;
		//else
		//	cck_highpwr = false;

		cck_agc_rpt =  pPhyStaRpt->cck_agc_rpt_ofdm_cfosho_a ;

		//The RSSI formula should be modified according to the gain table
		if(!cck_highpwr)
		{
			report =( cck_agc_rpt & 0xc0 )>>6;
			switch(report)
			{
				// 03312009 modified by cosa
				// Modify the RF RNA gain value to -40, -20, -2, 14 by Jenyu's suggestion
				// Note: different RF with the different RNA gain.
				case 0x3:
					rx_pwr_all = -46 - (cck_agc_rpt & 0x3e);
					break;
				case 0x2:
					rx_pwr_all = -26 - (cck_agc_rpt & 0x3e);
					break;
				case 0x1:
					rx_pwr_all = -12 - (cck_agc_rpt & 0x3e);
					break;
				case 0x0:
					rx_pwr_all = 16 - (cck_agc_rpt & 0x3e);
					break;
			}
		}
		else
		{
			//report = pDrvInfo->cfosho[0] & 0x60;
			//report = pPhyStaRpt->cck_agc_rpt_ofdm_cfosho_a& 0x60;

			report = (cck_agc_rpt & 0x60)>>5;
			switch(report)
			{
				case 0x3:
					rx_pwr_all = -46 - ((cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x2:
					rx_pwr_all = -26 - ((cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x1:
					rx_pwr_all = -12 - ((cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x0:
					rx_pwr_all = 16 - ((cck_agc_rpt & 0x1f)<<1) ;
					break;
			}
		}

		PWDB_ALL = odm_QueryRxPwrPercentage(rx_pwr_all);

		//Modification for ext-LNA board
		if(pDM_Odm->BoardType == ODM_BOARD_HIGHPWR)
		{
			if((cck_agc_rpt>>7) == 0){
				PWDB_ALL = (PWDB_ALL>94)?100:(PWDB_ALL +6);
			}
			else
                   {
				if(PWDB_ALL > 38)
					PWDB_ALL -= 16;
				else
					PWDB_ALL = (PWDB_ALL<=16)?(PWDB_ALL>>2):(PWDB_ALL -12);
			}

			//CCK modification
			if(PWDB_ALL > 25 && PWDB_ALL <= 60)
				PWDB_ALL += 6;
			//else if (PWDB_ALL <= 25)
			//	PWDB_ALL += 8;
		}
		else//Modification for int-LNA board
		{
			if(PWDB_ALL > 99)
				PWDB_ALL -= 8;
			else if(PWDB_ALL > 50 && PWDB_ALL <= 68)
				PWDB_ALL += 4;
		}

		pPhyInfo->RxPWDBAll = PWDB_ALL;
		pPhyInfo->BTRxRSSIPercentage = PWDB_ALL;
		pPhyInfo->RecvSignalPower = rx_pwr_all;
		//
		// (3) Get Signal Quality (EVM)
		//
		if(pPktinfo->bPacketMatchBSSID)
		{
			u8	SQ,SQ_rpt;

			SQ_rpt = pPhyStaRpt->cck_sig_qual_ofdm_pwdb_all;

			if(SQ_rpt > 64)
				SQ = 0;
			else if (SQ_rpt < 20)
				SQ = 100;
			else
				SQ = ((64-SQ_rpt) * 100) / 44;

			pPhyInfo->SignalQuality = SQ;
			pPhyInfo->RxMIMOSignalQuality[ODM_RF_PATH_A] = SQ;
			pPhyInfo->RxMIMOSignalQuality[ODM_RF_PATH_B] = -1;
		}
	}
	else //is OFDM rate
	{
		pDM_Odm->PhyDbgInfo.NumQryPhyStatusOFDM++;

		//
		// (1)Get RSSI for HT rate
		//

	 for(i = ODM_RF_PATH_A; i < ODM_RF_PATH_MAX; i++)
		{
			// 2008/01/30 MH we will judge RF RX path now.
			if (pDM_Odm->RFPathRxEnable & BIT(i))
				rf_rx_num++;
			//else
				//continue;

			rx_pwr[i] = ((pPhyStaRpt->path_agc[i].gain& 0x3F)*2) - 110;

			pPhyInfo->RxPwr[i] = rx_pwr[i];

			/* Translate DBM to percentage. */
			RSSI = odm_QueryRxPwrPercentage(rx_pwr[i]);
			total_rssi += RSSI;
			//RTPRINT(FRX, RX_PHY_SS, ("RF-%d RXPWR=%x RSSI=%d\n", i, rx_pwr[i], RSSI));

			//Modification for ext-LNA board
			if(pDM_Odm->BoardType == ODM_BOARD_HIGHPWR)
			{
				if((pPhyStaRpt->path_agc[i].trsw) == 1)
					RSSI = (RSSI>94)?100:(RSSI +6);
				else
					RSSI = (RSSI<=16)?(RSSI>>3):(RSSI -16);

				if((RSSI <= 34) && (RSSI >=4))
					RSSI -= 4;
			}

			pPhyInfo->RxMIMOSignalStrength[i] =(u8) RSSI;

			//Get Rx snr value in DB
			pPhyInfo->RxSNR[i] = pDM_Odm->PhyDbgInfo.RxSNRdB[i] = (s32)(pPhyStaRpt->path_rxsnr[i]/2);
		}

		//
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//
		rx_pwr_all = (((pPhyStaRpt->cck_sig_qual_ofdm_pwdb_all) >> 1 )& 0x7f) -110;
		//RTPRINT(FRX, RX_PHY_SS, ("PWDB_ALL=%d\n",	PWDB_ALL));

		PWDB_ALL_BT = PWDB_ALL = odm_QueryRxPwrPercentage(rx_pwr_all);
		//RTPRINT(FRX, RX_PHY_SS, ("PWDB_ALL=%d\n",PWDB_ALL));

		pPhyInfo->RxPWDBAll = PWDB_ALL;
		//ODM_RT_TRACE(pDM_Odm,ODM_COMP_RSSI_MONITOR, ODM_DBG_LOUD, ("ODM OFDM RSSI=%d\n",pPhyInfo->RxPWDBAll));
		pPhyInfo->BTRxRSSIPercentage = PWDB_ALL_BT;
		pPhyInfo->RxPower = rx_pwr_all;
		pPhyInfo->RecvSignalPower = rx_pwr_all;

		//
		// (3)EVM of HT rate
		//
		if(pPktinfo->Rate >=DESC92C_RATEMCS8 && pPktinfo->Rate <=DESC92C_RATEMCS15)
			Max_spatial_stream = 2; //both spatial stream make sense
		else
			Max_spatial_stream = 1; //only spatial stream 1 makes sense

		for(i=0; i<Max_spatial_stream; i++) {
			// Do not use shift operation like "rx_evmX >>= 1" because the compilor of free build environment
			// fill most significant bit to "zero" when doing shifting operation which may change a negative
			// value to positive one, then the dbm value (which is supposed to be negative)  is not correct anymore.
			EVM = odm_EVMdbToPercentage( (pPhyStaRpt->stream_rxevm[i] ));	//dbm

			if(pPktinfo->bPacketMatchBSSID) {
				if(i==ODM_RF_PATH_A) // Fill value in RFD, Get the first spatial stream only
				{
					pPhyInfo->SignalQuality = (u8)(EVM & 0xff);
				}
				pPhyInfo->RxMIMOSignalQuality[i] = (u8)(EVM & 0xff);
			}
		}

	}
	//UI BSS List signal strength(in percentage), make it good looking, from 0~100.
	//It is assigned to the BSS List in GetValueFromBeaconOrProbeRsp().
	if(isCCKrate)
	{
		pPhyInfo->SignalStrength = (u8)(odm_SignalScaleMapping(pDM_Odm, PWDB_ALL));//PWDB_ALL;
	} else {
		if (rf_rx_num != 0)
			pPhyInfo->SignalStrength = (u8)(odm_SignalScaleMapping(pDM_Odm, total_rssi/=rf_rx_num));
	}
}

void
odm_Init_RSSIForDM(
	PDM_ODM_T	pDM_Odm
	)
{

}

static void odm_Process_RSSIForDM(
	PDM_ODM_T					pDM_Odm,
		PODM_PHY_INFO_T			pPhyInfo,
		PODM_PACKET_INFO_T			pPktinfo
	)
{

	s32			UndecoratedSmoothedPWDB, UndecoratedSmoothedCCK, UndecoratedSmoothedOFDM, RSSI_Ave;
	u8			isCCKrate=0;
	u8			RSSI_max, RSSI_min, i;
	u32			OFDM_pkt=0;
	u32			Weighting=0;
	PSTA_INFO_T	pEntry;

	if(pPktinfo->StationID == 0xFF)
		return;

	pEntry = pDM_Odm->pODM_StaInfo[pPktinfo->StationID];
	if(!IS_STA_VALID(pEntry) )
		return;
	if((!pPktinfo->bPacketMatchBSSID) )
		return;

	isCCKrate = (pPktinfo->Rate <= DESC92C_RATE11M) ? true : false;

	//-----------------Smart Antenna Debug Message------------------//

	UndecoratedSmoothedCCK =  pEntry->rssi_stat.UndecoratedSmoothedCCK;
	UndecoratedSmoothedOFDM = pEntry->rssi_stat.UndecoratedSmoothedOFDM;
	UndecoratedSmoothedPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;

	if(pPktinfo->bPacketToSelf || pPktinfo->bPacketBeacon)
	{

		if(!isCCKrate)//ofdm rate
		{
			if(pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_B] == 0){
				RSSI_Ave = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_A];
			}
			else
			{
				//DbgPrint("pRfd->Status.RxMIMOSignalStrength[0] = %d, pRfd->Status.RxMIMOSignalStrength[1] = %d \n",
					//pRfd->Status.RxMIMOSignalStrength[0], pRfd->Status.RxMIMOSignalStrength[1]);


				if(pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_A] > pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_B])
				{
					RSSI_max = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_A];
					RSSI_min = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_B];
				}
				else
				{
					RSSI_max = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_B];
					RSSI_min = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_A];
				}
				if((RSSI_max -RSSI_min) < 3)
					RSSI_Ave = RSSI_max;
				else if((RSSI_max -RSSI_min) < 6)
					RSSI_Ave = RSSI_max - 1;
				else if((RSSI_max -RSSI_min) < 10)
					RSSI_Ave = RSSI_max - 2;
				else
					RSSI_Ave = RSSI_max - 3;
			}

			//1 Process OFDM RSSI
			if(UndecoratedSmoothedOFDM <= 0)	// initialize
			{
				UndecoratedSmoothedOFDM = pPhyInfo->RxPWDBAll;
			}
			else
			{
				if(pPhyInfo->RxPWDBAll > (u32)UndecoratedSmoothedOFDM)
				{
					UndecoratedSmoothedOFDM =
							( ((UndecoratedSmoothedOFDM)*(Rx_Smooth_Factor-1)) +
							(RSSI_Ave)) /(Rx_Smooth_Factor);
					UndecoratedSmoothedOFDM = UndecoratedSmoothedOFDM + 1;
				}
				else
				{
					UndecoratedSmoothedOFDM =
							( ((UndecoratedSmoothedOFDM)*(Rx_Smooth_Factor-1)) +
							(RSSI_Ave)) /(Rx_Smooth_Factor);
				}
			}

			pEntry->rssi_stat.PacketMap = (pEntry->rssi_stat.PacketMap<<1) | BIT0;

		}
		else
		{
			RSSI_Ave = pPhyInfo->RxPWDBAll;

			//1 Process CCK RSSI
			if(UndecoratedSmoothedCCK <= 0)	// initialize
			{
				UndecoratedSmoothedCCK = pPhyInfo->RxPWDBAll;
			}
			else
			{
				if(pPhyInfo->RxPWDBAll > (u32)UndecoratedSmoothedCCK)
				{
					UndecoratedSmoothedCCK =
							( ((UndecoratedSmoothedCCK)*(Rx_Smooth_Factor-1)) +
							(pPhyInfo->RxPWDBAll)) /(Rx_Smooth_Factor);
					UndecoratedSmoothedCCK = UndecoratedSmoothedCCK + 1;
				}
				else
				{
					UndecoratedSmoothedCCK =
							( ((UndecoratedSmoothedCCK)*(Rx_Smooth_Factor-1)) +
							(pPhyInfo->RxPWDBAll)) /(Rx_Smooth_Factor);
				}
			}
			pEntry->rssi_stat.PacketMap = pEntry->rssi_stat.PacketMap<<1;
		}

		//if(pEntry)
		{
			//2011.07.28 LukeLee: modified to prevent unstable CCK RSSI
			if(pEntry->rssi_stat.ValidBit >= 64)
				pEntry->rssi_stat.ValidBit = 64;
			else
				pEntry->rssi_stat.ValidBit++;

			for(i=0; i<pEntry->rssi_stat.ValidBit; i++)
				OFDM_pkt += (u8)(pEntry->rssi_stat.PacketMap>>i)&BIT0;

			if(pEntry->rssi_stat.ValidBit == 64)
			{
				Weighting = ((OFDM_pkt<<4) > 64)?64:(OFDM_pkt<<4);
				UndecoratedSmoothedPWDB = (Weighting*UndecoratedSmoothedOFDM+(64-Weighting)*UndecoratedSmoothedCCK)>>6;
			}
			else
			{
				if(pEntry->rssi_stat.ValidBit != 0)
					UndecoratedSmoothedPWDB = (OFDM_pkt*UndecoratedSmoothedOFDM+(pEntry->rssi_stat.ValidBit-OFDM_pkt)*UndecoratedSmoothedCCK)/pEntry->rssi_stat.ValidBit;
				else
					UndecoratedSmoothedPWDB = 0;
			}

			pEntry->rssi_stat.UndecoratedSmoothedCCK = UndecoratedSmoothedCCK;
			pEntry->rssi_stat.UndecoratedSmoothedOFDM = UndecoratedSmoothedOFDM;
			pEntry->rssi_stat.UndecoratedSmoothedPWDB = UndecoratedSmoothedPWDB;

			//DbgPrint("OFDM_pkt=%d, Weighting=%d\n", OFDM_pkt, Weighting);
			//DbgPrint("UndecoratedSmoothedOFDM=%d, UndecoratedSmoothedPWDB=%d, UndecoratedSmoothedCCK=%d\n",
			//	UndecoratedSmoothedOFDM, UndecoratedSmoothedPWDB, UndecoratedSmoothedCCK);

		}

	}
}


//
// Endianness before calling this API
//
static void ODM_PhyStatusQuery_92CSeries(
	PDM_ODM_T					pDM_Odm,
		PODM_PHY_INFO_T				pPhyInfo,
		u8 *						pPhyStatus,
		PODM_PACKET_INFO_T			pPktinfo
	)
{

	odm_RxPhyStatus92CSeries_Parsing(
							pDM_Odm,
							pPhyInfo,
							pPhyStatus,
							pPktinfo);

	if( pDM_Odm->RSSI_test == true)
	{
		// Select the packets to do RSSI checking for antenna switching.
		if(pPktinfo->bPacketToSelf || pPktinfo->bPacketBeacon )
			ODM_SwAntDivChkPerPktRssi(pDM_Odm,pPktinfo->StationID,pPhyInfo);
	}
	else
	{
		odm_Process_RSSIForDM(pDM_Odm,pPhyInfo,pPktinfo);
	}

}



//
// Endianness before calling this API
//
static void ODM_PhyStatusQuery_JaguarSeries(
	PDM_ODM_T					pDM_Odm,
		PODM_PHY_INFO_T			pPhyInfo,
		u8 *						pPhyStatus,
		PODM_PACKET_INFO_T			pPktinfo
	)
{


}

void
ODM_PhyStatusQuery(
	PDM_ODM_T					pDM_Odm,
		PODM_PHY_INFO_T				pPhyInfo,
		u8 *						pPhyStatus,
		PODM_PACKET_INFO_T			pPktinfo
	)
{
	ODM_PhyStatusQuery_92CSeries(pDM_Odm,pPhyInfo,pPhyStatus,pPktinfo);
}

// For future use.
void
ODM_MacStatusQuery(
	PDM_ODM_T					pDM_Odm,
		u8 *						pMacStatus,
		u8						MacID,
		bool						bPacketMatchBSSID,
		bool						bPacketToSelf,
		bool						bPacketBeacon
	)
{
	// 2011/10/19 Driver team will handle in the future.

}

HAL_STATUS
ODM_ConfigRFWithHeaderFile(
	PDM_ODM_T			pDM_Odm,
	ODM_RF_RADIO_PATH_E	Content,
	ODM_RF_RADIO_PATH_E	eRFPath
    )
{
	//RT_STATUS	rtStatus = RT_STATUS_SUCCESS;


    ODM_RT_TRACE(pDM_Odm, ODM_COMP_INIT, ODM_DBG_LOUD, ("===>ODM_ConfigRFWithHeaderFile\n"));
#if (RTL8723A_SUPPORT == 1)
	if (pDM_Odm->SupportICType == ODM_RTL8723A)
	{
		if(eRFPath == ODM_RF_PATH_A)
			READ_AND_CONFIG_MP(8723A,_RadioA_1T_);

		ODM_RT_TRACE(pDM_Odm, ODM_COMP_INIT, ODM_DBG_LOUD, (" ===> ODM_ConfigRFWithHeaderFile() Radio_A:Rtl8723RadioA_1TArray\n"));
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_INIT, ODM_DBG_LOUD, (" ===> ODM_ConfigRFWithHeaderFile() Radio_B:Rtl8723RadioB_1TArray\n"));
	}

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_INIT, ODM_DBG_TRACE, ("ODM_ConfigRFWithHeaderFile: Radio No %x\n", eRFPath));
	//rtStatus = RT_STATUS_SUCCESS;
#endif
	return HAL_STATUS_SUCCESS;
}


HAL_STATUS
ODM_ConfigBBWithHeaderFile(
	PDM_ODM_T			pDM_Odm,
	ODM_BB_Config_Type		ConfigType
	)
{
#if (RTL8723A_SUPPORT == 1)
    if(pDM_Odm->SupportICType == ODM_RTL8723A)
	{

		if(ConfigType == CONFIG_BB_PHY_REG)
		{
			READ_AND_CONFIG_MP(8723A,_PHY_REG_1T_);
		}
		else if(ConfigType == CONFIG_BB_AGC_TAB)
		{
			READ_AND_CONFIG_MP(8723A,_AGC_TAB_1T_);
		}
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_INIT, ODM_DBG_LOUD, (" ===> phy_ConfigBBWithHeaderFile() phy:Rtl8723AGCTAB_1TArray\n"));
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_INIT, ODM_DBG_LOUD, (" ===> phy_ConfigBBWithHeaderFile() agc:Rtl8723PHY_REG_1TArray\n"));
	}
#endif

	return HAL_STATUS_SUCCESS;
}

HAL_STATUS
ODM_ConfigMACWithHeaderFile(
	PDM_ODM_T	pDM_Odm
	)
{
	u8 result = HAL_STATUS_SUCCESS;
#if (RTL8723A_SUPPORT == 1)
	if (pDM_Odm->SupportICType == ODM_RTL8723A)
	{
		READ_AND_CONFIG_MP(8723A,_MAC_REG_);
	}
#endif
	return result;
}
