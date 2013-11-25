#define		OISCMD

#include	"Ois.h"
#include	"OisDef.h"

#include	"HtcActOisBinder.h"


void			MesFil( unsigned char ) ;					
#ifdef		MODULE_CALIBRATION		
 #ifndef	HALLADJ_HW
  void			LopIni( unsigned char ) ;					
 #endif
 void			LopPar( unsigned char ) ;					
 #ifndef	HALLADJ_HW
  void			LopSin( unsigned char, unsigned char ) ;	
  unsigned char	LopAdj( unsigned char ) ;					
  void			LopMes( void ) ;							
 #endif
 #ifndef	HALLADJ_HW
 unsigned long	GinMes( unsigned char ) ;					
 #endif
#endif
void			GyrCon( unsigned char ) ;					
short			GenMes( unsigned short, unsigned char ) ;	

#ifdef		MODULE_CALIBRATION		
 #ifndef	HALLADJ_HW
  unsigned long	TneOff( UnDwdVal, unsigned char ) ;			
  unsigned long	TneBia( UnDwdVal, unsigned char ) ;			
 #endif
#endif
#ifdef USE_EXE2PROM
void			AdjSav( unsigned char ) ;					
#endif
void 			StbOnn( void ) ;							

void			SetSineWave(   unsigned char , unsigned char );
void			StartSineWave( void );
void			StopSineWave(  void );

void			SetMeasFil(    unsigned char , unsigned char , unsigned char );
void			StartMeasFil( void );
void			StopMeasFil( void );



#define		MES_XG1			0								
#define		MES_XG2			1								

#define		HALL_ADJ		0
#define		LOOPGAIN		1
#define		THROUGH			2
#define		NOISE			3


 #define		TNE 			80								
#ifdef	HALLADJ_HW

 #define	 __MEASURE_LOOPGAIN 	 0x00
 #define	 __MEASURE_BIASOFFSET	 0x01

#else

 #define		MARJIN			0x0300							
 #define		BIAS_ADJ_BORDER	0x1998							

 #define		HALL_MAX_GAP	BIAS_ADJ_BORDER - MARJIN
 #define		HALL_MIN_GAP	BIAS_ADJ_BORDER + MARJIN
 #define		BIAS_LIMIT		0xFFFF							
 #define		OFFSET_DIV		2								
 #define		TIME_OUT		40								

#endif


#ifdef	HALLADJ_HW
 unsigned char UcAdjBsy;

#else
 unsigned short	UsStpSiz	= 0 ;							
 unsigned short	UsErrBia, UsErrOfs ;
#endif



#define		ZOOMTBL	4
const unsigned long	ClGyxZom[ ZOOMTBL ]	= {
		0x3F800000,
		0x3F800000,		
		0x3FBD317A,		
		0x3FF6D6ED		
	} ;

const unsigned long	ClGyyZom[ ZOOMTBL ]	= {
		0x3F800000,
		0x3F800000,		
		0x3FBD317A,		
		0x3FF6D6ED 		
	} ;


#define		COEFTBL	7
const unsigned long	ClDiCof[ COEFTBL ]	= {
		0x3F7FF000,		
		0x3F7FF200,		
		0x3F7FF400,		
		0x3F7FF600,		
		0x3F7FF800,		
		0x3F7FFA00,		
		0x3F7FFC00		
	} ;


#ifdef		MODULE_CALIBRATION		
 #ifdef	HALLADJ_HW
 #else
 #define	INITBIASVAL		0xA000
 #endif
unsigned short	TneRun( void )
{
	unsigned char	UcHlySts, UcHlxSts, UcAtxSts, UcAtySts ;
	unsigned short	UsFinSts , UsOscSts ; 								
 #ifndef	HALLADJ_HW
	UnDwdVal		StTneVal ;
 #endif
 #ifdef USE_EXE2PROM
	unsigned short	UsDatBk ;
 #endif

	
	 UsOscSts	= OscAdj() ;
 #ifdef USE_EXE2PROM
	UsDatBk = (unsigned short)StAdjPar.UcOscVal ;
	E2pWrt( OSC_CLK_VAL,	2, ( unsigned char * )&UsDatBk ) ;
 #endif


 #ifdef	HALLADJ_HW
	UcHlySts = BiasOffsetAdj( Y_DIR , 0 ) ;
	WitTim( TNE ) ;
	UcHlxSts = BiasOffsetAdj( X_DIR , 0 ) ;
	WitTim( TNE ) ;
	UcHlySts = BiasOffsetAdj( Y_DIR , 1 ) ;
	WitTim( TNE ) ;
	UcHlxSts = BiasOffsetAdj( X_DIR , 1 ) ;
 #else
	SrvCon( X_DIR, ON ) ;
	SrvCon( Y_DIR, OFF ) ;
	WitTim( TNE ) ;

	StTneVal.UlDwdVal	= TnePtp( Y_DIR , PTP_BEFORE ) ;
	UcHlySts	= TneCen( Y_DIR, StTneVal ) ;
	
	SrvCon( Y_DIR, ON ) ;
	SrvCon( X_DIR, OFF ) ;
	WitTim( TNE ) ;

	StTneVal.UlDwdVal	= TnePtp( X_DIR , PTP_BEFORE ) ;
	UcHlxSts	= TneCen( X_DIR, StTneVal ) ;

	SrvCon( X_DIR, ON ) ;
	SrvCon( Y_DIR, OFF ) ;
	WitTim( TNE ) ;

	StTneVal.UlDwdVal	= TnePtp( Y_DIR , PTP_AFTER ) ;
	UcHlySts	= TneCen( Y_DIR, StTneVal ) ;

	SrvCon( Y_DIR, ON ) ;
	SrvCon( X_DIR, OFF ) ;
	WitTim( TNE ) ;

	StTneVal.UlDwdVal	= TnePtp( X_DIR , PTP_AFTER ) ;
	UcHlxSts	= TneCen( X_DIR, StTneVal ) ;

	
	if( UcHlxSts - EXE_END )
	{
		SrvCon( Y_DIR, ON ) ;
		SrvCon( X_DIR, OFF ) ;
		WitTim( TNE ) ;

		RamWriteA( DAHLXB, INITBIASVAL ) ;		
		StTneVal.UlDwdVal	= TnePtp( X_DIR , PTP_AFTER ) ;
		UcHlxSts	= TneCen( X2_DIR, StTneVal ) ;

	}
	if( UcHlySts - EXE_END )
	{
		SrvCon( X_DIR, ON ) ;
		SrvCon( Y_DIR, OFF ) ;
		WitTim( TNE ) ;

		RamWriteA( DAHLYB, INITBIASVAL ) ;		
		StTneVal.UlDwdVal	= TnePtp( Y_DIR , PTP_AFTER ) ;
		UcHlySts	= TneCen( Y2_DIR, StTneVal ) ;

	}
	
  #ifdef	NEUTRAL_CENTER
	TneHvc();
  #else	

	
	
	SrvCon( X_DIR, ON ) ;
	SrvCon( Y_DIR, ON ) ;
  #endif	
	
 #endif
	WitTim( TNE ) ;

	RamWriteA( ADHXOFF, StAdjPar.StHalAdj.UsHlxCna	) ;	 	
	RamWriteA( ADHYOFF, StAdjPar.StHalAdj.UsHlyCna	) ; 	

	RamReadA( DAHLXO, &StAdjPar.StHalAdj.UsHlxOff ) ;		
	RamReadA( DAHLXB, &StAdjPar.StHalAdj.UsHlxGan ) ;		
	RamReadA( DAHLYO, &StAdjPar.StHalAdj.UsHlyOff ) ;		
	RamReadA( DAHLYB, &StAdjPar.StHalAdj.UsHlyGan ) ;		
	RamReadA( ADHXOFF, &StAdjPar.StHalAdj.UsAdxOff ) ;		
	RamReadA( ADHYOFF, &StAdjPar.StHalAdj.UsAdyOff ) ;		

 #ifdef USE_EXE2PROM
	AdjSav( Y_DIR ) ;
	AdjSav( X_DIR ) ;
 #endif
	
	WitTim( TNE ) ;

	
	UcAtxSts	= LopGan( X_DIR ) ;
 #ifdef USE_EXE2PROM
	E2pWrt( LOOP_GAIN_STATUS_X,	2, ( unsigned char * )&StAdjPar.StLopGan.UsLxgSts ) ;
	E2pWrt( LOOP_GAIN_X,		2, ( unsigned char * )&StAdjPar.StLopGan.UsLxgVal ) ;
 #endif

	
	UcAtySts	= LopGan( Y_DIR ) ;
 #ifdef USE_EXE2PROM
	E2pWrt( LOOP_GAIN_STATUS_Y,	2, ( unsigned char * )&StAdjPar.StLopGan.UsLygSts ) ;
	E2pWrt( LOOP_GAIN_Y,		2, ( unsigned char * )&StAdjPar.StLopGan.UsLygVal ) ;
 #endif

	TneGvc() ;
 #ifdef USE_EXE2PROM
	E2pWrt( GYRO_OFFSET_STATUS_X,	2, ( unsigned char * )&StAdjPar.StGvcOff.UsGxoSts ) ;
	E2pWrt( GYRO_AD_OFFSET_X,	2, ( unsigned char * )&StAdjPar.StGvcOff.UsGxoVal ) ;
	E2pWrt( GYRO_OFFSET_STATUS_Y,	2, ( unsigned char * )&StAdjPar.StGvcOff.UsGyoSts ) ;
	E2pWrt( GYRO_AD_OFFSET_Y,	2, ( unsigned char * )&StAdjPar.StGvcOff.UsGyoVal ) ;
 #endif


	UsFinSts	= (unsigned short)( UcHlxSts - EXE_END ) + (unsigned short)( UcHlySts - EXE_END ) + (unsigned short)( UcAtxSts - EXE_END ) + (unsigned short)( UcAtySts - EXE_END ) + ( UsOscSts - (unsigned short)EXE_END ) + (unsigned short)EXE_END ;
 #ifdef USE_EXE2PROM
	
	E2pWrt( ADJ_COMP_FLAG,	2, ( unsigned char * )&UsFinSts ) ;
 #endif

	return( UsFinSts ) ;
}


 #ifndef	HALLADJ_HW

 #define		HALL_H_VAL	0x7FFF
 
