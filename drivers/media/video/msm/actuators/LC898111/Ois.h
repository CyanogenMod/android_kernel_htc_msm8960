#define	FW_VER			0x0005
#define StAdjPar StAdjPar_lc898111
#define UsStpSiz UsStpSiz_lc898111

#define RegReadA RegReadA_lc898111
#define RegWriteA RegWriteA_lc898111
#define RamReadA RamReadA_lc898111
#define RamWriteA RamWriteA_lc898111
#define RamRead32A RamRead32A_lc898111
#define RamWrite32A RamWrite32A_lc898111
#define WitTim WitTim_lc898111

#ifdef	OISINI
	#define	OISINI__
#else
	#define	OISINI__		extern
#endif







#ifdef	OISCMD
	#define	OISCMD__
#else
	#define	OISCMD__		extern
#endif




#ifdef	USE_EXE2PROM
 
 #define		SPIE2PROM			
#endif

#define		CIRC_MOVE			
#define		STANDBY_MODE		

#define		GAIN_CONT			

	
#define	LOWCURRENT			


#define		TESTPNTL				
#define		NEUTRAL_CENTER
#define		H1COEF_CHANGER			
#define		OIS05DEG				



#define		EXE_END		0x02		
#define		EXE_HXADJ	0x06		
#define		EXE_HYADJ	0x0A		
#define		EXE_LXADJ	0x12		
#define		EXE_LYADJ	0x22		
#define		EXE_GXADJ	0x42		
#define		EXE_GYADJ	0x82		
#define		EXE_OCADJ	0x402		
#define		EXE_ERR		0x99		

#define	SUCCESS			0x00		
#define	FAILURE			0x01		

#ifndef ON
 #define	ON				0x01		
 #define	OFF				0x00		
#endif

#define	X_DIR			0x00		
#define	Y_DIR			0x01		
#define	X2_DIR			0x10		
#define	Y2_DIR			0x11		

#define	NOP_TIME		0.00004166F

#ifdef STANDBY_MODE
 
 #define		STB1_ON		0x00		
 #define		STB1_OFF	0x01		
 #define		STB2_ON		0x02		
 #define		STB2_OFF	0x03		
 #define		STB3_ON		0x04		
 #define		STB3_OFF	0x05		
 #define		STB4_ON		0x06		
 #define		STB4_OFF	0x07		
 #define		STB5_LOP	0x08		
OISINI__	unsigned char	UcStbySt ;	
 #define		STBYST_OFF	0x00
 #define		STBYST_ON	0x01
#endif


 #define		DAHLXO_INI		0xE5C0
 #define		DAHLXB_INI		0x0000
 #define		DAHLYO_INI		0xF118
 #define		DAHLYB_INI		0x1401
 #define		LXGAIN_INI		0x4000
 #define		LYGAIN_INI		0x4000
 #define		ADHXOFF_INI		0x0000
 #define		ADHYOFF_INI		0x0000
 #define		BAIS_CUR		0x11
 #define		AMP_GAIN		0x55

 #define		OSC_INI			0xAD		

 #define		DGYRO_OFST_XH	0x00
 #define		DGYRO_OFST_XL	0x00
 #define		DGYRO_OFST_YH	0x00
 #define		DGYRO_OFST_YL	0x00

 #define		OPTCEN_X		0x0000
 #define		OPTCEN_Y		0x0000

 #define		GXGAIN_INI		0xBF307D61
 #define		GYGAIN_INI		0x3F4D5602

 #define		GYROX_INI		0x45
 #define		GYROY_INI		0x43
 
 #define		PZGXP_INI		0x7FFF
 #define		PZGYP_INI		0x8001

#define		LXGAIN_LOP		0x3000
#define		LYGAIN_LOP		0x3000

#ifdef OIS05DEG
 #define		GYRO_LMT4L		0x3F99999A		
 #define		GYRO_LMT4H		0x3F99999A		

 #define		GYRA12_HGH		0x3F8CCCCD		
 #define		GYRA12_MID		0x3F000000		
 #define		GYRA34_HGH		0x3F8CCCCD		
#endif