unsigned long	TnePtp ( unsigned char	UcDirSel, unsigned char	UcBfrAft )
{
	UnDwdVal		StTneVal ;
	unsigned char	UcRegVal ;

	MesFil( THROUGH ) ;					


	if ( !UcDirSel ) {
		RamWriteA( wavxg , HALL_H_VAL );												
		SetSinWavePara( 0x0A , XHALWAVE ); 
		if( UcPwmMod == PWMMOD_PWM ) {
			RegReadA( LXX3, &UcRegVal ) ;
			RegWriteA( LXX3,  UcRegVal | 0x04 ) ;
		}
	}else{
		RamWriteA( wavyg , HALL_H_VAL );												
		SetSinWavePara( 0x0A , YHALWAVE ); 
		if( UcPwmMod == PWMMOD_PWM ) {
			RegReadA( LYX3, &UcRegVal ) ;
			RegWriteA( LYX3,  UcRegVal | 0x04 ) ;
		}
	}

	if ( !UcDirSel ) {					
		RegWriteA( MS1INADD, (unsigned char)HXIDAT ) ;
	} else {							
		RegWriteA( MS1INADD, (unsigned char)HYIDAT ) ;
	}

	BsyWit( MSMA, 0x02 ) ;				

	RegWriteA( MSMA, 0x00 ) ;			

	RamReadA( MSSMAXAV, &StTneVal.StDwdVal.UsHigVal ) ;
	RamReadA( MSSMINAV, &StTneVal.StDwdVal.UsLowVal ) ;

	if ( !UcDirSel ) {					
		SetSinWavePara( 0x00 , XHALWAVE ); 	
		if( UcPwmMod == PWMMOD_PWM ) {
			RegWriteA( LXX3,  UcRegVal ) ;
		}
	}else{
		SetSinWavePara( 0x00 , YHALWAVE ); 	
		if( UcPwmMod == PWMMOD_PWM ) {
			RegWriteA( LYX3,  UcRegVal ) ;
		}
	}

	if( UcBfrAft == 0 ) {
		if( UcDirSel == X_DIR ) {
			StAdjPar.StHalAdj.UsHlxCen	= ( ( signed short )StTneVal.StDwdVal.UsHigVal + ( signed short )StTneVal.StDwdVal.UsLowVal ) / 2 ;
			StAdjPar.StHalAdj.UsHlxMax	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlxMin	= StTneVal.StDwdVal.UsLowVal ;
		} else {
			StAdjPar.StHalAdj.UsHlyCen	= ( ( signed short )StTneVal.StDwdVal.UsHigVal + ( signed short )StTneVal.StDwdVal.UsLowVal ) / 2 ;
			StAdjPar.StHalAdj.UsHlyMax	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlyMin	= StTneVal.StDwdVal.UsLowVal ;
		}
	} else {
		if( UcDirSel == X_DIR ){
 #ifdef	NEUTRAL_CENTER
 #else	
			StAdjPar.StHalAdj.UsHlxCna	= ( ( signed short )StTneVal.StDwdVal.UsHigVal + ( signed short )StTneVal.StDwdVal.UsLowVal ) / 2 ;
 #endif	
			StAdjPar.StHalAdj.UsHlxMxa	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlxMna	= StTneVal.StDwdVal.UsLowVal ;
		} else {
 #ifdef	NEUTRAL_CENTER
 #else	
			StAdjPar.StHalAdj.UsHlyCna	= ( ( signed short )StTneVal.StDwdVal.UsHigVal + ( signed short )StTneVal.StDwdVal.UsLowVal ) / 2 ;
 #endif	
			StAdjPar.StHalAdj.UsHlyMxa	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlyMna	= StTneVal.StDwdVal.UsLowVal ;
		}
	}


	StTneVal.StDwdVal.UsHigVal	= 0x7fff - StTneVal.StDwdVal.UsHigVal ;		
	StTneVal.StDwdVal.UsLowVal	= StTneVal.StDwdVal.UsLowVal - 0x7fff ; 	

	
	return( StTneVal.UlDwdVal ) ;
}

#define		DIVVAL	2
unsigned char	TneCen( unsigned char	UcTneAxs, UnDwdVal	StTneVal )
{
	unsigned char 	UcTneRst, UcTmeOut, UcTofRst ;
	unsigned short	UsOffDif ;

	UsErrBia	= 0 ;
	UsErrOfs	= 0 ;
	UcTmeOut	= 1 ;
	UsStpSiz	= 1 ;
	UcTneRst	= FAILURE ;
	UcTofRst	= FAILURE ;

	while ( UcTneRst && UcTmeOut )
	{
		if( UcTofRst == FAILURE ) {
			StTneVal.UlDwdVal	= TneOff( StTneVal, UcTneAxs ) ;
		} else {
			StTneVal.UlDwdVal	= TneBia( StTneVal, UcTneAxs ) ;
			UcTofRst	= FAILURE ;
		}

		if ( StTneVal.StDwdVal.UsHigVal > StTneVal.StDwdVal.UsLowVal ) {									
			UsOffDif	= ( StTneVal.StDwdVal.UsHigVal - StTneVal.StDwdVal.UsLowVal ) / 2 ;
		} else {
			UsOffDif	= ( StTneVal.StDwdVal.UsLowVal - StTneVal.StDwdVal.UsHigVal ) / 2 ;
		}

		if( UsOffDif < MARJIN ) {
			UcTofRst	= SUCCESS ;
		} else {
			UcTofRst	= FAILURE ;
		}

		if ( ( StTneVal.StDwdVal.UsHigVal < HALL_MIN_GAP && StTneVal.StDwdVal.UsLowVal < HALL_MIN_GAP )		
		&& ( StTneVal.StDwdVal.UsHigVal > HALL_MAX_GAP && StTneVal.StDwdVal.UsLowVal > HALL_MAX_GAP ) ) {
			UcTneRst	= SUCCESS ;
			break ;
		} else if ( UsStpSiz == 0 ) {
			UcTneRst	= SUCCESS ;
			break ;
		} else {
			UcTneRst	= FAILURE ;
			UcTmeOut++ ;
		}

		if( UcTneAxs & 0xF0 )
		{
			if ( ( UcTmeOut / DIVVAL ) == TIME_OUT ) {
				UcTmeOut	= 0 ;
			}		 																							
		}else{
			if ( UcTmeOut == TIME_OUT ) {
				UcTmeOut	= 0 ;
			}		 																							
		}
	}

	if( UcTneRst == FAILURE ) {
		if( !( UcTneAxs & 0x0F ) ) {
			UcTneRst					= EXE_HXADJ ;
			StAdjPar.StHalAdj.UsHlxGan	= 0xFFFF ;
			StAdjPar.StHalAdj.UsHlxOff	= 0xFFFF ;
		} else {
			UcTneRst					= EXE_HYADJ ;
			StAdjPar.StHalAdj.UsHlyGan	= 0xFFFF ;
			StAdjPar.StHalAdj.UsHlyOff	= 0xFFFF ;
		}
	} else {
		UcTneRst	= EXE_END ;
	}

	return( UcTneRst ) ;
}



unsigned long	TneBia( UnDwdVal	StTneVal, unsigned char	UcTneAxs )
{
	long					SlSetBia ;
	unsigned short			UsSetBia ;
	unsigned char			UcChkFst ;
	static unsigned short	UsTneVax ;							

	UcChkFst	= 1 ;

	if ( UsStpSiz == 1) {
		UsTneVax	= 2 ;

		if ( ( StTneVal.StDwdVal.UsHigVal + StTneVal.StDwdVal.UsLowVal ) / 2 < BIAS_ADJ_BORDER ) {
			UcChkFst	= 0 ;
		}

		if( ( UcTneAxs & 0xF0 ) &&	UcChkFst )
		{
			;
		}else{
		
			if ( !UcTneAxs ) {										
				RamWriteA( DAHLXB, 0x8001 ) ; 				
				RamWriteA( DAHLXO, 0x0000 ) ;				
				UsStpSiz	= BIAS_LIMIT / UsTneVax ;
			} else {
				RamWriteA( DAHLYB, 0x8001 ) ; 				
				RamWriteA( DAHLYO, 0x0000 ) ;				
				UsStpSiz	= BIAS_LIMIT / UsTneVax ;
			}
		}
	}

	if ( !( UcTneAxs & 0x0F ) ) {
		RamReadA( DAHLXB, &UsSetBia ) ;					
		SlSetBia	= ( long )UsSetBia ;
	} else {
		RamReadA( DAHLYB, &UsSetBia ) ;					
		SlSetBia	= ( long )UsSetBia ;
	}

	if( SlSetBia > 0x00008000 ) {
		SlSetBia	|= 0xFFFF0000 ;
	}

	if( UcChkFst ) {
		if( UcTneAxs & 0xF0 )
		{

			if ( ( StTneVal.StDwdVal.UsHigVal + StTneVal.StDwdVal.UsLowVal ) / 2 > BIAS_ADJ_BORDER ) {	
				SlSetBia	+= 0x0100 ;
			} else {
				SlSetBia	-= 0x0100 ;
			}
			UsStpSiz	= 0x0200 ;
			
		}else{
		
			if ( ( StTneVal.StDwdVal.UsHigVal + StTneVal.StDwdVal.UsLowVal ) / 2 > BIAS_ADJ_BORDER ) {	
				SlSetBia	+= UsStpSiz ;
			} else {
				SlSetBia	-= UsStpSiz ;
			}

			UsTneVax	= UsTneVax * 2 ;
			UsStpSiz	= BIAS_LIMIT / UsTneVax ;
			
		}
	}

	if( SlSetBia > ( long )0x00007FFF ) {
		SlSetBia	= 0x00007FFF ;
	} else if( SlSetBia < ( long )0xFFFF8001 ) {
		SlSetBia	= 0xFFFF8001 ;
	}

	if ( !( UcTneAxs & 0x0F ) ) {
		RamWriteA( DAHLXB, SlSetBia ) ;		
	} else {
		RamWriteA( DAHLYB, SlSetBia ) ;		
	}

	StTneVal.UlDwdVal	= TnePtp( UcTneAxs & 0x0F , PTP_AFTER ) ;

	return( StTneVal.UlDwdVal ) ;
}



unsigned long	TneOff( UnDwdVal	StTneVal, unsigned char	UcTneAxs )
{
	long			SlSetOff ;
	unsigned short	UsSetOff ;

	
	UcTneAxs &= 0x0F ;
	
	
	if ( !UcTneAxs ) {																			
		RamReadA( DAHLXO, &UsSetOff ) ;															
		SlSetOff	= ( long )UsSetOff ;
	} else {
		RamReadA( DAHLYO, &UsSetOff ) ;															
		SlSetOff	= ( long )UsSetOff ;
	}

	if( SlSetOff > 0x00008000 ) {
		SlSetOff	|= 0xFFFF0000 ;
	}

	if ( StTneVal.StDwdVal.UsHigVal > StTneVal.StDwdVal.UsLowVal ) {
		SlSetOff	+= ( StTneVal.StDwdVal.UsHigVal - StTneVal.StDwdVal.UsLowVal ) / OFFSET_DIV ;	
	} else {
		SlSetOff	-= ( StTneVal.StDwdVal.UsLowVal - StTneVal.StDwdVal.UsHigVal ) / OFFSET_DIV ;	
	}

	if( SlSetOff > ( long )0x00007FFF ) {
		SlSetOff	= 0x00007FFF ;
	} else if( SlSetOff < ( long )0xFFFF8001 ) {
		SlSetOff	= 0xFFFF8001 ;
	}

	if ( !UcTneAxs ) {
		RamWriteA( DAHLXO, SlSetOff ) ;		
	} else {
		RamWriteA( DAHLYO, SlSetOff ) ;		
	}

	StTneVal.UlDwdVal	= TnePtp( UcTneAxs, PTP_AFTER ) ;

	return( StTneVal.UlDwdVal ) ;
}

 #endif
#endif
void	MesFil( unsigned char	UcMesMod )
{
	if( !UcMesMod ) {								
		
		RamWriteA( ms1aa, 0x0285 ) ;		
		RamWriteA( ms1ab, 0x0285 ) ;		
		RamWriteA( ms1ac, 0x7AF5 ) ;		
		RamWriteA( ms1ad, 0x0000 ) ;		
		RamWriteA( ms1ae, 0x0000 ) ;		
		RamWriteA( ms1ba, 0x7FFF ) ;		
		RamWriteA( ms1bb, 0x0000 ) ;		
		RamWriteA( ms1bc, 0x0000 ) ;		
		RamWriteA( ms1bd, 0x0000 ) ;		
		RamWriteA( ms1be, 0x0000 ) ;		

		RegWriteA( MSF1SOF, 0x00 ) ;		
		
		
		RamWriteA( ms2aa, 0x0285 ) ;		
		RamWriteA( ms2ab, 0x0285 ) ;		
		RamWriteA( ms2ac, 0x7AF5 ) ;		
		RamWriteA( ms2ad, 0x0000 ) ;		
		RamWriteA( ms2ae, 0x0000 ) ;		
		RamWriteA( ms2ba, 0x7FFF ) ;		
		RamWriteA( ms2bb, 0x0000 ) ;		
		RamWriteA( ms2bc, 0x0000 ) ;		
		RamWriteA( ms2bd, 0x0000 ) ;		
		RamWriteA( ms2be, 0x0000 ) ;		

		RegWriteA( MSF2SOF, 0x00 ) ;		
		
	} else if( UcMesMod == LOOPGAIN ) {				
		
		RamWriteA( ms1aa, 0x0F21 ) ;		
		RamWriteA( ms1ab, 0x0F21 ) ;		
		RamWriteA( ms1ac, 0x61BD ) ;		
		RamWriteA( ms1ad, 0x0000 ) ;		
		RamWriteA( ms1ae, 0x0000 ) ;		
		RamWriteA( ms1ba, 0x7F7D ) ;		
		RamWriteA( ms1bb, 0x8083 ) ;		
		RamWriteA( ms1bc, 0x7EF9 ) ;		
		RamWriteA( ms1bd, 0x0000 ) ;		
		RamWriteA( ms1be, 0x0000 ) ;		

		RegWriteA( MSF1SOF, 0x00 ) ;		
		
		
		RamWriteA( ms2aa, 0x0F21 ) ;		
		RamWriteA( ms2ab, 0x0F21 ) ;		
		RamWriteA( ms2ac, 0x61BD ) ;		
		RamWriteA( ms2ad, 0x0000 ) ;		
		RamWriteA( ms2ae, 0x0000 ) ;		
		RamWriteA( ms2ba, 0x7F7D ) ;		
		RamWriteA( ms2bb, 0x8083 ) ;		
		RamWriteA( ms2bc, 0x7EF9 ) ;		
		RamWriteA( ms2bd, 0x0000 ) ;		
		RamWriteA( ms2be, 0x0000 ) ;		

		RegWriteA( MSF2SOF, 0x00 ) ;		
		
	} else if( UcMesMod == THROUGH ) {				
		
		RamWriteA( ms1aa, 0x7FFF ) ;		
		RamWriteA( ms1ab, 0x0000 ) ;		
		RamWriteA( ms1ac, 0x0000 ) ;		
		RamWriteA( ms1ad, 0x0000 ) ;		
		RamWriteA( ms1ae, 0x0000 ) ;		
		RamWriteA( ms1ba, 0x7FFF ) ;		
		RamWriteA( ms1bb, 0x0000 ) ;		
		RamWriteA( ms1bc, 0x0000 ) ;		
		RamWriteA( ms1bd, 0x0000 ) ;		
		RamWriteA( ms1be, 0x0000 ) ;		

		RegWriteA( MSF1SOF, 0x00 ) ;		
		
		
		RamWriteA( ms2aa, 0x7FFF ) ;		
		RamWriteA( ms2ab, 0x0000 ) ;		
		RamWriteA( ms2ac, 0x0000 ) ;		
		RamWriteA( ms2ad, 0x0000 ) ;		
		RamWriteA( ms2ae, 0x0000 ) ;		
		RamWriteA( ms2ba, 0x7FFF ) ;		
		RamWriteA( ms2bb, 0x0000 ) ;		
		RamWriteA( ms2bc, 0x0000 ) ;		
		RamWriteA( ms2bd, 0x0000 ) ;		
		RamWriteA( ms2be, 0x0000 ) ;		

		RegWriteA( MSF2SOF, 0x00 ) ;		
		
	} else if( UcMesMod == NOISE ) {				
		
		RamWriteA( ms1aa, 0x0303 ) ;		
		RamWriteA( ms1ab, 0x0303 ) ;		
		RamWriteA( ms1ac, 0x79F8 ) ;		
		RamWriteA( ms1ad, 0x0000 ) ;		
		RamWriteA( ms1ae, 0x0000 ) ;		
		RamWriteA( ms1ba, 0x0303 ) ;		
		RamWriteA( ms1bb, 0x0303 ) ;		
		RamWriteA( ms1bc, 0x79F8 ) ;		
		RamWriteA( ms1bd, 0x0000 ) ;		
		RamWriteA( ms1be, 0x0000 ) ;		

		RegWriteA( MSF1SOF, 0x00 ) ;		
	
		
		RamWriteA( ms2aa, 0x0303 ) ;		
		RamWriteA( ms2ab, 0x0303 ) ;		
		RamWriteA( ms2ac, 0x79F8 ) ;		
		RamWriteA( ms2ad, 0x0000 ) ;		
		RamWriteA( ms2ae, 0x0000 ) ;		
		RamWriteA( ms2ba, 0x0303 ) ;		
		RamWriteA( ms2bb, 0x0303 ) ;		
		RamWriteA( ms2bc, 0x79F8 ) ;		
		RamWriteA( ms2bd, 0x0000 ) ;		
		RamWriteA( ms2be, 0x0000 ) ;		

		RegWriteA( MSF2SOF, 0x00 ) ;		
		
	}
}



void	SrvCon( unsigned char	UcDirSel, unsigned char	UcSwcCon )
{
	if( UcSwcCon ) {
		if( !UcDirSel ) {									
			RegWriteA( LXEQEN, 0xC5 ) ;				
			RamWriteA( lxggf, 0x0000 ) ;			
		} else {											
			RegWriteA( LYEQEN, 0xC5 ) ;				
			RamWriteA( lyggf, 0x0000 ) ;			
		}
	} else {
		if( !UcDirSel ) {									
			RegWriteA( LXEQEN, 0x45 ) ;				
			RamWriteA( LXDODAT, 0x0000 ) ;			
		} else {											
			RegWriteA( LYEQEN, 0x45 ) ;				
			RamWriteA( LYDODAT, 0x0000 ) ;			
		}
	}
}



#ifdef		MODULE_CALIBRATION		
unsigned char	LopGan( unsigned char	UcDirSel )
{
	unsigned char	UcLpAdjSts ;
	
 #ifdef	HALLADJ_HW
	UcLpAdjSts	= LoopGainAdj( UcDirSel ) ;
 #else
	MesFil( LOOPGAIN ) ;

	
	SrvCon( X_DIR, ON ) ;
	SrvCon( Y_DIR, ON ) ;

	
	WitTim( 300 ) ;

	
	LopIni( UcDirSel ) ;

	
	UcLpAdjSts	= LopAdj( UcDirSel ) ;
 #endif
	

	if( !UcLpAdjSts ) {
		return( EXE_END ) ;
	} else {
		if( !UcDirSel ) {
			return( EXE_LXADJ ) ;
		} else {
			return( EXE_LYADJ ) ;
		}
	}
}



 #ifndef	HALLADJ_HW
void	LopIni( unsigned char	UcDirSel )
{
	
	LopPar( UcDirSel ) ;

	
	LopSin( UcDirSel, ON ) ;

}
 #endif


void	LopPar( unsigned char	UcDirSel )
{
	unsigned short	UsLopGan ;

	if( !UcDirSel ) {
		UsLopGan	= LXGAIN_LOP ;
		RamWriteA( lxgain, UsLopGan ) ;			
	} else {
		UsLopGan	= LYGAIN_LOP ;
		RamWriteA( lygain, UsLopGan ) ;			
	}
}



 #ifndef	HALLADJ_HW
void	LopSin( unsigned char	UcDirSel, unsigned char	UcSonOff )
{
	if( UcSonOff ) {
		RegWriteA( SWFC1, 0x36 ) ;								

		if( !UcDirSel ) {
			RegWriteA( SINXADD, 0x00 ) ;			
			RegWriteA( SINYADD, 0x00 ) ;			
			
			RegWriteA( SWSEL, 0x80 ) ;							
			RegWriteA( SWEN, 0xC0 ) ;							
			RegWriteA( SWFC2, 0x48 ) ;							
			if( UcPwmMod == PWMMOD_CVL ) {
				RamWriteA( lxxg, 0x198A ) ;							
			}else{
				RamWriteA( lxxg, 0x32F5 ) ;							
			}
		} else {
			RegWriteA( SINXADD, 0x00 ) ;			
			RegWriteA( SINYADD, 0x00 ) ;			
			
			RegWriteA( SWSEL, 0x40 ) ;							
			RegWriteA( SWEN, 0xC0 ) ;							
			RegWriteA( SWFC2, 0x48 ) ;							
			if( UcPwmMod == PWMMOD_CVL ) {
				RamWriteA( lyxg, 0x198A ) ;							
			}else{
				RamWriteA( lyxg, 0x32F5 ) ;							
			}
		}
	} else {
		RegWriteA( SWEN, 0x00 ) ;								
		if( !UcDirSel ) {
			RamWriteA( lxxg, 0x0000 ) ;							
		} else {
					RamWriteA( lyxg, 0x0000 ) ;					
		}
	}
}