#ifdef OIS06DEG
 #define		GYRO_LMT4L		0x3FB33333		
 #define		GYRO_LMT4H		0x3FB33333		

 #define		GYRA12_HGH		0x3FA66666		
 #define		GYRA12_MID		0x3F000000		
 #define		GYRA34_HGH		0x3FA66666		
#endif

#ifdef OIS07DEG
 #define		GYRO_LMT4L		0x3FD1EB85		
 #define		GYRO_LMT4H		0x3FD1EB85		

 #define		GYRA12_HGH		0x3FC51EB8		
 #define		GYRA12_MID		0x3F000000		
 #define		GYRA34_HGH		0x3FC51EB8		
#endif

 #define		GYRA34_MID		0x3DCCCCCD		
 #define		GYRB12_HGH		0x3DF5C28F		
 #define		GYRB12_MID		0x3CA3D70A		
 #define		GYRB34_HGH		0x3CA3D70A		
 #define		GYRB34_MID		0x3A03126F		

#ifdef	LMT1MODE
 #define		GYRO_LMT1H		0x3DCCCCCD		
#else
 #define		GYRO_LMT1H		0x3F800000		
#endif 

#ifdef I2CE2PROM									
 #define	E2POFST				0x0000				
#endif
#ifdef SPIE2PROM									
 #define	E2POFST				0x1000				
#endif
#define		CENTER_HALL_AD_X	(0x0000 + E2POFST)		
#define		MAX_HALL_BEFORE_X	(0x0002 + E2POFST)		
#define		MAX_HALL_AFTER_X	(0x0004 + E2POFST)		
#define		MIN_HALL_BEFORE_X	(0x0006 + E2POFST)		
#define		MIN_HALL_AFTER_X	(0x0008 + E2POFST)		
#define		HALL_BIAS_X			(0x000A + E2POFST)		
#define		HALL_OFFSET_X		(0x000C + E2POFST)		
#define		HALL_AD_OFFSET_X	(0x000E + E2POFST)		
#define		CENTER_HALL_AD_Y	(0x0010 + E2POFST)		
#define		MAX_HALL_BEFORE_Y	(0x0012 + E2POFST)		
#define		MAX_HALL_AFTER_Y	(0x0014 + E2POFST)		
#define		MIN_HALL_BEFORE_Y	(0x0016 + E2POFST)		
#define		MIN_HALL_AFTER_Y	(0x0018 + E2POFST)		
#define		HALL_BIAS_Y			(0x001A + E2POFST)		
#define		HALL_OFFSET_Y		(0x001C + E2POFST)		
#define		HALL_AD_OFFSET_Y	(0x001E + E2POFST)		
#define		LOOP_GAIN_STATUS_X	(0x0020 + E2POFST)
#define		LOOP_GAIN_STATUS_Y	(0x0022 + E2POFST)
#define		OPT_CENTER_X		(0x0024 + E2POFST)		
#define		OPT_CENTER_Y		(0x0026 + E2POFST)		
#define		LOOP_GAIN_X			(0x0028 + E2POFST)
#define		LOOP_GAIN_Y			(0x002A + E2POFST)
#define		GYRO_AD_OFFSET_X	(0x002C + E2POFST)
#define		GYRO_AD_OFFSET_Y	(0x002E + E2POFST)
#define		ADJ_COMP_FLAG		(0x0030 + E2POFST)
#define		GYRO_GAIN_X			(0x0032 + E2POFST)
#define		GYRO_GAIN_Y			(0x0036 + E2POFST)
#define		OSC_CLK_VAL			(0x003A + E2POFST)		





 #define	VAL_SET				0x00		
 #define	VAL_FIX				0x01		
 #define	VAL_SPC				0x02		


struct STHALREG {
	unsigned short	UsRegAdd ;
	unsigned char	UcRegDat ;
} ;													

struct STHALFIL {
	unsigned short	UsRamAdd ;
	unsigned short	UsRamDat ;
} ;													

struct STGYRFIL {
	unsigned short	UsRamAdd ;
	unsigned long	UlRamDat ;
} ;													

struct STCMDTBL
{
	unsigned short Cmd ;
	unsigned int UiCmdStf ;
	void ( *UcCmdPtr )( void ) ;
} ;