unsigned char	LopAdj( unsigned char	UcDirSel )
{
	unsigned char	UcAdjSts	= FAILURE ;
	unsigned long	UlAdcXg1 ;
	unsigned long	UlAdcXg2 ;
	unsigned short 	UsRtnVal ;
#if 0
	float			SfCmpVal ;
#else
	long			SfCmpVal;
#endif
	unsigned char	UcIdxCnt ;
	unsigned char	UcIdxCn1 ;
	unsigned char	UcIdxCn2 ;

	unsigned short	UsGanVal[ 5 ] ;
	unsigned short	UsTemVal ;

	if( !UcDirSel ) {
		RegWriteA( MS1INADD, 0x46 ) ;											
		RegWriteA( MS2INADD, 0x47 ) ;											
	} else {
		RegWriteA( MS1INADD, 0x86 ) ;											
		RegWriteA( MS2INADD, 0x87 ) ;											
	}

	
	for( UcIdxCnt = 0 ; UcIdxCnt < 5 ; UcIdxCnt++ )
	{
		LopMes( ) ;																

		UlAdcXg1	= GinMes( MES_XG1 ) ;										
		UlAdcXg2	= GinMes( MES_XG2 ) ;										

#if 0
		SfCmpVal	= ( float )UlAdcXg2 / ( float )UlAdcXg1 ;					
#else
		SfCmpVal	= (UlAdcXg2*1000) / UlAdcXg1 ;					
#endif

		if( !UcDirSel ) {
			RamReadA( lxgain, &UsRtnVal ) ;									
		} else {
			RamReadA( lygain, &UsRtnVal ) ;									
		}

#if 0
		UsRtnVal	= ( unsigned short )( ( float )UsRtnVal * SfCmpVal ) ;
#else
		UsRtnVal	= ( unsigned short )( (UsRtnVal * SfCmpVal) / 1000) ;
#endif

		UsGanVal[ UcIdxCnt ]	= UsRtnVal ;
	}

	for( UcIdxCn1 = 0 ; UcIdxCn1 < 4 ; UcIdxCn1++ )
	{
		for( UcIdxCn2 = UcIdxCn1+1 ; UcIdxCn2 < 5 ; UcIdxCn2++ )
		{
			if( UsGanVal[ UcIdxCn1 ] > UsGanVal[ UcIdxCn2 ] ) {
				UsTemVal				= UsGanVal[ UcIdxCn1 ] ;
				UsGanVal[ UcIdxCn1 ]	= UsGanVal[ UcIdxCn2 ] ;
				UsGanVal[ UcIdxCn2 ]	= UsTemVal ;
			}
		}
	}

	UsRtnVal	= ( unsigned short )( ( ( long )UsGanVal[ 1 ] + ( long )UsGanVal[ 2 ] + ( long )UsGanVal[ 3 ] ) / 3 ) ;

	LopSin( UcDirSel, OFF ) ;

	if( UsRtnVal < 0x8000 ) {												
		UcAdjSts	= SUCCESS ;												
	}

	if( UcAdjSts ) {
		if( !UcDirSel ) {
			RamWriteA( lxgain, 0x7FFF ) ;							
			StAdjPar.StLopGan.UsLxgVal	= 0x7FFF ;
			StAdjPar.StLopGan.UsLxgSts	= 0x0000 ;
		} else {
			RamWriteA( lygain, 0x7FFF ) ;							
			StAdjPar.StLopGan.UsLygVal	= 0x7FFF ;
			StAdjPar.StLopGan.UsLygSts	= 0x0000 ;
		}
	} else {
		if( !UcDirSel ) {
			RamWriteA( lxgain, UsRtnVal ) ;							
			StAdjPar.StLopGan.UsLxgVal	= UsRtnVal ;
			StAdjPar.StLopGan.UsLxgSts	= 0xFFFF ;
		} else {
			RamWriteA( lygain, UsRtnVal ) ;							
			StAdjPar.StLopGan.UsLygVal	= UsRtnVal ;
			StAdjPar.StLopGan.UsLygSts	= 0xFFFF ;
		}
	}
	return( UcAdjSts ) ;
}


void	LopMes( void )
{
	BsyWit( DLYCLR2, 0xC0 ) ;							
	RegWriteA( MSMPLNSH, 0x03 ) ;						
	RegWriteA( MSMPLNSL, 0xFF ) ;						
	RegWriteA( MSF1EN, 0x01 ) ;							
	RegWriteA( MSF2EN, 0x01 ) ;							
	BsyWit( MSMA, 0xE1 ) ;								

	RegWriteA( MSMA, 0x00 ) ;							
	RegWriteA( MSF1EN, 0x00 ) ;							
	RegWriteA( MSF2EN, 0x00 ) ;							
}


unsigned long	GinMes( unsigned char	UcXg1Xg2 )
{
	unsigned short	UsMesVll, UsMesVlh ;
	unsigned long	UlMesVal ;

	if( !UcXg1Xg2 ) {
		RamReadA( MSAVAC1L, &UsMesVll ) ;			
		RamReadA( MSAVAC1H, &UsMesVlh ) ;			
	} else {
		RamReadA( MSAVAC2L, &UsMesVll ) ;			
		RamReadA( MSAVAC2H, &UsMesVlh ) ;			
	}

	UlMesVal	= ( unsigned long )( ( ( unsigned long )UsMesVlh << 16 ) | UsMesVll ) ;

	return( UlMesVal ) ;
}

 #endif


#define	LIMITH		0x0FA0
#define	LIMITL		0xF060
#define	INITVAL		0x0000

unsigned char	TneGvc( void )
{
	unsigned char  UcRsltSts;
	
	
	
	
	RamWriteA( ADGXOFF, 0x0000 ) ;	
	RamWriteA( ADGYOFF, 0x0000 ) ;	
	RegWriteA( IZAH,	(unsigned char)(INITVAL >> 8) ) ;	
	RegWriteA( IZAL,	(unsigned char)INITVAL ) ;			
	RegWriteA( IZBH,	(unsigned char)(INITVAL >> 8) ) ;	
	RegWriteA( IZBL,	(unsigned char)INITVAL ) ;			
	
	RegWriteA( GDLYMON10, 0xF5 ) ;		
	RegWriteA( GDLYMON11, 0x01 ) ;		
	RegWriteA( GDLYMON20, 0xF5 ) ;		
	RegWriteA( GDLYMON21, 0x00 ) ;		
	MesFil( THROUGH ) ;				
	RegWriteA( MSF1EN, 0x01 ) ;			
	
	
	
	BsyWit( DLYCLR2, 0x80 ) ;			
	StAdjPar.StGvcOff.UsGxoVal = (unsigned short)GenMes( GYRMON1, 0 );		
	RegWriteA( IZAH, (unsigned char)(StAdjPar.StGvcOff.UsGxoVal >> 8) ) ;	
	RegWriteA( IZAL, (unsigned char)(StAdjPar.StGvcOff.UsGxoVal) ) ;		
	
	
	
	BsyWit( DLYCLR2, 0x80 ) ;			
	StAdjPar.StGvcOff.UsGyoVal = (unsigned short)GenMes( GYRMON2, 0 );		
	RegWriteA( IZBH, (unsigned char)(StAdjPar.StGvcOff.UsGyoVal >> 8) ) ;	
	RegWriteA( IZBL, (unsigned char)(StAdjPar.StGvcOff.UsGyoVal) ) ;		
	
	RegWriteA( MSF1EN, 0x00 ) ;			
	
	UcRsltSts = EXE_END ;						

	StAdjPar.StGvcOff.UsGxoSts	= 0xFFFF ;
	if(( (short)StAdjPar.StGvcOff.UsGxoVal < (short)LIMITL ) || ( (short)StAdjPar.StGvcOff.UsGxoVal > (short)LIMITH ))
	{
		UcRsltSts |= EXE_GXADJ ;
		StAdjPar.StGvcOff.UsGxoSts	= 0x0000 ;
	}
	
	StAdjPar.StGvcOff.UsGyoSts	= 0xFFFF ;
	if(( (short)StAdjPar.StGvcOff.UsGyoVal < (short)LIMITL ) || ( (short)StAdjPar.StGvcOff.UsGyoVal > (short)LIMITH ))
	{
		UcRsltSts |= EXE_GYADJ ;
		StAdjPar.StGvcOff.UsGyoSts	= 0x0000 ;
	}
	return( UcRsltSts );
		
}

#endif

unsigned char	RtnCen( unsigned char	UcCmdPar )
{
	unsigned char	UcCmdSts ;

	UcCmdSts	= EXE_END ;

	GyrCon( OFF ) ;											

	if( !UcCmdPar ) {										

		StbOnn() ;											
		
	} else if( UcCmdPar == 0x01 ) {							

		SrvCon( X_DIR, ON ) ;								
		SrvCon( Y_DIR, OFF ) ;
	} else if( UcCmdPar == 0x02 ) {							

		SrvCon( X_DIR, OFF ) ;								
		SrvCon( Y_DIR, ON ) ;
	}

	return( UcCmdSts ) ;
}



void	GyrCon( unsigned char	UcGyrCon )
{

	
	RegWriteA( GSHTON, 0x00 ) ;									

	if( UcGyrCon ) {													

		ClrGyr( 0x02 , CLR_GYR_DLY_RAM );			
		
		RamWriteA( lxggf, 0x7fff ) ;	
		RamWriteA( lyggf, 0x7fff ) ;	
		
#ifdef	GAIN_CONT
		
		AutoGainControlSw( ON ) ;											
#endif
	} else {															

#ifdef	GAIN_CONT
		
		AutoGainControlSw( OFF ) ;											
#endif
	}
}



void	OisEna( void )
{
	
	SrvCon( X_DIR, ON ) ;
	SrvCon( Y_DIR, ON ) ;

	GyrCon( ON ) ;
}



void	TimPro( void )
{
	if( UcOscAdjFlg )
	{
		if( UcOscAdjFlg == MEASSTR )
		{
			RegWriteA( OSCCNTEN, 0x01 ) ;		
			UcOscAdjFlg = MEASCNT ;
		}
		else if( UcOscAdjFlg == MEASCNT )
		{
			RegWriteA( OSCCNTEN, 0x00 ) ;		
			UcOscAdjFlg = MEASFIX ;
		}
	}
}



void	S2cPro( unsigned char uc_mode )
{
	if( uc_mode == 1 )
	{
#ifdef H1COEF_CHANGER
		SetH1cMod( S2MODE ) ;							
#endif
		RegWriteA( G2NDCEFON0, 0x04 ) ;											
		
		RegWriteA( GSHTON, 0x11 ) ;												

	}
	else
	{
		
		RegWriteA( GSHTON, 0x00 ) ;												
		RegWriteA( G2NDCEFON0, 0x00 ) ;											
#ifdef H1COEF_CHANGER
		SetH1cMod( UcH1LvlMod ) ;							
#endif

	}
	
}


short	GenMes( unsigned short	UsRamAdd, unsigned char	UcMesMod )
{
	short	SsMesRlt ;

	RegWriteA( MS1INADD, ( unsigned char )( UsRamAdd & 0x00ff ) ) ;	

	if( !UcMesMod ) {
		RegWriteA( MSMPLNSH, 0x00 ) ;									
		RegWriteA( MSMPLNSL, 0x3F ) ;									
	} else {
		RegWriteA( MSMPLNSH, 0x00 ) ;									
		RegWriteA( MSMPLNSL, 0x00 ) ;									
	}

	BsyWit( MSMA, 0x01 ) ;											

	RegWriteA( MSMA, 0x00 ) ;										

	RamReadA( MSAV1, ( unsigned short * )&SsMesRlt ) ;				

	return( SsMesRlt ) ;
}


	
	
	
	
	
	
const unsigned char	CucFreqVal[ 17 ]	= {
		0xF0,				
		0x0E,				
		0x1E,				
		0x19,				
		0x3E,				
		0x02,				
		0x14,				
		0x5C,				
		0x7E,				
		0x6B,				
		0x12,				
		0x46,				
		0xAD,				
		0x56,				
		0xAB,				
		0x00,				
		0x3E				
	} ;
	
void	SetSinWavePara( unsigned char UcTableVal ,	unsigned char UcMethodVal )
{
	unsigned char	UcFreqDat ;
	unsigned char	UcMethod ;


	if(UcTableVal > 0x10 )
		UcTableVal = 0x10 ;			
	UcFreqDat = CucFreqVal[ UcTableVal ] ;	
	
	if( UcMethodVal == SINEWAVE) {
		UcMethod = 0x4C ;			
	}else if( UcMethodVal == CIRCWAVE ){
		UcMethod = 0xCC ;			
	}else{
		UcMethod = 0x4C ;			
	}
	
	if(( UcMethodVal != XHALWAVE ) && ( UcMethodVal != YHALWAVE )) {
		MesFil( NOISE ) ;			
	}
	
	if( UcFreqDat == 0xF0 )			
	{
		RegWriteA( SINXADD, 0x00 ) ;			
		RegWriteA( SINYADD, 0x00 ) ;			
		
		RegWriteA( MS1OUTADD, 0x00 ) ;			
		RegWriteA( MS2OUTADD, 0x00 ) ;			
		RegWriteA( SWFC1, 0x1D ) ;				
		RegWriteA( SWEN, 0x00 ) ;				
		RegWriteA( SWSEL, 0x00 ) ;				
		RegWriteA( SWFC2, 0x08 ) ;				
		RamWriteA( LXDX, 0x0000 ) ;				
		RamWriteA( LYDX, 0x0000 ) ;				
		if(( UcMethodVal != XHALWAVE ) && ( UcMethodVal != YHALWAVE )) {
			RamWriteA( HXINOD, UsCntXof ) ;			
			RamWriteA( HYINOD, UsCntYof ) ;			
		}else{
			RamWriteA( LXDODAT, 0x0000 ) ;			
			RamWriteA( LYDODAT, 0x0000 ) ;			
		}
		RegWriteA( MSFDS, 0x00 ) ;				
		RegWriteA( MSF1EN, 0x00 ) ;				
		RegWriteA( MSF2EN, 0x00 ) ;				
	}
	else
	{
		RegWriteA( SINXADD, 0x00 ) ;			
		RegWriteA( SINYADD, 0x00 ) ;			
		
		RegWriteA( SWFC1, UcFreqDat ) ;			
		RegWriteA( SWFC2, UcMethod ) ;			
		RegWriteA( SWSEL, 0xC0 ) ;				
		RegWriteA( SWEN, 0x80 ) ;				
		
		if(( UcMethodVal != XHALWAVE ) && ( UcMethodVal != YHALWAVE )) {
			RegWriteA( MS1INADD, 0x48 ) ;			
			RegWriteA( MS1OUTADD, 0x27 ) ;			
			RegWriteA( MS2INADD, 0x88 ) ;			
			RegWriteA( MS2OUTADD, 0x67 ) ;			
		}else{
			if( UcMethodVal == XHALWAVE ){
				RegWriteA( SINXADD , (unsigned char)LXDODAT );							
			}else{
				RegWriteA( SINYADD , (unsigned char)LYDODAT );							
			}
			RegWriteA( MSMPLNSL, 0x00 ) ;			
			RegWriteA( MSMPLNSH, 0x00 ) ;			
		}
		
		RegWriteA( MSFDS, 0x01 ) ;				
		RegWriteA( MSF1EN, 0x81 ) ;				
		RegWriteA( MSF2EN, 0x81 ) ;				
		
	}
	
	
}

#ifdef USE_EXE2PROM
void	E2pRed( unsigned short UsAdr, unsigned char UcLen, unsigned char *UcPtr )
{
	unsigned char UcAdh = UsAdr >> 8 ;
	unsigned char UcAdl = UsAdr & 0xFF ;
	unsigned char UcCnt ;

#ifdef I2CE2PROM
	RegWriteA( BSYSEL,	0x06 ) ;		
	RegWriteA( E2L,		UcAdl ) ;		
	RegWriteA( E2H,		UcAdh ) ;		
	
	RegWriteA( E2ACC, UcLen );			

	Bsy2Wit( ) ;						

	for( UcCnt = 0; UcCnt < UcLen; UcCnt++ ) {
		RegReadA( E2DAT0 + UcCnt, &UcPtr[ UcCnt ] ) ;	
		
	}
#endif
#ifdef SPIE2PROM
	RegWriteA( BSYSEL,	0x07 ) ;		
	RegWriteA( E2L,		UcAdl ) ;		
	RegWriteA( E2H,		UcAdh ) ;		
	
	RegWriteA( E2ACCR, UcLen << 1 );	

	Bsy2Wit( ) ;						

	for( UcCnt = 0; UcCnt < UcLen; UcCnt++ ) {
		RegReadA( E2DAT0 + UcCnt, &UcPtr[ UcCnt ] ) ;	
	}
#endif

}



void	E2pWrt( unsigned short UsAdr, unsigned char UcLen, unsigned char *UcPtr )
{
	unsigned char UcAdh = UsAdr >> 8 ;
	unsigned char UcAdl = UsAdr & 0xFF ;
	unsigned char UcWrtFlg ;
	unsigned char UcCnt ;

#ifdef I2CE2PROM		
	RegWriteA( BSYSEL,	0x06 ) ;		
	RegWriteA( E2L,		UcAdl ) ;		
	RegWriteA( E2H,		UcAdh ) ;		

										
	for( UcCnt = 0; UcCnt < UcLen; UcCnt++ ) {
		RegWriteA( E2DAT0 + UcCnt, UcPtr[ UcCnt ] ) ;	
	}

	UcWrtFlg = UcLen * 16 ;				

	RegWriteA( E2ACC, UcWrtFlg ) ;		

	Bsy2Wit( ) ;						

#endif

#ifdef SPIE2PROM		
	
	if(( UsAdr & 0x1FFF) != 0x0000 )
	{
		RegWriteA( BSYSEL,	0x07 ) ;		
		RegWriteA( E2L,		UcAdl ) ;		
		RegWriteA( E2H,		UcAdh ) ;		

											
		for( UcCnt = 0; UcCnt < UcLen; UcCnt++ ) {
			RegWriteA( E2DAT0 + UcCnt, UcPtr[ UcCnt ] ) ;	
		}

		UcWrtFlg = UcLen << 1 ;				

		RegWriteA( E2ACCW, UcWrtFlg ) ;		

		Bsy2Wit( ) ;						
	}

#endif
}



void	AdjSav( unsigned char	UcAxsSel )
{
	unsigned char *	NuAdjPar ;
	unsigned short	UsEepAdd ;


	switch( UcAxsSel )
	{
		case X_DIR :																	
			NuAdjPar	= ( unsigned char * )&StAdjPar.StHalAdj.UsHlxCna ;
			for( UsEepAdd = CENTER_HALL_AD_X ; UsEepAdd < CENTER_HALL_AD_Y ; UsEepAdd	+= 2 )
			{
				E2pWrt( UsEepAdd, 2, NuAdjPar ) ;
				NuAdjPar	+= 2 ;
			}
			break ;
		case Y_DIR :																	
			NuAdjPar	= ( unsigned char * )&StAdjPar.StHalAdj.UsHlyCna ;
			for( UsEepAdd = CENTER_HALL_AD_Y ; UsEepAdd < LOOP_GAIN_STATUS_X ; UsEepAdd	+= 2 )
			{
				E2pWrt( UsEepAdd, 2, NuAdjPar ) ;
				NuAdjPar	+= 2 ;
			}
			break ;
		default :
			break ;
	}
}
#endif


#ifdef STANDBY_MODE
void	SetStandby( unsigned char UcContMode )
{
	switch(UcContMode)
	{
	case STB1_ON:
		RegWriteA( STBB, 0x00 ) ;			
		RegWriteA( PWMA, 0x00 ) ;			
		RegWriteA( LNA,  0x00 ) ;			
		DrvSw( OFF ) ;						
		RegWriteA( PWMMONFC, 0x00 ) ;		
		RegWriteA( DAMONFC, 0x00 ) ;		
		SelectGySleep( ON ) ;				
		break ;
	case STB1_OFF:
		SelectGySleep( OFF ) ;				
		RegWriteA( DAMONFC, 0x00 ) ;		
		RegWriteA( PWMMONFC, 0x80 ) ;		
		DrvSw( ON ) ;						
		RegWriteA( LNA,  0xC0 ) ;			
		RegWriteA( PWMA, 0xC0 ) ;			
		RegWriteA( STBB, 0x0F ) ;			
		break ;
	case STB2_ON:
		RegWriteA( STBB, 0x00 ) ;			
		RegWriteA( PWMA, 0x00 ) ;			
		RegWriteA( LNA,  0x00 ) ;			
		DrvSw( OFF ) ;						
		RegWriteA( PWMMONFC, 0x00 ) ;		
		RegWriteA( DAMONFC, 0x00 ) ;		
		SelectGySleep( ON ) ;				
		RegWriteA( CLKON, 0x00 ) ;			
		break ;
	case STB2_OFF:
#ifdef I2CE2PROM
		RegWriteA( CLKON, 0x33 ) ;			
#else
 #ifdef SPIE2PROM
		RegWriteA( CLKON, 0x17 ) ;			
 #else
		RegWriteA( CLKON, 0x13 ) ;			
 #endif
#endif
		SelectGySleep( OFF ) ;				
		RegWriteA( DAMONFC, 0x00 ) ;		
		RegWriteA( PWMMONFC, 0x80 ) ;		
		DrvSw( ON ) ;						
		RegWriteA( LNA,  0xC0 ) ;			
		RegWriteA( PWMA, 0xC0 ) ;			
		RegWriteA( STBB, 0x0F ) ;			
		break ;
	case STB3_ON:
		RegWriteA( STBB, 0x00 ) ;			
		RegWriteA( PWMA, 0x00 ) ;			
		RegWriteA( LNA,  0x00 ) ;			
		DrvSw( OFF ) ;						
		RegWriteA( PWMMONFC, 0x00 ) ;		
		RegWriteA( DAMONFC, 0x00 ) ;		
		SelectGySleep( ON ) ;				
		RegWriteA( CLKON, 0x00 ) ;			
		RegWriteA( I2CSEL, 0x01 ) ;			
		RegWriteA( OSCSTOP, 0x02 ) ;		
		break ;
	case STB3_OFF:
		RegWriteA( OSCSTOP, 0x00 ) ;		
		RegWriteA( I2CSEL, 0x00 ) ;			
#ifdef I2CE2PROM
		RegWriteA( CLKON, 0x33 ) ;			
#else
 #ifdef SPIE2PROM
		RegWriteA( CLKON, 0x17 ) ;			
 #else
		RegWriteA( CLKON, 0x13 ) ;			
 #endif
#endif
		SelectGySleep( OFF ) ;				
		RegWriteA( DAMONFC, 0x00 ) ;		
		RegWriteA( PWMMONFC, 0x80 ) ;		
		DrvSw( ON ) ;						
		RegWriteA( LNA,  0xC0 ) ;			
		RegWriteA( PWMA, 0xC0 ) ;			
		RegWriteA( STBB, 0x0F ) ;			
		break ;
		
	case STB4_ON:
		RegWriteA( STBB, 0x00 ) ;			
		RegWriteA( PWMA, 0x00 ) ;			
		RegWriteA( LNA,  0x00 ) ;			
		DrvSw( OFF ) ;						
		RegWriteA( PWMMONFC, 0x00 ) ;		
		RegWriteA( DAMONFC, 0x00 ) ;		
		RegWriteA( CLKON, 0x12 ) ;			
		break ;
	case STB4_OFF:
#ifdef I2CE2PROM
		RegWriteA( CLKON, 0x33 ) ;			
#else
 #ifdef SPIE2PROM
		RegWriteA( CLKON, 0x17 ) ;			
 #else
		RegWriteA( CLKON, 0x13 ) ;			
 #endif
#endif
		RegWriteA( DAMONFC, 0x00 ) ;		
		RegWriteA( PWMMONFC, 0x80 ) ;		
		DrvSw( ON ) ;						
		RegWriteA( LNA,  0xC0 ) ;			
		RegWriteA( PWMA, 0xC0 ) ;			
		RegWriteA( STBB, 0x0F ) ;			
		break ;
	}
}
#endif