union	WRDVAL{
	unsigned short	UsWrdVal ;
	unsigned char	UcWrkVal[ 2 ] ;
	struct {
		unsigned char	UcLowVal ;
		unsigned char	UcHigVal ;
	} StWrdVal ;
} ;

typedef union WRDVAL	UnWrdVal ;

union	DWDVAL {
	unsigned long	UlDwdVal ;
	unsigned short	UsDwdVal[ 2 ] ;
	struct {
		unsigned short	UsLowVal ;
		unsigned short	UsHigVal ;
	} StDwdVal ;
	struct {
		unsigned char	UcRamVa0 ;
		unsigned char	UcRamVa1 ;
		unsigned char	UcRamVa2 ;
		unsigned char	UcRamVa3 ;
	} StCdwVal ;
} ;

typedef union DWDVAL	UnDwdVal;

union	FLTVAL {
	float			SfFltVal ;
	unsigned long	UlLngVal ;
	unsigned short	UsDwdVal[ 2 ] ;
	struct {
		unsigned short	UsLowVal ;
		unsigned short	UsHigVal ;
	} StFltVal ;
} ;

typedef union FLTVAL	UnFltVal ;


typedef struct STADJPAR {
	struct {
		unsigned char	UcAdjPhs ;				

		unsigned short	UsHlxCna ;				
		unsigned short	UsHlxMax ;				
		unsigned short	UsHlxMxa ;				
		unsigned short	UsHlxMin ;				
		unsigned short	UsHlxMna ;				
		unsigned short	UsHlxGan ;				
		unsigned short	UsHlxOff ;				
		unsigned short	UsAdxOff ;				
		unsigned short	UsHlxCen ;				

		unsigned short	UsHlyCna ;				
		unsigned short	UsHlyMax ;				
		unsigned short	UsHlyMxa ;				
		unsigned short	UsHlyMin ;				
		unsigned short	UsHlyMna ;				
		unsigned short	UsHlyGan ;				
		unsigned short	UsHlyOff ;				
		unsigned short	UsAdyOff ;				
		unsigned short	UsHlyCen ;				
	} StHalAdj ;

	struct {
		unsigned short	UsLxgVal ;				
		unsigned short	UsLygVal ;				
		unsigned short	UsLxgSts ;				
		unsigned short	UsLygSts ;				
	} StLopGan ;

	struct {
		unsigned short	UsGxoVal ;				
		unsigned short	UsGyoVal ;				
		unsigned short	UsGxoSts ;				
		unsigned short	UsGyoSts ;				
	} StGvcOff ;
	
	unsigned char		UcOscVal ;				

} stAdjPar ;

OISCMD__	stAdjPar	StAdjPar ;				

typedef struct	STLBGCON {
	unsigned short	UsLgxVal ;					
	unsigned short	UsLgyVal ;					
	unsigned char	UcLcnFlg ;					
} stLbgCon ;

OISCMD__	stLbgCon	StLbgCon ;				
OISCMD__	unsigned char	UcOscAdjFlg ;		
  #define	MEASSTR		0x01
  #define	MEASCNT		0x08
  #define	MEASFIX		0x80
#ifdef H1COEF_CHANGER
 OISCMD__	unsigned char	UcH1LvlMod ;		
#endif

OISINI__	unsigned short	UsCntXof ;				
OISINI__	unsigned short	UsCntYof ;				


OISINI__	unsigned char	UcPwmMod ;				
#define		PWMMOD_CVL	0x00		
#define		PWMMOD_PWM	0x01		

#define		INIT_PWMMODE	PWMMOD_CVL		



OISINI__ void	IniSet( void ) ;													
			#define		FS_SEL		3		

OISINI__ void	ClrGyr( unsigned char, unsigned char ); 							   
	#define CLR_GYR_PRM_RAM 	0x01
	#define CLR_GYR_DLY_RAM 	0x02
	#define CLR_GYR_ALL_RAM 	0x03