void	SetZsp( unsigned char	UcZoomStepDat )
{
	unsigned long	UlGyrZmx, UlGyrZmy, UlGyrZrx, UlGyrZry ;

	
	
	if(UcZoomStepDat > (ZOOMTBL - 1))
		UcZoomStepDat = (ZOOMTBL -1) ;										

	if( UcZoomStepDat == 0 )				
	{
		UlGyrZmx	= ClGyxZom[ 0 ] ;		
		UlGyrZmy	= ClGyyZom[ 0 ] ;		
		
	}
	else
	{
		UlGyrZmx	= ClGyxZom[ UcZoomStepDat ] ;
		UlGyrZmy	= ClGyyZom[ UcZoomStepDat ] ;
		
		
	}
	
	
#ifdef	TESTPNTL
	RamWrite32A( gxgain, UlGyrZmx ) ;		
	RamWrite32A( gygain, UlGyrZmy ) ;		

	RamRead32A( gxgain, &UlGyrZrx ) ;		
	RamRead32A( gygain, &UlGyrZry ) ;		

	
	if( UlGyrZmx != UlGyrZrx ) {
		RamWrite32A( gxgain, UlGyrZmx ) ;		
	}

	if( UlGyrZmy != UlGyrZry ) {
		RamWrite32A( gygain, UlGyrZmy ) ;		
	}
#else
	RamWrite32A( gxlens, UlGyrZmx ) ;		
	RamWrite32A( gylens, UlGyrZmy ) ;		

	RamRead32A( gxlens, &UlGyrZrx ) ;		
	RamRead32A( gylens, &UlGyrZry ) ;		

	
	if( UlGyrZmx != UlGyrZrx ) {
		RamWrite32A( gxlens, UlGyrZmx ) ;		
	}

	if( UlGyrZmy != UlGyrZry ) {
		RamWrite32A( gylens, UlGyrZmy ) ;		
	}
#endif

}

 
void StbOnn( void )
{
	unsigned char	UcRegValx,UcRegValy;					
	unsigned char	UcRegIni ;
	unsigned char	UcRegVal1 ;
	unsigned short	UsRamVal1 , UsRamVal2 , UsRamVal3 ,UsRamVal4 , UsRamVal5 , UsRamVal6 ;
	
	RegReadA( LXEQEN, &UcRegValx ) ;				
	RegReadA( LYEQEN, &UcRegValy ) ;				
	if( (( UcRegValx & 0x80 ) != 0x80 ) && (( UcRegValy & 0x80 ) != 0x80 ))
	{
		RegReadA( SSSEN, &UcRegVal1 ) ;				
		RamReadA( HXSMTMP , &UsRamVal1 ) ;			
		RamReadA( HXTMP , &UsRamVal2 ) ;			
		RamReadA( HXIN , &UsRamVal3 ) ;				
		RamReadA( HYSMTMP , &UsRamVal4 ) ;			
		RamReadA( HYTMP , &UsRamVal5 ) ;			
		RamReadA( HYIN , &UsRamVal6 ) ;				
		
		RegWriteA( SSSEN,  0x88 ) ;				
		
		
		SrvCon( X_DIR, ON ) ;
		SrvCon( Y_DIR, ON ) ;
		
		UcRegIni = 0x11;
		while( (UcRegIni & 0x77) != 0x66 )
		{
			RegReadA( SSSEN,  &UcRegIni ) ;			
		}
		RegReadA( SSSEN, &UcRegVal1 ) ;				
		RamReadA( HXSMTMP , &UsRamVal1 ) ;			
		RamReadA( HXTMP , &UsRamVal2 ) ;			
		RamReadA( HXIN , &UsRamVal3 ) ;				
		RamReadA( HYSMTMP , &UsRamVal4 ) ;			
		RamReadA( HYTMP , &UsRamVal5 ) ;			
		RamReadA( HYIN , &UsRamVal6 ) ;				
		RegWriteA( SSSEN,  0x00 ) ;				
		
	}
	else
	{
		SrvCon( X_DIR, ON ) ;
		SrvCon( Y_DIR, ON ) ;
	}
}

void	OptCen( unsigned char UcOptmode , unsigned short UsOptXval , unsigned short UsOptYval )
{
	switch ( UcOptmode ) {
		case VAL_SET :
			RamWriteA( HXINOD	, UsOptXval ) ;								
			RamWriteA( HYINOD	, UsOptYval ) ;								
			break ;
		case VAL_FIX :
			UsCntXof = UsOptXval ;
			UsCntYof = UsOptYval ;
			RamWriteA( HXINOD	, UsCntXof ) ;								
			RamWriteA( HYINOD	, UsCntYof ) ;								
#ifdef USE_EXE2PROM
			E2pWrt( OPT_CENTER_X,	2, ( unsigned char * )&UsCntXof ) ;
			E2pWrt( OPT_CENTER_Y,	2, ( unsigned char * )&UsCntYof ) ;
#endif

			break ;
		case VAL_SPC :
			RamReadA( HXINOD   , &UsOptXval ) ;								
			RamReadA( HYINOD   , &UsOptYval ) ;								
			UsCntXof = UsOptXval ;
			UsCntYof = UsOptYval ;
#ifdef USE_EXE2PROM
			E2pWrt( OPT_CENTER_X,	2, ( unsigned char * )&UsCntXof ) ;
			E2pWrt( OPT_CENTER_Y,	2, ( unsigned char * )&UsCntYof ) ;
#endif

			break ;
	}

}


#ifdef		MODULE_CALIBRATION		
#define	RRATETABLE	8
#define	CRATETABLE	16
const signed char	ScRselRate[ RRATETABLE ]	= {
		-10,			
		 -6,			
		 -3,			
		  0,			
		  5,			
		  9,			
		 14,			
		 20				
	} ;
const signed char	ScCselRate[ CRATETABLE ]	= {
		-14,			
		-12,			
		-10,			
		 -8,			
		 -5,			
		 -3,			
		 -1,			
		  0,			
		  0,			
		  3,			
		  5,			
		  7,			
		  9,			
		 11,			
		 13,			
		 15				
	} ;
	

#define	TARGET_FREQ		48000.0F
#define	START_RSEL		0x03	
#define	START_CSEL		0x07	
#define	MEAS_MAX		32		
#define	UNDR_MEAS		0x00
#define	FIX_MEAS		0x01
#define	JST_FIX			0x03
#define	OVR_MEAS		0x80
#define	RSELFX			0x08
#define	RSEL1ST			0x01
#define	RSEL2ND			0x02
#define	CSELFX			0x80
#define	CSELPLS			0x10
#define	CSELMNS			0x20