OISINI__ void	BsyWit( unsigned short, unsigned char ) ;				
OISINI__ void	WitTim( unsigned short ) ;											
OISINI__ void	MemClr( unsigned char *, unsigned short ) ;							
OISINI__ void	GyOutSignal( void ) ;									
OISINI__ void	Bsy2Wit( void ) ;										
#ifdef STANDBY_MODE
OISINI__ void	AccWit( unsigned char ) ;								
OISINI__ void	SelectGySleep( unsigned char ) ;						
#endif
#ifdef	GAIN_CONT
OISINI__ void	AutoGainControlSw( unsigned char ) ;							
#endif
OISINI__ void	DrvSw( unsigned char UcDrvSw ) ;						

OISCMD__ void			SrvCon( unsigned char, unsigned char ) ;					


#ifdef		MODULE_CALIBRATION		
 OISCMD__ unsigned short	TneRun( void ) ;											
#endif

OISCMD__ unsigned char	RtnCen( unsigned char ) ;									
OISCMD__ void			OisEna( void ) ;											
OISCMD__ void			TimPro( void ) ;											
OISCMD__ void			S2cPro( unsigned char ) ;									
OISCMD__ void			SetSinWavePara( unsigned char , unsigned char ) ;			
	#define		SINEWAVE	0
	#define		XHALWAVE	1
	#define		YHALWAVE	2
	#define		CIRCWAVE	255
#ifdef		MODULE_CALIBRATION		
 OISCMD__ unsigned char	TneGvc( void ) ;											
 #ifdef	NEUTRAL_CENTER											
  OISCMD__ unsigned char	TneHvc( void ) ;											
 #endif	
#endif

#ifdef USE_EXE2PROM
OISCMD__ void			E2pRed( unsigned short, unsigned char, unsigned char * ) ;	
OISCMD__ void			E2pWrt( unsigned short, unsigned char, unsigned char * ) ;	
#endif
OISCMD__ void			SetZsp( unsigned char ) ;									
OISCMD__ void			OptCen( unsigned char, unsigned short, unsigned short ) ;	

#ifdef		MODULE_CALIBRATION		
 OISCMD__ unsigned char	LopGan( unsigned char ) ;									
#endif

#ifdef STANDBY_MODE
 OISCMD__ void			SetStandby( unsigned char ) ;								
#endif

#ifdef		MODULE_CALIBRATION		
 OISCMD__ unsigned short	OscAdj( void ) ;											

 #ifdef	HALLADJ_HW
  OISCMD__ unsigned char	LoopGainAdj(   unsigned char );
  OISCMD__ unsigned char	BiasOffsetAdj( unsigned char , unsigned char );
 #endif
#endif

OISCMD__ void			GyrGan( unsigned char , unsigned long , unsigned long ) ;	
OISCMD__ void			SetPanTiltMode( unsigned char ) ;							
#ifdef		MODULE_CALIBRATION		
 #ifndef	HALLADJ_HW
 OISCMD__ unsigned long	TnePtp( unsigned char, unsigned char ) ;					
 OISCMD__ unsigned char	TneCen( unsigned char, UnDwdVal ) ;							
 #define		PTP_BEFORE		0
 #define		PTP_AFTER		1
 #endif
#endif
#ifdef GAIN_CONT
 unsigned char	TriSts( void ) ;													
#endif
OISCMD__  unsigned char	DrvPwmSw( unsigned char ) ;											
	#define		Mlnp		0					
	#define		Mpwm		1					
OISCMD__ void			SetGcf( unsigned char ) ;									
#ifdef H1COEF_CHANGER
 OISCMD__ void			SetH1cMod( unsigned char ) ;								
 #define		S2MODE		0x40
 #define		ACTMODE		0x80
 #define		MAXLMT		0x3FD1Eb85				
 #ifdef OIS05DEG
  #define		MINLMT		0x3F8CCCCD				
 #endif
 #ifdef OIS06DEG
  #define		MINLMT		0x3FA66666				
 #endif
 #ifdef OIS07DEG
  #define		MINLMT		0x3FB33333				
 #endif
 #define		CHGCOEF		0xB92FA5DE
 #define		S2COEF		0x3F800000				
#endif
unsigned short	RdFwVr( void ) ;													