unsigned short	OscAdj( void )
{
	unsigned char	UcMeasFlg ;									
	UnWrdVal		StClkVal ;									
	unsigned char	UcMeasCnt ;									
	unsigned char	UcOscrsel , UcOsccsel ;						
	unsigned char	UcPwmDivBk ;								
	unsigned char	UcClkJdg ;									
	float			FcalA,FcalB ;								
	signed char		ScTblRate_Val, ScTblRate_Now, ScTblRate_Tgt ;	
	float			FlRatePbk,FlRateMbk ;							
	unsigned char	UcOsccselP , UcOsccselM ;					
	unsigned short	UsResult ;
	
	UcMeasFlg = 0 ;						
	UcMeasCnt = 0;						
	UcClkJdg = UNDR_MEAS;				
	UcOscrsel = START_RSEL ;
	UcOsccsel = START_CSEL ;
	
	
	
	RegReadA( PWMDIV, &UcPwmDivBk ) ;	
	RegWriteA( PWMDIV,	0x00 ) ;		
	RegWriteA( OSCSET, ( UcOscrsel << 5 ) | (UcOsccsel << 1 ) | 0x01 ) ;	
	
	while( UcClkJdg == UNDR_MEAS )
	{
		UcMeasCnt++ ;						
		UcOscAdjFlg = MEASSTR ;				
		
		while( UcOscAdjFlg != MEASFIX )
		{
			;
		}
		
		UcOscAdjFlg = 0x00 ;				
		RegReadA( OSCCK_CNTR0, &StClkVal.StWrdVal.UcLowVal ) ;		
		RegReadA( OSCCK_CNTR1, &StClkVal.StWrdVal.UcHigVal ) ;		
		
		FcalA = (float)StClkVal.UsWrdVal ;
		FcalB = TARGET_FREQ / FcalA ;
		FcalB =  FcalB - 1.0F ;
		FcalB *= 100.0F ;
		
		if( FcalB == 0.0F )
		{
			UcClkJdg = JST_FIX ;					
			UcMeasFlg |= ( CSELFX | RSELFX ) ;		
			break ;
		}

		
		if( !(UcMeasFlg & RSELFX) )
		{
			if(UcMeasFlg & RSEL1ST)
			{
				UcMeasFlg |= ( RSELFX | RSEL2ND ) ;
			}
			else
			{
				UcMeasFlg |= RSEL1ST ;
			}
			ScTblRate_Now = ScRselRate[ UcOscrsel ] ;					
			ScTblRate_Tgt = ScTblRate_Now + (short)FcalB ;
			if( ScTblRate_Now > ScTblRate_Tgt )
			{
				while(1)
				{
					if( UcOscrsel == 0 )
					{
						break;
					}
					UcOscrsel -= 1 ;
					ScTblRate_Val = ScRselRate[ UcOscrsel ] ;	
					if( ScTblRate_Tgt >= ScTblRate_Val )
					{
						break;
					}
				}
			}
			else if( ScTblRate_Now < ScTblRate_Tgt )
			{
				while(1)
				{
					if(UcOscrsel == (RRATETABLE - 1))
					{
						break;
					}
					UcOscrsel += 1 ;
					ScTblRate_Val = ScRselRate[ UcOscrsel ] ;	
					if( ScTblRate_Tgt <= ScTblRate_Val )
					{
						break;
					}
				}
			}
			else
			{
				;
			}
		}
		else
		{		
		
			if( FcalB > 0 )			
			{
				UcMeasFlg |= CSELPLS ;
				FlRatePbk = FcalB ;
				UcOsccselP = UcOsccsel ;
				if( UcMeasFlg & CSELMNS)
				{
					UcMeasFlg |= CSELFX ;
					UcClkJdg = FIX_MEAS ;			
				}
				else if(UcOsccsel == (CRATETABLE - 1))
				{
					if(UcOscrsel < ( RRATETABLE - 1 ))
					{
						UcOscrsel += 1 ;
						UcOsccsel = START_CSEL ;
						UcMeasFlg = 0 ;			
					}
					else
					{
						UcClkJdg = OVR_MEAS ;			
					}
				}
				else
				{
					UcOsccsel += 1 ;
				}
			}
			else					
			{
				UcMeasFlg |= CSELMNS ;
				FlRateMbk = (-1)*FcalB ;
				UcOsccselM = UcOsccsel ;
				if( UcMeasFlg & CSELPLS)
				{
					UcMeasFlg |= CSELFX ;
					UcClkJdg = FIX_MEAS ;			
				}
				else if(UcOsccsel == 0x00)
				{
					if(UcOscrsel > 0)
					{
						UcOscrsel -= 1 ;
						UcOsccsel = START_CSEL ;
						UcMeasFlg = 0 ;			
					}
					else
					{
					UcClkJdg = OVR_MEAS ;			
					}
				}
				else
				{
					UcOsccsel -= 1 ;
				}
			}
			if(UcMeasCnt >= MEAS_MAX)
			{
				UcClkJdg = OVR_MEAS ;			
			}
		}	
		RegWriteA( OSCSET, ( UcOscrsel << 5 ) | (UcOsccsel << 1 ) | 0x01 ) ;	
	}
	
	UsResult = EXE_END ;
	
	if(UcClkJdg == FIX_MEAS)
	{
		if( FlRatePbk < FlRateMbk )
		{
			UcOsccsel = UcOsccselP ; 
		}
		else
		{
			UcOsccsel = UcOsccselM ; 
		}
	
		RegWriteA( OSCSET, ( UcOscrsel << 5 ) | (UcOsccsel << 1 ) | 0x01 ) ;	
	}
	StAdjPar.UcOscVal = ( ( UcOscrsel << 5 ) | (UcOsccsel << 1 ) | 0x01 );
	
	if(UcClkJdg == OVR_MEAS)
	{
		UsResult = EXE_OCADJ ;
		StAdjPar.UcOscVal = 0x00 ;
	}
	RegWriteA( PWMDIV,	UcPwmDivBk ) ;		

	return( UsResult );
}
#endif
	
#ifdef HALLADJ_HW
void SetSineWave( unsigned char UcJikuSel , unsigned char UcMeasMode )
{
	unsigned char	UcSWFC1[]	= { 0xBA , 0x4F } ,			
					UcSWFC2[]	= { 0x01 , 0x02 } ;			

	unsigned char	UcSwSel[2][2] = { { 0x80 , 0x40 } , 						
									  { 0x00 , 0x00 }							
									};

	UcMeasMode &= 0x01;
	UcJikuSel  &= 0x01;

	

	
	RamWriteA( WAVXO , 0x0000 );												
	RamWriteA( WAVYO , 0x0000 );												

	
	RamWriteA( wavxg , 0x7FFF );												
	RamWriteA( wavyg , 0x7FFF );												

	
	
	
	
	RegWriteA( SWFC1 , UcSWFC1[UcMeasMode] );									
	RegWriteA( SWFC2 , UcSWFC2[UcMeasMode] );									
	RegWriteA( SWFC3 , 0x00 );													
	RegWriteA( SWFC4 , 0x00 );													
	RegWriteA( SWFC5 , 0x00 );													

	
	RegWriteA( SWSEL , UcSwSel[UcMeasMode][UcJikuSel] );						

	
	if( !UcMeasMode )		
	{
		RegWriteA( SINXADD , 0x00 );											
		RegWriteA( SINYADD , 0x00 );											
	}
	else if( !UcJikuSel )	
	{
		RegWriteA( SINXADD , (unsigned char)LXDODAT );							
		RegWriteA( SINYADD , 0x00 );											
	}
	else					
	{
		RegWriteA( SINXADD , 0x00 );											
		RegWriteA( SINYADD , (unsigned char)LYDODAT );							
	}
}

void StartSineWave( void )
{
	
	
	
	
	
	
	
	
	RegWriteA( SWEN , 0x80 );													
}

void StopSineWave( void )
{
	
	RegWriteA( SWSEL   , 0x00 );												
	RegWriteA( SINXADD , 0x00 );												
	RegWriteA( SINYADD , 0x00 );												

	
	RegWriteA( SWEN  , 0x00 );													
}

void SetMeasFil( unsigned char UcJikuSel , unsigned char UcMeasMode , unsigned char UcFilSel )
{
	unsigned short	UsIn1Add[2][2] = { { LXC1	, LYC1	 } ,					
									   { ADHXI0 , ADHYI0 }						
									 } ,
					UsIn2Add[2][2] = { { LXC2	, LYC2	 } ,					
									   { 0x0000 , 0x0000 }						
									 } ;

	
	UcJikuSel  &= 0x01;
	UcMeasMode &= 0x01;
	if( UcFilSel > NOISE ) UcFilSel = THROUGH;
	
	MesFil( UcFilSel ) ;					

	RegWriteA( MS1INADD , (unsigned char)UsIn1Add[UcMeasMode][UcJikuSel] ); 	
	RegWriteA( MS1OUTADD, 0x00 );												


	RegWriteA( MS2INADD , (unsigned char)UsIn2Add[UcMeasMode][UcJikuSel] ); 	
	RegWriteA( MS2OUTADD, 0x00 );												

	
	
	
	
	RegWriteA( MSFDS , 0x00 );													
}

void StartMeasFil( void )
{
	
	RegWriteA( MSF1EN , 0x01 ); 												
	RegWriteA( MSF2EN , 0x01 ); 												
}

void StopMeasFil( void )
{
	
	RegWriteA( MSF1EN , 0x00 ); 												
	RegWriteA( MSF2EN , 0x00 ); 												
}

unsigned char	 LoopGainAdj( unsigned char UcJikuSel)
{
	
	
	
	
	
	

	unsigned short	UsSineAdd[] = { lxxg   , lyxg	} ;
	unsigned short	UsSineGanCVL[] = { 0x198A , 0x198A } ;
	unsigned short	UsSineGanPWM[] = { 0x32F5 , 0x32F5 } ;
	unsigned short	UsRltVal ;
	unsigned char	UcAdjSts	= FAILURE ;
	
	UcJikuSel &= 0x01;

	StbOnn() ;											
	
	
	WitTim( 200 ) ;
	
	
	LopPar( UcJikuSel ) ;
	
	
	SetSineWave( UcJikuSel , __MEASURE_LOOPGAIN );

	
	if( UcPwmMod == PWMMOD_CVL ) {
		RamWriteA( UsSineAdd[UcJikuSel] , UsSineGanCVL[UcJikuSel] ); 					
	}else{
		RamWriteA( UsSineAdd[UcJikuSel] , UsSineGanPWM[UcJikuSel] ); 					
	}
	
	
	RegWriteA( MSMPLNSH , 0x03 );												
	RegWriteA( MSMPLNSL , 0xFF );												

	
	RamWriteA( AJLPMAX	, 0x7FFF ); 											
	RamWriteA( AJLPMIN	, 0x2000 ); 											
	RegWriteA( AJWAIT	, 0x00 );												
	RegWriteA( AJTIMEOUT, 0xFF );												
	RegWriteA( AJNUM	, 0x00 );												

	
	SetMeasFil( UcJikuSel , __MEASURE_LOOPGAIN , LOOPGAIN );

	
	StartMeasFil();

	
	StartSineWave();

	
	RegWriteA( AJTA 	, (0x98 | UcJikuSel) ); 								

	
	do{
		RegReadA( FLGM , &UcAdjBsy );											
	}while(( UcAdjBsy & 0x40 ) != 0x00 );

	RegReadA( AJSTATUS , &UcAdjBsy );								
	if( UcAdjBsy )
	{
		if( UcJikuSel == X_DIR )
		{
			RamReadA( lxgain, &UsRltVal ) ;							
			StAdjPar.StLopGan.UsLxgVal	= UsRltVal ;
			StAdjPar.StLopGan.UsLxgSts	= 0x0000 ;
		} else {
			RamReadA( lygain, &UsRltVal ) ;							
			StAdjPar.StLopGan.UsLygVal	= UsRltVal ;
			StAdjPar.StLopGan.UsLygSts	= 0x0000 ;
		}
		RegWriteA( AJSTATUS , 0x00 );								

	}
	else
	{
		if( UcJikuSel == X_DIR )
		{
			RamReadA( lxgain, &UsRltVal ) ;							
			StAdjPar.StLopGan.UsLxgVal	= UsRltVal ;
			StAdjPar.StLopGan.UsLxgSts	= 0xFFFF ;
		} else {
			RamReadA( lygain, &UsRltVal ) ;							
			StAdjPar.StLopGan.UsLygVal	= UsRltVal ;
			StAdjPar.StLopGan.UsLygSts	= 0xFFFF ;
		}
		UcAdjSts	= SUCCESS ;												
	}


	
	RamWriteA( UsSineAdd[UcJikuSel] , 0x0000 ); 								
	
	
	StopSineWave();

	
	StopMeasFil();

	return( UcAdjSts ) ;
}

unsigned char  BiasOffsetAdj( unsigned char UcJikuSel , unsigned char UcMeasCnt )
{
	
	
	
	
	
	
	
	
	unsigned char 	UcHadjRst ;
	unsigned short	UsEqSwAddOff[]	 = { LXEQEN , LYEQEN } ;
	unsigned short	UsEqSwAddOn[]	 = { LYEQEN , LXEQEN } ;
									
	unsigned short	UsTgtVal[2][5]	  = {{ 0x4026 , 0x0400 , 0xFC00 , 0x6A65 , 0x6265 },	
										 { 0x2013 , 0x0200 , 0xFE00 , 0x67B5 , 0x6515 }} ;	
	

	if(UcMeasCnt > 1)		UcMeasCnt = 1 ;
	
	UcJikuSel &= 0x01;

	
	SetSineWave( UcJikuSel , __MEASURE_BIASOFFSET );

	
	RegWriteA( UsEqSwAddOff[UcJikuSel] ,	0x45 );
	RegWriteA( UsEqSwAddOn[UcJikuSel] ,		0xC5 );

	
	RegWriteA( MSMPLNSH , 0x00 );									
	RegWriteA( MSMPLNSL , 0x03 );									

	
	RamWriteA( AJHOFFD	, UsTgtVal[UcMeasCnt][0] );					
	RamWriteA( AJCTMAX	, UsTgtVal[UcMeasCnt][1] );					
	RamWriteA( AJCTMIN	, UsTgtVal[UcMeasCnt][2] );					
	RamWriteA( AJHGAIND , UsTgtVal[UcMeasCnt][0] );					
	RamWriteA( AJAPMAX	, UsTgtVal[UcMeasCnt][3] );					
	RamWriteA( AJAPMIN	, UsTgtVal[UcMeasCnt][4] );					
	RegWriteA( AJWAIT	, 0x00 );									
	RegWriteA( AJTIMEOUT, 0xE5 );									

	
	SetMeasFil( UcJikuSel , __MEASURE_BIASOFFSET , HALL_ADJ );

	
	StartMeasFil();

	
	StartSineWave();

	
	RegWriteA( AJTA 	, (0x0E | UcJikuSel) );						

	
	do{
		RegReadA( FLGM , &UcAdjBsy );								
	}while(( UcAdjBsy & 0x40 ) != 0x00 );
	
	RegReadA( AJSTATUS , &UcAdjBsy );								
	if( UcAdjBsy )
	{
		if( UcJikuSel == X_DIR )
		{
			UcHadjRst = EXE_HXADJ ;
		}
		else
		{
			UcHadjRst = EXE_HYADJ ;
		}
		RegWriteA( AJSTATUS , 0x00 );								

	}
	else
	{
		UcHadjRst = EXE_END ;
	}

	
	StopSineWave();

	
	StopMeasFil();

	
	RegWriteA( UsEqSwAddOff[UcJikuSel] , 0xC5 );
	
	if( UcJikuSel == X_DIR )
	{
		RamReadA( MSSAPAV, &StAdjPar.StHalAdj.UsHlxMxa	) ;	 	
		RamReadA( MSSCTAV, &StAdjPar.StHalAdj.UsHlxCna	) ;	 	
	}
	else
	{
		RamReadA( MSSAPAV, &StAdjPar.StHalAdj.UsHlyMxa	) ;	 	
		RamReadA( MSSCTAV, &StAdjPar.StHalAdj.UsHlyCna	) ;	 	
	}

	return( UcHadjRst ) ;
}

#endif


void	GyrGan( unsigned char UcGygmode , unsigned long UlGygXval , unsigned long UlGygYval )
{
	switch ( UcGygmode ) {
		case VAL_SET :
			RamWrite32A( gxzoom, UlGygXval ) ;		
			RamWrite32A( gyzoom, UlGygYval ) ;		
			break ;
		case VAL_FIX :
			RamWrite32A( gxzoom, UlGygXval ) ;		
			RamWrite32A( gyzoom, UlGygYval ) ;		
#ifdef USE_EXE2PROM
			E2pWrt( GYRO_GAIN_X,	4, ( unsigned char * )&UlGygXval ) ;
			E2pWrt( GYRO_GAIN_Y,	4, ( unsigned char * )&UlGygYval ) ;
#endif
			break ;
		case VAL_SPC :
			RamRead32A( gxzoom, &UlGygXval ) ;		
			RamRead32A( gyzoom, &UlGygYval ) ;		
#ifdef USE_EXE2PROM
			E2pWrt( GYRO_GAIN_X,	4, ( unsigned char * )&UlGygXval ) ;
			E2pWrt( GYRO_GAIN_Y,	4, ( unsigned char * )&UlGygYval ) ;
#endif
			break ;
	}

}

void	SetPanTiltMode( unsigned char UcPnTmod )
{
	switch ( UcPnTmod ) {
		case OFF :
			RegWriteA( GPANON, 0x00 ) ;			
			break ;
		case ON :
			RegWriteA( GPANON, 0x11 ) ;			
			break ;
	}

}

#ifdef GAIN_CONT
unsigned char	TriSts( void )
{
	unsigned char UcRsltSts = 0;
	unsigned char UcVal ;

	RegReadA( GADJGANGXMOD, &UcVal ) ;	
	if( UcVal & 0x07 ){						
		RegReadA( GLEVJUGE, &UcVal ) ;	
		UcRsltSts = UcVal & 0x11 ;		
		UcRsltSts |= 0x80 ;				
	}
	return( UcRsltSts ) ;
}
#endif

unsigned char	DrvPwmSw( unsigned char UcSelPwmMod )
{

	switch ( UcSelPwmMod ) {
		case Mlnp :
			RegWriteA( DRVFC	, 0xE3 ) ;						
			RegWriteA( LXEQFC2 	, 0x01 ) ;						
			RegWriteA( LYEQFC2	, 0x01 ) ;						
			UcPwmMod = PWMMOD_CVL ;
			break ;
		
		case Mpwm :
			RegWriteA( LXEQFC2	, 0x00 ) ;						
			RegWriteA( LYEQFC2	, 0x00 ) ;						
 #ifdef	LOWCURRENT
			RegWriteA( DRVFC	, 0x03 ) ;						
 #else
			RegWriteA( DRVFC	, 0xC3 ) ;						
 #endif
			UcPwmMod = PWMMOD_PWM ;
 			break ;
	}
	
	return( UcSelPwmMod << 4 ) ;
}

#ifdef		MODULE_CALIBRATION		
 #ifdef	NEUTRAL_CENTER
unsigned char	TneHvc( void )
{
	unsigned char	UcRsltSts;
	unsigned short	UsMesRlt1 ;
	unsigned short	UsMesRlt2 ;
	
	SrvCon( X_DIR, OFF ) ;				
	SrvCon( Y_DIR, OFF ) ;				
	
	
	
	
	WitTim( 500 ) ;
	
	
	
	MesFil( THROUGH ) ;					
	
	RegWriteA( MSF1EN, 0x01 ) ;			
	RegWriteA( MSF2EN, 0x01 ) ;			
	
	RegWriteA( MSMPLNSH, 0x00 ) ;		
	RegWriteA( MSMPLNSL, 0x3F ) ;		
	
	RegWriteA( MS1INADD, 0x01 ) ;		
	RegWriteA( MS2INADD, 0x04 ) ;		
	
	
	BsyWit( MSMA, 0x01 ) ;				
	
	
	RamReadA( MSAV1, &UsMesRlt1 ) ;	
	RamReadA( MSAV2, &UsMesRlt2 ) ;	
	
	StAdjPar.StHalAdj.UsHlxCna = UsMesRlt1;			
	StAdjPar.StHalAdj.UsHlxCen = UsMesRlt1;			
	
	StAdjPar.StHalAdj.UsHlyCna = UsMesRlt2;			
	StAdjPar.StHalAdj.UsHlyCen = UsMesRlt2;			
	
	RegWriteA( MSF1EN, 0x00 ) ;			
	RegWriteA( MSF2EN, 0x00 ) ;			
	
	UcRsltSts = EXE_END ;				
	
	SrvCon( X_DIR, ON ) ;				
	SrvCon( Y_DIR, ON ) ;				
	
	return( UcRsltSts );
}
 #endif	
#endif
	
void	SetGcf( unsigned char	UcSetNum )
{
	unsigned long	UlGyrCof ;

	
	
	if(UcSetNum > (COEFTBL - 1))
		UcSetNum = (COEFTBL -1) ;			

	UlGyrCof	= ClDiCof[ UcSetNum ] ;
		
	
	RamWrite32A( gxh1c, UlGyrCof ) ;		
	RamWrite32A( gyh1c, UlGyrCof ) ;		

#ifdef H1COEF_CHANGER
		SetH1cMod( UcSetNum ) ;							
#endif

}

#ifdef H1COEF_CHANGER
void	SetH1cMod( unsigned char	UcSetNum )
{
	unsigned long	UlGyrCof ;

	
	switch( UcSetNum ){
	case ( ACTMODE ):				
		
		
		UlGyrCof	= ClDiCof[ 0 ] ;
			
		UcH1LvlMod = 0 ;
		
		
		RamWrite32A( gxl2a_2, MINLMT ) ;		
		RamWrite32A( gxl2b_2, MAXLMT ) ;		

		RamWrite32A( gyl2a_2, MINLMT ) ;		
		RamWrite32A( gyl2b_2, MAXLMT ) ;		

		RamWrite32A( gxh1c_2, UlGyrCof ) ;		
		RamWrite32A( gxl2c_2, CHGCOEF ) ;		

		RamWrite32A( gyh1c_2, UlGyrCof ) ;		
		RamWrite32A( gyl2c_2, CHGCOEF ) ;		
		
		RegWriteA( WG_LSEL,	0x51 );				
		RegWriteA( WG_HCHR,	0x04 );				
		break ;
		
	case( S2MODE ):				
		RegWriteA( WG_LSEL,	0x00 );				
		RegWriteA( WG_HCHR,	0x00 );				

		RamWrite32A( gxh1c_2, S2COEF ) ;		
		RamWrite32A( gyh1c_2, S2COEF ) ;		
		break ;
		
	default :
		
		if(UcSetNum > (COEFTBL - 1))
			UcSetNum = (COEFTBL -1) ;			

		UlGyrCof	= ClDiCof[ UcSetNum ] ;

		UcH1LvlMod = UcSetNum ;
			
		RamWrite32A( gxh1c_2, UlGyrCof ) ;		
		RamWrite32A( gyh1c_2, UlGyrCof ) ;		
		
		RegWriteA( WG_LSEL,	0x51 );				
		RegWriteA( WG_HCHR,	0x04 );				
		break ;
	}
}
#endif

unsigned short	RdFwVr( void )
{
	return( FW_VER ) ;
}
