#define		OISINI

#include	"Ois.h"
#include	"OisFil.h"
#include	"OisDef.h"

#include	"HtcActOisBinder.h"


void	IniClk( void ) ;		
void	IniIop( void ) ;		
void	IniMon( void ) ;		
void	IniSrv( void ) ;		
void	IniGyr( void ) ;		
void	IniHfl( void ) ;		
void	IniGfl( void ) ;		
void	IniAdj( void ) ;		
void	IniCmd( void ) ;		
void	IniDgy( void ) ;		



void	IniSet( void )
{
	
	IniClk() ;
	
	IniIop() ;
	
	IniDgy() ;
	
	IniMon() ;
	
	IniSrv() ;
	
	IniGyr() ;
	
	IniHfl() ;
	
	IniGfl() ;
	
	IniAdj() ;
	
	IniCmd() ;
}



void	IniClk( void )
{
	RegWriteA( OSCSTOP,  0x00 ) ;		
	RegWriteA( OSCSET,   0x65 ) ;		
	UcOscAdjFlg	= 0 ;					
	RegWriteA( OSCCNTEN, 0x00 ) ;		
	
	
	RegWriteA( CLKTST,	0x00 ) ;		

#ifdef I2CE2PROM
	RegWriteA( CLKON,	0x33 ) ;		
#else
 #ifdef	SPIE2PROM
	RegWriteA( CLKON,	0x17 ) ;		
 #else
	RegWriteA( CLKON,	0x13 ) ;		
 #endif
#endif
	
	RegWriteA( EEPDIV,	0x02 ) ;		

	RegWriteA( SRVDIV,  0x02 ) ;		
										
	RegWriteA( PWMDIV,	0x00 ) ;		
	
	RegWriteA( TSTDIV,	0x04 ) ;		
	RegWriteA( GIFDIV,	0x03 ) ;		
	RegWriteA( CALDIV,	0x06 ) ;		
}



void	IniIop( void )
{
	
	RegWriteA( P0LEV0, 0x00 ) ;		
	RegWriteA( P0LEV1, 0x00 ) ;		
	RegWriteA( P0DIR0, 0x77 ) ;		
	RegWriteA( P0DIR1, 0x01 ) ;		

	
	RegWriteA( P0PON0, 0x8D ) ;		
	RegWriteA( P0PON1, 0x0D ) ;		
	RegWriteA( P0PUD0, 0x85 ) ;		
	RegWriteA( P0PUD1, 0x09 ) ;		

	
	RegWriteA( IOP0SEL, 0x00 ); 	
	RegWriteA( IOP1SEL, 0x00 ); 	
	RegWriteA( IOP2SEL, 0x00 ); 	
									
	RegWriteA( IOP3SEL, 0x00 ); 	
									
#ifdef I2CE2PROM
	RegWriteA( IOP4SEL, 0x21 ); 	
									
#else
 #ifdef SPIE2PROM
	RegWriteA( IOP4SEL, 0x11 ); 	
									
 #else
	RegWriteA( IOP4SEL, 0x00 ); 	
									
 #endif
#endif
	RegWriteA( IOP5SEL, 0x01 ); 	
									
#ifdef SPIE2PROM
	RegWriteA( IOP6SEL, 0x00 ); 	
									
#else
	RegWriteA( IOP6SEL, 0x11 ); 	
									
#endif
	RegWriteA( IOP7SEL, 0x00 ); 	
									
	RegWriteA( IOP8SEL, 0x00 ); 	
									

	
	RegWriteA( BSYSEL, 0x00 );		
									
									
									
									
									
									
									
									

	
	RegWriteA( SPIMD3, 0x00 );		
	RegWriteA( I2CSEL, 0x00 );		
	RegWriteA( SRMODE, 0x02 );		
									
#ifdef I2CE2PROM
	RegWriteA( EEPMODE, 0x01 );		
#else
	RegWriteA( EEPMODE, 0x00 );		
#endif
}



void	IniDgy( void )
{
	unsigned char	UcGrini ;
	
	
	
	
	
	
	RegWriteA( SPIM 	, 0x01 );							
															

	
	RegWriteA( GRSEL	, 0x01 );							

	
	RegWriteA( GRINI	, 0x80 );							
	RegWriteA( GRINT	, 0x00 );							


	RegReadA( GRINI	, &UcGrini );							
	RegWriteA( GRINI	, ( UcGrini | SLOWMODE) );			
	
	RegWriteA( GRADR0	, 0x6A ) ;					
	RegWriteA( GSETDT	, 0x10 ) ;					
	RegWriteA( GRACC	, 0x10 );					
	AccWit( 0x10 ) ;								

	RegWriteA( GRADR0,	0x1B ) ;					
	RegWriteA( GSETDT,	( FS_SEL << 3) ) ;			
	RegWriteA( GRACC,	0x10 ) ;					
	AccWit( 0x10 ) ;								

	RegReadA( GRINI	, &UcGrini );					
	RegWriteA( GRINI, ( UcGrini & ~SLOWMODE) );		
	
	UcStbySt = STBYST_OFF ;		
	
	GyOutSignal() ;

}


void	IniMon( void )
{
	RegWriteA( PWMMONFC, 0x80 ) ;				
	
	RegWriteA( MONSELA, 0x5C ) ;				
	RegWriteA( MONSELB, 0x5D ) ;				
#ifdef I2CE2PROM
	RegWriteA( MONSELC, 0x2D ) ;				
#else
	RegWriteA( MONSELC, 0x62 ) ;				
#endif
	RegWriteA( MONSELD, 0x63 ) ;				
}



void	IniSrv( void )
{
	UcPwmMod = INIT_PWMMODE ;					
	
	RegWriteA( VGA_SET, 0x30 ) ;				
	RegWriteA( LSVFC1 , 0x00 ) ;				
	if( UcPwmMod == PWMMOD_CVL ) {
		RegWriteA( LXEQFC2 , 0x01 ) ;				
		RegWriteA( LYEQFC2 , 0x01 ) ;				
	}else{
		RegWriteA( LXEQFC2 , 0x00 ) ;				
		RegWriteA( LYEQFC2 , 0x00 ) ;				
	}
	
	
	RegWriteA( LXEQEN , 0x45 );					
	RegWriteA( LXEQFC , 0x00 );					
	
	RamWriteA( ADHXOFF,   0x0000 ) ;			
	RamWriteA( ADSAD4OFF, 0x0000 ) ;			
	RamWriteA( HXINOD,    0x0000 ) ;			
	RamWriteA( HXDCIN,    0x0000 ) ;			
	RamWriteA( HXSEPT1,    0x0000 ) ;			
	RamWriteA( HXSEPT2,    0x0000 ) ;			
	RamWriteA( HXSEPT3,    0x0000 ) ;			
	RamWriteA( LXDOBZ,     0x0000 ) ;			
	RamWriteA( LXFZF,      0x0000 ) ;			
	RamWriteA( LXFZB,      0x0000 ) ;			
	RamWriteA( LXDX,      0x0000 ) ;			
	RamWriteA( LXLMT,     0x7FFF ) ;			
	RamWriteA( LXLMT2,    0x7FFF ) ;			
	RamWriteA( LXLMTSD,   0x0000 ) ;			
	RamWriteA( PLXOFF,    0x0000 ) ;			
	RamWriteA( LXDODAT,   0x0000 ) ;			

	
	RegWriteA( LYEQEN , 0x45 );					
	RegWriteA( LYEQFC , 0x00 );					
	
	RamWriteA( ADHYOFF,   0x0000 ) ;			
	RamWriteA( HYINOD,    0x0000 ) ;			
	RamWriteA( HYDCIN,    0x0000 ) ;			
	RamWriteA( HYSEPT1,    0x0000 ) ;			
	RamWriteA( HYSEPT2,    0x0000 ) ;			
	RamWriteA( HYSEPT3,    0x0000 ) ;			
	RamWriteA( LYDOBZ,     0x0000 ) ;			
	RamWriteA( LYFZF,      0x0000 ) ;			
	RamWriteA( LYFZB,      0x0000 ) ;			
	RamWriteA( LYDX,      0x0000 ) ;			
	RamWriteA( LYLMT,     0x7FFF ) ;			
	RamWriteA( LYLMT2,    0x7FFF ) ;			
	RamWriteA( LYLMTSD,   0x0000 ) ;			
	RamWriteA( PLYOFF,    0x0000 ) ;			
	RamWriteA( LYDODAT,   0x0000 ) ;			

	
	RegWriteA( GNEQEN, 0x00 ) ;				
	RegWriteA( GNEQFC, 0x00 ) ;				
	RegWriteA( GNINADD, 0x0F ) ;			
	RegWriteA( GNOUTADD, 0x00 ) ;			

	RamWriteA( GNLMT, 0x7FFF ) ;			
	RamWriteA( GDX, 0x0000 ) ;				
	RamWriteA( GDOFFSET, 0x0000 ) ;			
	RamWriteA( GOFF, 0x0000 ) ;				

	
	RegWriteA( GDPXFC, 0x00 ) ;				

	
	RegWriteA( LCXFC, (unsigned char)0x00 ) ;			
	RegWriteA( LCYFC, (unsigned char)0x00 ) ;			

	RegWriteA( LCY1INADD, (unsigned char)LXDOIN ) ;		
	RegWriteA( LCY1OUTADD, (unsigned char)DLY00 ) ;		
	RegWriteA( LCX1INADD, (unsigned char)DLY00 ) ;		
	RegWriteA( LCX1OUTADD, (unsigned char)LXADOIN ) ;	

	RegWriteA( LCX2INADD, (unsigned char)LYDOIN ) ;		
	RegWriteA( LCX2OUTADD, (unsigned char)DLY01 ) ;		
	RegWriteA( LCY2INADD, (unsigned char)DLY01 ) ;		
	RegWriteA( LCY2OUTADD, (unsigned char)LYADOIN ) ;	

	
	RamWriteA( LCY1A0, 0x0000 ) ;			
	RamWriteA( LCY1A1, 0x4600 ) ;			
	RamWriteA( LCY1A2, 0x0000 ) ;			
	RamWriteA( LCY1A3, 0x3000 ) ;			
	RamWriteA( LCY1A4, 0x0000 ) ;			
	RamWriteA( LCY1A5, 0x0000 ) ;			
	RamWriteA( LCY1A6, 0x0000 ) ;			

	RamWriteA( LCX1A0, 0x0000 ) ;			
	RamWriteA( LCX1A1, 0x4600 ) ;			
	RamWriteA( LCX1A2, 0x0000 ) ;			
	RamWriteA( LCX1A3, 0x3000 ) ;			
	RamWriteA( LCX1A4, 0x0000 ) ;			
	RamWriteA( LCX1A5, 0x0000 ) ;			
	RamWriteA( LCX1A6, 0x0000 ) ;			

	RamWriteA( LCX2A0, 0x0000 ) ;			
	RamWriteA( LCX2A1, 0x4600 ) ;			
	RamWriteA( LCX2A2, 0x0000 ) ;			
	RamWriteA( LCX2A3, 0x3000 ) ;			
	RamWriteA( LCX2A4, 0x0000 ) ;			
	RamWriteA( LCX2A5, 0x0000 ) ;			
	RamWriteA( LCX2A6, 0x0000 ) ;			
	
	RamWriteA( LCY2A0, 0x0000 ) ;			
	RamWriteA( LCY2A1, 0x4600 ) ;			
	RamWriteA( LCY2A2, 0x0000 ) ;			
	RamWriteA( LCY2A3, 0x3000 ) ;			
	RamWriteA( LCY2A4, 0x0000 ) ;			
	RamWriteA( LCY2A5, 0x0000 ) ;			
	RamWriteA( LCY2A6, 0x0000 ) ;			
	
	RegWriteA( GDPX1INADD,  0x00 ) ;		
	RegWriteA( GDPX1OUTADD, 0x00 ) ;		
	RegWriteA( GDPX2INADD,  0x00 ) ;		
	RegWriteA( GDPX2OUTADD, 0x00 ) ;		
	RegWriteA( GDPX3INADD,  0x00 ) ;		
	RegWriteA( GDPX3OUTADD, 0x00 ) ;		

	RegWriteA( GDPYFC, 0x00 ) ;				
	RegWriteA( GDPY1INADD, 0x00 ) ;			
	RegWriteA( GDPY1OUTADD, 0x00 ) ;		
	RegWriteA( GDPY2INADD, 0x00 ) ;			
	RegWriteA( GDPY2OUTADD, 0x00 ) ;		
	RegWriteA( GDPY3INADD, 0x00 ) ;			
	RegWriteA( GDPY3OUTADD, 0x00 ) ;		
	
	
	RegWriteA( FFXEN, 0x00 ) ;				
	RegWriteA( FFXFC, 0x00 ) ;				
	RegWriteA( FFXDS, 0x00 ) ;				
	RegWriteA( FXINADD, 0x2C ) ;			
	RegWriteA( FXOUTADD, 0x49 ) ;			

	
	RegWriteA( FFYEN, 0x00 ) ;				
	RegWriteA( FFYFC, 0x00 ) ;				
	RegWriteA( FFYDS, 0x00 ) ;				
	RegWriteA( FYINADD, 0x6C ) ;			
	RegWriteA( FYOUTADD, 0x89 ) ;			
	
	
	RegWriteA( MSF1EN, 0x00 ) ;				
	RegWriteA( MSF2EN, 0x00 ) ;				
	RegWriteA( MSFDS,  0x00 ) ;				
	RegWriteA( MS1INADD, 0x46 ) ;			
	RegWriteA( MS1OUTADD, 0x00 ) ;			
	RegWriteA( MS2INADD, 0x47 ) ;			
	RegWriteA( MS2OUTADD, 0x00 ) ;			

	
	RegWriteA( GYINFC, 0x00 ) ;				

	
	RegWriteA( SWEN, 0x08 ) ;				
	RegWriteA( SWFC2, 0x08 ) ;				
	RegWriteA( SWSEL, 0x00 ) ;				
	RegWriteA( SINXADD, 0x00 ) ;			
	RegWriteA( SINYADD, 0x00 ) ;			

	
	RegWriteA( DAMONFC, 0x00 ) ;			
	RegWriteA( MDLY1ADD, 0x10 ) ;			
	RegWriteA( MDLY2ADD, 0x11 ) ;			

	
	BsyWit( DLYCLR, 0xFF ) ;				
	BsyWit( DLYCLR2, 0xEC ) ;				
	RegWriteA( DLYCLR	, 0x00 );			

	
	RegWriteA( RTXADD, 0x00 ) ;				
	RegWriteA( RTYADD, 0x00 ) ;				
	
	
	DrvSw( OFF ) ;							
	RegWriteA( DRVFC2	, 0x40 );			
	RegWriteA( DRVSELX	, 0x00 );			
	RegWriteA( DRVSELY	, 0x00 );			
 #ifdef LOWCURRENT
	RegWriteA( PWMFC,   0x4D ) ;			
 #else
	RegWriteA( PWMFC,   0x11 ) ;			
 #endif
	RegWriteA( PWMA,    0x00 ) ;			
	RegWriteA( PWMDLY1,  0x04 ) ;			
	RegWriteA( PWMDLY2,  0x04 ) ;			

	RegWriteA( LNA		, 0xC0 );			
	RegWriteA( LNFC 	, 0x02 );			
	RegWriteA( LNSMTHX	, 0x80 );			
	RegWriteA( LNSMTHY	, 0x80 );			

	RegWriteA( GEPWMFC, 0x01 ) ;			
	RegWriteA( GEPWMDLY, 0x00 ) ;			

	
	RegWriteA( MSMA, 0x00 ) ;				

	
	RegWriteA( FLGM, 0xCC ) ;				
	RegWriteA( FLGIST, 0xCC ) ;				
	RegWriteA( FLGIM2, 0xF8 ) ;				
	RegWriteA( FLGIST2, 0xF8 ) ;			

	
	RegWriteA( FCSW, 0x00 ) ;				
	RegWriteA( FCSW2, 0x00 ) ;				
	
	
	RamWriteA( HXSMSTP   , 0x0400 ) ;					
	RamWriteA( HYSMSTP   , 0x0400 ) ;					

	RegWriteA( SSSFC1, 0x43 ) ;				
	RegWriteA( SSSFC2, 0x03 ) ;				
	RegWriteA( SSSFC3, 0x50 ) ;				

	
	RegWriteA( SEOEN,  0x00 ) ;				
	RegWriteA( SEOFC1, 0x01 ) ;				
	RegWriteA( SEOFC2, 0x77 ) ;				
	RamWriteA( LXSEOLMT   , 0x7000 ) ;					
	RamWriteA( LYSEOLMT   , 0x7000 ) ;					

	RegWriteA( STBB, 0x00 ) ;				
	
}



#ifdef GAIN_CONT
  #define	TRI_LEVEL		0x3A03126F		
  #define	TRI_HIGH		0x3F800000		
  #define	TIMELOW			0x50			
  #define	TIMEMID			0x05			
  #define	TIMEHGH			0x05			
  #define	TIMEBSE			0x5D			
  #define	XMONADR			GXXFZ
  #define	YMONADR			GYXFZ
  #define	GANADR			gxadj
  #define	XMINGAIN		0x00000000
  #define	XMAXGAIN		0x3F800000
  #define	YMINGAIN		0x00000000
  #define	YMAXGAIN		0x3F800000
  #define	XSTEPUP			0x38D1B717		
  #define	XSTEPDN			0xBD4CCCCD		
  #define	YSTEPUP			0x38D1B717		
  #define	YSTEPDN			0xBD4CCCCD		
#endif

void	IniGyr( void )
{
	
	
	ClrGyr( 0x00 , CLR_GYR_ALL_RAM );
	
	
	RegWriteA( GEQSW	, 0x11 );		
	RegWriteA( GSHAKEON , 0x01 );		
	RegWriteA( GSHTON	, 0x00 );		
										
	RegWriteA( G2NDCEFON1,0x00 );		
	RegWriteA( G2NDCEFON0,0x00 );		
	RegWriteA( GADMEANON, 0x00 );		
	RegWriteA( GVREFADD , 0x14 );		
	RegWriteA( GSHTMOD , 0x0E );		
	RegWriteA( GLMT3MOD , 0x00 );		
										
	RegWriteA( GLMT3SEL , 0x00 );		
										
	RegWriteA( GGADON	, 0x01 );		
										
										
	RegWriteA( GGADSMP1 , 0x01 );		
	RegWriteA( GGADSMP0 , 0x00 );		
	RegWriteA( GGADSMPT , 0x0E);		
										
										

	
	RegWriteA( GDWNSMP1 , 0x00 );		
	RegWriteA( GDWNSMP2 , 0x00 );		
	RegWriteA( GDWNSMP3 , 0x00 );		
	
	
	RegWriteA( GEXPLMTH , 0x80 );		
	RegWriteA( GEXPLMTL , 0x5A );		
	
	
	RamWrite32A( gxlmt1L, 0x00000000 ) ;	
	RamWrite32A( gxlmt1H, GYRO_LMT1H ) ;	
	RamWrite32A( gylmt1L, 0x00000000 ) ;	
	RamWrite32A( gylmt1H, GYRO_LMT1H ) ;	
	RamWrite32A( gxlmt2L, 0x00000000 ) ;	
	RamWrite32A( gxlmt2H, 0x3F800000 ) ;	
	RamWrite32A( gylmt2L, 0x00000000 ) ;	
	RamWrite32A( gylmt2H, 0x3F800000 ) ;	
	RamWrite32A( gxlmt4SL, GYRO_LMT4L ) ;	
	RamWrite32A( gxlmt4SH, GYRO_LMT4H ) ;	
	RamWrite32A( gylmt4SL, GYRO_LMT4L ) ;	
	RamWrite32A( gylmt4SH, GYRO_LMT4H ) ;	

	
	RamWrite32A( gxlmt3H0, 0x3F000000 ) ;	
	RamWrite32A( gylmt3H0, 0x3F000000 ) ;	
	RamWrite32A( gxlmt3H1, 0x3F000000 ) ;	
	RamWrite32A( gylmt3H1, 0x3F000000 ) ;	

	
	RegWriteA( GDLYMON10, 0xF5 ) ;			
	RegWriteA( GDLYMON11, 0x01 ) ;			
	RegWriteA( GDLYMON20, 0xF5 ) ;			
	RegWriteA( GDLYMON21, 0x00 ) ;			
	RamWrite32A( gdm1g, 0x3F800000 ) ;		
	RamWrite32A( gdm2g, 0x3F800000 ) ;		
	RegWriteA( GDLYMON30, 0xF5 ) ;			
	RegWriteA( GDLYMON31, 0x01 ) ;			
	RegWriteA( GDLYMON40, 0xF5 ) ;			
	RegWriteA( GDLYMON41, 0x00 ) ;			
	RamWrite32A( gdm3g, 0x3F800000 ) ;		
	RamWrite32A( gdm4g, 0x3F800000 ) ;		
	RegWriteA( GPINMON3, 0x3C ) ;			
	RegWriteA( GPINMON4, 0x38 ) ;			
	
	
#ifdef	LMT1MODE
	RegWriteA( GDPI1ADD1, 0x01 );		
	RegWriteA( GDPI1ADD0, 0xC0 );		
	RegWriteA( GDPO1ADD1, 0x01 );		
	RegWriteA( GDPO1ADD0, 0xCB );		
	RegWriteA( GDPI2ADD1, 0x00 );		
	RegWriteA( GDPI2ADD0, 0xC0 );		
	RegWriteA( GDPO2ADD1, 0x00 );		
	RegWriteA( GDPO2ADD0, 0xCB );		
#else
	RegWriteA( GDPI1ADD1, 0x01 );		
	RegWriteA( GDPI1ADD0, 0xC0 );		
	RegWriteA( GDPO1ADD1, 0x01 );		
	RegWriteA( GDPO1ADD0, 0xC0 );		
	RegWriteA( GDPI2ADD1, 0x00 );		
	RegWriteA( GDPI2ADD0, 0xC0 );		
	RegWriteA( GDPO2ADD1, 0x00 );		
	RegWriteA( GDPO2ADD0, 0xC0 );		
#endif
	
	
	RegWriteA( GSINTST	, 0x00 );		
										
	
	
	RegWriteA( GPANADDA, 		0x14 );		
	RegWriteA( GPANADDB, 		0x0E );		
	
	 
	RamWrite32A( SttxHis, 	0x00000000 );			
	RamWrite32A( SttyHis, 	0x00000000 );			
	RamWrite32A( SttxaL, 	0x00000000 );			
	RamWrite32A( SttxbL, 	0x00000000 );			
	RamWrite32A( Sttx12aM, 	GYRA12_MID );	
	RamWrite32A( Sttx12aH, 	GYRA12_HGH );	
	RamWrite32A( Sttx12bM, 	GYRB12_MID );	
	RamWrite32A( Sttx12bH, 	GYRB12_HGH );	
	RamWrite32A( Sttx34aM, 	GYRA34_MID );	
	RamWrite32A( Sttx34aH, 	GYRA34_HGH );	
	RamWrite32A( Sttx34bM, 	GYRB34_MID );	
	RamWrite32A( Sttx34bH, 	GYRB34_HGH );	
	RamWrite32A( SttyaL, 	0x00000000 );			
	RamWrite32A( SttybL, 	0x00000000 );			
	RamWrite32A( Stty12aM, 	GYRA12_MID );	
	RamWrite32A( Stty12aH, 	GYRA12_HGH );	
	RamWrite32A( Stty12bM, 	GYRB12_MID );	
	RamWrite32A( Stty12bH, 	GYRB12_HGH );	
	RamWrite32A( Stty34aM, 	GYRA34_MID );	
	RamWrite32A( Stty34aH, 	GYRA34_HGH );	
	RamWrite32A( Stty34bM, 	GYRB34_MID );	
	RamWrite32A( Stty34bH, 	GYRB34_HGH );	
	
	
	RegWriteA( GPANLEVABS, 		0x00 );		
	
	
	RegWriteA( GPANSTT1DWNSMP0, 0x00 );		
	RegWriteA( GPANSTT1DWNSMP1, 0x00 );		
	RegWriteA( GPANSTT2DWNSMP0, 0x00 );		
	RegWriteA( GPANSTT2DWNSMP1, 0x00 );		
	RegWriteA( GPANSTT3DWNSMP0, 0x00 );		
	RegWriteA( GPANSTT3DWNSMP1, 0x00 );		
	RegWriteA( GPANSTT4DWNSMP0, 0x00 );		
	RegWriteA( GPANSTT4DWNSMP1, 0x00 );		
	RegWriteA( GMEANAUTO, 		0x01 );		

	
	RegWriteA( GPANSTTFRCE, 	0x00 );		
	
	
	
	RegWriteA( GPANSTT21JUG0, 	0x00 );		
	RegWriteA( GPANSTT21JUG1, 	0x00 );		
	
	RegWriteA( GPANSTT31JUG0, 	0x00 );		
	RegWriteA( GPANSTT31JUG1, 	0x00 );		
	
	RegWriteA( GPANSTT41JUG0, 	0x13 );		
	RegWriteA( GPANSTT41JUG1, 	0x00 );		
	
	RegWriteA( GPANSTT12JUG0, 	0x00 );		
	RegWriteA( GPANSTT12JUG1, 	0x07 );		
	
	RegWriteA( GPANSTT13JUG0, 	0x00 );		
	RegWriteA( GPANSTT13JUG1, 	0x00 );		
	
	RegWriteA( GPANSTT23JUG0, 	0x11 );		
	RegWriteA( GPANSTT23JUG1, 	0x01 );		
	
	RegWriteA( GPANSTT43JUG0, 	0x00 );		
	RegWriteA( GPANSTT43JUG1, 	0x00 );		
	
	RegWriteA( GPANSTT34JUG0, 	0x11 );		
	RegWriteA( GPANSTT34JUG1, 	0x01 );		
	
	RegWriteA( GPANSTT24JUG0, 	0x00 );		
	RegWriteA( GPANSTT24JUG1, 	0x00 );		
	
	RegWriteA( GPANSTT42JUG0, 	0x00 );		
	RegWriteA( GPANSTT42JUG1, 	0x00 );		

	
	RegWriteA( GPANSTT1LEVTMR, 	0x00 );		
	RegWriteA( GPANSTT2LEVTMR, 	0x00 );		
	RegWriteA( GPANSTT3LEVTMR, 	0x00 );		
	RegWriteA( GPANSTT4LEVTMR, 	0x03 );		
	
	
#ifdef	LMT1MODE
	RegWriteA( GPANTRSON0, 		0x07 );		
	RegWriteA( GPANTRSON1, 		0x1E );		
#else
	RegWriteA( GPANTRSON0, 		0x01 );		
	RegWriteA( GPANTRSON1, 		0x1C );		
#endif
	
	
	RegWriteA( GPANSTTSETGYRO, 	0x00 );		
#ifdef	LMT1MODE
	RegWriteA( GPANSTTSETGAIN, 	0x00 );		
	RegWriteA( GPANSTTSETISTP, 	0x00 );		
	RegWriteA( GPANSTTSETI1FTR,	0x58 );		
	RegWriteA( GPANSTTSETI2FTR,	0x00 );		
#else
	RegWriteA( GPANSTTSETGAIN, 	0x10 );		
	RegWriteA( GPANSTTSETISTP, 	0x00 );		
	RegWriteA( GPANSTTSETI1FTR,	0x90 );		
	RegWriteA( GPANSTTSETI2FTR,	0x90 );		
#endif
	RegWriteA( GPANSTTSETL2FTR, 0x00 );		
	RegWriteA( GPANSTTSETL3FTR,	0x00 );		
	RegWriteA( GPANSTTSETL4FTR,	0x00 );		
	
	
	RegWriteA( GPANSTTSETILHLD,	0x00 );		
	
	
	RegWriteA( GPANSTTSETHPS,	0xF0 );		
	RegWriteA( GHPSMOD,			0x00 );		
	RegWriteA( GPANHPSTMR0,		0x5C );		
	RegWriteA( GPANHPSTMR1,		0x00 );		
	
	
	RegWriteA( GPANSTT2TMR0,	0x01 );		
	RegWriteA( GPANSTT2TMR1,	0x00 );		
	RegWriteA( GPANSTT4TMR0,	0x02 );		
	RegWriteA( GPANSTT4TMR1,	0x00 );		
	
	RegWriteA( GPANSTTXXXTH,	0x00 );		

#ifdef GAIN_CONT
	RamWrite32A( gxlevmid, TRI_LEVEL );					
	RamWrite32A( gxlevhgh, TRI_HIGH );					
	RamWrite32A( gylevmid, TRI_LEVEL );					
	RamWrite32A( gylevhgh, TRI_HIGH );					
	RamWrite32A( gxadjmin, XMINGAIN );					
	RamWrite32A( gxadjmax, XMAXGAIN );					
	RamWrite32A( gxadjdn, XSTEPDN );					
	RamWrite32A( gxadjup, XSTEPUP );					
	RamWrite32A( gyadjmin, YMINGAIN );					
	RamWrite32A( gyadjmax, YMAXGAIN );					
	RamWrite32A( gyadjdn, YSTEPDN );					
	RamWrite32A( gyadjup, YSTEPUP );					
	
	RegWriteA( GLEVGXADD, (unsigned char)XMONADR );		
	RegWriteA( GLEVGYADD, (unsigned char)YMONADR );		
	RegWriteA( GLEVTMR, 		TIMEBSE );				
	RegWriteA( GLEVTMRLOWGX, 	TIMELOW );				
	RegWriteA( GLEVTMRMIDGX, 	TIMEMID );				
	RegWriteA( GLEVTMRHGHGX, 	TIMEHGH );				
	RegWriteA( GLEVTMRLOWGY, 	TIMELOW );				
	RegWriteA( GLEVTMRMIDGY, 	TIMEMID );				
	RegWriteA( GLEVTMRHGHGY, 	TIMEHGH );				
	RegWriteA( GLEVFILMOD, 		0x00 );					
	RegWriteA( GADJGANADD, (unsigned char)GANADR );		
	RegWriteA( GADJGANGO, 		0x00 );					

	
	AutoGainControlSw( OFF ) ;							
#endif
	
	
	RegWriteA( GEQON	, 0x01 );		


}



void	IniHfl( void )
{
	unsigned short	UsAryId ;
	
	
	
	UsAryId	= 0 ;
	while( CsHalReg[ UsAryId ].UsRegAdd != 0xFFFF )
	{
		RegWriteA( CsHalReg[ UsAryId ].UsRegAdd, CsHalReg[ UsAryId ].UcRegDat ) ;
		UsAryId++ ;
	}

	
	UsAryId	= 0 ;
	while( CsHalFil[ UsAryId ].UsRamAdd != 0xFFFF )
	{
		RamWriteA( CsHalFil[ UsAryId ].UsRamAdd, CsHalFil[ UsAryId ].UsRamDat ) ;
		UsAryId++ ;
	}
	
}



void	IniGfl( void )
{
 	unsigned short	UsAryId ;
	
	
	UsAryId	= 0 ;
	while( CsGyrFil[ UsAryId ].UsRamAdd != 0xFFFF )
	{
		RamWrite32A( CsGyrFil[ UsAryId ].UsRamAdd, CsGyrFil[ UsAryId ].UlRamDat ) ;
		UsAryId++ ;
	}
	
}



void	IniAdj( void )
{
	
	RegWriteA( CMSDAC, BAIS_CUR ) ;				
	RegWriteA( OPGSEL, AMP_GAIN ) ;				
	
#ifdef USE_EXE2PROM
	unsigned short	UsAdjCompF ;
	unsigned short	UsAdjData ;
	unsigned long	UlAdjData ;
	
	E2pRed( (unsigned short)ADJ_COMP_FLAG, 2, ( unsigned char * )&UsAdjCompF ) ;	
	
	
	if( (UsAdjCompF == 0x0000 ) || (UsAdjCompF & ( EXE_HXADJ - EXE_END )) ){
		RamWriteA( DAHLXO, DAHLXO_INI ) ;	
		RamWriteA( DAHLXB, DAHLXB_INI ) ;	
		RamWriteA( ADHXOFF, 0x0000 ) ;		
	}else{
		E2pRed( (unsigned short)HALL_OFFSET_X, 2, ( unsigned char * )&UsAdjData ) ;
		RamWriteA( DAHLXO, UsAdjData ) ;	
		E2pRed( (unsigned short)HALL_BIAS_X, 2, ( unsigned char * )&UsAdjData ) ;
		RamWriteA( DAHLXB, UsAdjData ) ;	
		E2pRed( (unsigned short)HALL_AD_OFFSET_X, 2, ( unsigned char * )&UsAdjData ) ;
		RamWriteA( ADHXOFF, UsAdjData ) ;	
	}
			
	
	if( (UsAdjCompF == 0x0000 ) || (UsAdjCompF & ( EXE_HYADJ - EXE_END )) ){
		RamWriteA( DAHLYO, DAHLYO_INI ) ;	
		RamWriteA( DAHLYB, DAHLYB_INI ) ;	
		RamWriteA( ADHYOFF, 0x0000 ) ;		
	}else{
		E2pRed( (unsigned short)HALL_OFFSET_Y, 2, ( unsigned char * )&UsAdjData ) ;
		RamWriteA( DAHLYO, UsAdjData ) ;	
		E2pRed( (unsigned short)HALL_BIAS_Y, 2, ( unsigned char * )&UsAdjData ) ;
		RamWriteA( DAHLYB, UsAdjData ) ;	
		E2pRed( (unsigned short)HALL_AD_OFFSET_Y, 2, ( unsigned char * )&UsAdjData ) ;
		RamWriteA( ADHYOFF, UsAdjData ) ;	
	}
			
	
	if( (UsAdjCompF == 0x0000 ) || (UsAdjCompF & ( EXE_LXADJ - EXE_END )) ){
		RamWriteA( lxgain, LXGAIN_INI ) ;	
	}else{
		E2pRed( (unsigned short)LOOP_GAIN_X, 2, ( unsigned char * )&UsAdjData ) ;
		RamWriteA( lxgain, UsAdjData ) ;	
	}
		
	
	if( (UsAdjCompF == 0x0000 ) || (UsAdjCompF & ( EXE_LYADJ - EXE_END )) ){
		RamWriteA( lygain, LYGAIN_INI ) ;	
	}else{
		E2pRed( (unsigned short)LOOP_GAIN_Y, 2, ( unsigned char * )&UsAdjData ) ;
		RamWriteA( lygain, UsAdjData ) ;	
	}
		
	
	E2pRed( (unsigned short)OPT_CENTER_X, 2, ( unsigned char * )&UsAdjData ) ;
	if( ( UsAdjData != 0x0000 ) && ( UsAdjData != 0xffff )){
		UsCntXof = UsAdjData ;					
	} else {
		UsCntXof = OPTCEN_X ;						
	}
	RamWriteA( HXINOD, UsCntXof ) ;				

	
	E2pRed( (unsigned short)OPT_CENTER_Y, 2, ( unsigned char * )&UsAdjData ) ;
	if( ( UsAdjData != 0x0000 ) && ( UsAdjData != 0xffff )){
		UsCntYof = UsAdjData ;					
	} else {
		UsCntYof = OPTCEN_Y ;						
	}
	RamWriteA( HYINOD, UsCntYof ) ;				
	
	
	E2pRed( (unsigned short)GYRO_AD_OFFSET_X, 2, ( unsigned char * )&UsAdjData ) ;
	if( ( UsAdjData == 0x0000 ) || ( UsAdjData == 0xffff )){
		RamWriteA( ADGXOFF, 0x0000 ) ;							
		RegWriteA( IZAH, DGYRO_OFST_XH ) ;						
		RegWriteA( IZAL, DGYRO_OFST_XL ) ;						
	}else{
		RamWriteA( ADGXOFF, 0x0000 ) ;							
		RegWriteA( IZAH, (unsigned char)(UsAdjData >> 8) ) ;	
		RegWriteA( IZAL, (unsigned char)(UsAdjData) ) ;			
	}
	
	
	E2pRed( (unsigned short)GYRO_AD_OFFSET_Y, 2, ( unsigned char * )&UsAdjData ) ;
	if( ( UsAdjData == 0x0000 ) || ( UsAdjData == 0xffff )){
		RamWriteA( ADGYOFF, 0x0000 ) ;							
		RegWriteA( IZBH, DGYRO_OFST_YH ) ;						
		RegWriteA( IZBL, DGYRO_OFST_YL ) ;						
	}else{
		RamWriteA( ADGYOFF, 0x0000 ) ;							
		RegWriteA( IZBH, (unsigned char)(UsAdjData >> 8) ) ;	
		RegWriteA( IZBL, (unsigned char)(UsAdjData) ) ;			
	}
		
	
	E2pRed( (unsigned short)GYRO_GAIN_X, 4 , ( unsigned char * )&UlAdjData ) ;
	if( ( UlAdjData != 0x00000000 ) && ( UlAdjData != 0xffffffff )){
		RamWrite32A( gxzoom, UlAdjData ) ;		
	}else{
		RamWrite32A( gxzoom, GXGAIN_INI ) ;		
	}
	
	
	E2pRed( (unsigned short)GYRO_GAIN_Y, 4 , ( unsigned char * )&UlAdjData ) ;
	if( ( UlAdjData != 0x00000000 ) && ( UlAdjData != 0xffffffff )){
		RamWrite32A( gyzoom, UlAdjData ) ;		
	}else{
		RamWrite32A( gyzoom, GXGAIN_INI ) ;		
	}
	
	
	E2pRed( (unsigned short)OSC_CLK_VAL, 2 , ( unsigned char * )&UsAdjData ) ;
	if((unsigned char)UsAdjData != 0xff ){
		UsCntYof = UsAdjData ;					
		RegWriteA( OSCSET, (unsigned char)UsAdjData ) ;		
	}else{
		RegWriteA( OSCSET, OSC_INI ) ;						
	}
	
	
#else	
	
	RegWriteA( IZAH,	DGYRO_OFST_XH ) ;	
	RegWriteA( IZAL,	DGYRO_OFST_XL ) ;	
	RegWriteA( IZBH,	DGYRO_OFST_YH ) ;	
	RegWriteA( IZBL,	DGYRO_OFST_YL ) ;	
	
	RamWriteA( DAHLXO, DAHLXO_INI ) ;		
	RamWriteA( DAHLXB, DAHLXB_INI ) ;		
	RamWriteA( DAHLYO, DAHLYO_INI ) ;		
	RamWriteA( DAHLYB, DAHLYB_INI ) ;		
	RamWriteA( ADHXOFF, ADHXOFF_INI ) ;		
	RamWriteA( ADHYOFF, ADHYOFF_INI ) ;		
	RamWriteA( lxgain, LXGAIN_INI ) ;		
	RamWriteA( lygain, LYGAIN_INI ) ;		
	RamWriteA( ADGXOFF, 0x0000 ) ;			
	RamWriteA( ADGYOFF, 0x0000 ) ;			
	UsCntXof = OPTCEN_X ;					
	UsCntYof = OPTCEN_Y ;					
	RamWriteA( HXINOD, UsCntXof ) ;			
	RamWriteA( HYINOD, UsCntYof ) ;			
	RamWrite32A( gxzoom, GXGAIN_INI ) ;		
	RamWrite32A( gyzoom, GYGAIN_INI ) ;		

	RegWriteA( OSCSET, OSC_INI ) ;			
	
#endif	
	
	RamWriteA( pzgxp, PZGXP_INI ) ;			
	RamWriteA( pzgyp, PZGYP_INI ) ;			
	
	RamWriteA( hxinog, 0x7fff ) ;			
	RamWriteA( hyinog, 0x7fff ) ;			
	
	SetZsp(0) ;								
	
	RegWriteA( PWMA 	, 0xC0 );			

	RegWriteA( STBB 	, 0x0F );							

	RegWriteA( LXEQEN 	, 0x45 );			
	RegWriteA( LYEQEN 	, 0x45 );			
	
	SetPanTiltMode( OFF ) ;					
#ifdef H1COEF_CHANGER
	SetH1cMod( ACTMODE ) ;					
#endif
	
	DrvSw( ON ) ;							
}



void	IniCmd( void )
{

	MemClr( ( unsigned char * )&StAdjPar, sizeof( stAdjPar ) ) ;	
	MemClr( ( unsigned char * )&StLbgCon, sizeof( stLbgCon ) ) ;	
	
}


void	BsyWit( unsigned short	UsTrgAdr, unsigned char	UcTrgDat )
{
	unsigned char	UcFlgVal ;

	RegWriteA( UsTrgAdr, UcTrgDat ) ;	

	UcFlgVal	= 1 ;

	while( UcFlgVal ) {
		RegReadA( FLGM, &UcFlgVal ) ;		
		UcFlgVal	&= 0x40 ;
	} ;

}


void	Bsy2Wit( void )
{
	unsigned char	UcFlgVal ;

	UcFlgVal	= 1 ;

	while( UcFlgVal ) {
		RegReadA( BSYSEL, &UcFlgVal ) ;
		UcFlgVal	&= 0x80 ;
	} ;

}


void	MemClr( unsigned char	*NcTgtPtr, unsigned short	UsClrSiz )
{
	unsigned short	UsClrIdx ;

	for ( UsClrIdx = 0 ; UsClrIdx < UsClrSiz ; UsClrIdx++ )
	{
		*NcTgtPtr	= 0 ;
		NcTgtPtr++ ;
	}
}


#if 0
void	WitTim( unsigned short	UsWitTim )
{
	unsigned long	UlLopIdx, UlWitCyc ;

	UlWitCyc	= ( unsigned long )( ( float )UsWitTim / NOP_TIME / ( float )12 ) ;

	for( UlLopIdx = 0 ; UlLopIdx < UlWitCyc ; UlLopIdx++ )
	{
		;
	}
}
#endif

void	GyOutSignal( void )
{

	RegWriteA( GRADR0,	GYROX_INI ) ;			
	RegWriteA( GRADR1,	GYROY_INI ) ;			
	
	
	RegWriteA( GRSEL	, 0x02 );							

}

#ifdef STANDBY_MODE
void	AccWit( unsigned char UcTrgDat )
{
	unsigned char	UcFlgVal ;

	UcFlgVal	= 1 ;

	while( UcFlgVal ) {
		RegReadA( GRACC, &UcFlgVal ) ;
		UcFlgVal	&= UcTrgDat ;
	} ;

}

void	SelectGySleep( unsigned char UcSelMode )
{
	unsigned char	UcRamIni ;
	unsigned char	UcGrini ;

	if(UcSelMode == ON)
	{
		RegWriteA( GEQON, 0x00 ) ;			
		RegWriteA( GRSEL,	0x01 ) ;		

		RegReadA( GRINI	, &UcGrini );					
		RegWriteA( GRINI, ( UcGrini | SLOWMODE) );		
		
		RegWriteA( GRADR0,	0x6B ) ;		
		RegWriteA( GRACC,	0x02 ) ;		
		AccWit( 0x10 ) ;					
		RegReadA( GRADT0H, &UcRamIni ) ;	
		
		UcRamIni |= 0x40 ;					
		
		RegWriteA( GRADR0,	0x6B ) ;		
		RegWriteA( GSETDT,	UcRamIni ) ;	
		RegWriteA( GRACC,	0x10 ) ;		
		AccWit( 0x10 ) ;					

	}
	else
	{
		RegWriteA( GRADR0,	0x6B ) ;		
		RegWriteA( GRACC,	0x02 ) ;		
		AccWit( 0x10 ) ;					
		RegReadA( GRADT0H, &UcRamIni ) ;	
		
		UcRamIni &= ~0x40 ;					
		
		RegWriteA( GSETDT,	UcRamIni ) ;	
		RegWriteA( GRACC,	0x10 ) ;		
		AccWit( 0x10 ) ;					

		RegReadA( GRINI	, &UcGrini );					
		RegWriteA( GRINI, ( UcGrini & ~SLOWMODE) );		
		
		GyOutSignal( ) ;					
		
		WitTim( 50 ) ;						
		
		RegWriteA( GEQON, 0x01 ) ;			
		ClrGyr( 0x06 , CLR_GYR_DLY_RAM );
	}
}
#endif

#ifdef	GAIN_CONT
void	AutoGainControlSw( unsigned char UcModeSw )
{

	if( UcModeSw == OFF )
	{
		RegWriteA( GADJGANGXMOD, 	0xE0 );					
		RegWriteA( GADJGANGYMOD, 	0xE0 );					
		RamWrite32A( GANADR			 , XMAXGAIN ) ;			
		RamWrite32A( GANADR | 0x0100 , YMAXGAIN ) ;			
	}
	else
	{
		RegWriteA( GADJGANGXMOD, 	0xE7 );					
		RegWriteA( GADJGANGYMOD, 	0xE7 );					
	}

}
#endif


void	ClrGyr( unsigned char UcClrFil , unsigned char UcClrMod )
{
	unsigned char	UcRamClr;
	unsigned char	UcClrBit;

	while(1){
		if( UcClrMod == CLR_GYR_DLY_RAM )
		{
			if( UcClrFil & 0x10 ){
				UcClrBit = 0x10 ;
			}else if( UcClrFil & 0x08 ){
				UcClrBit = 0x08 ;
			}else if( UcClrFil & 0x04 ){
				UcClrBit = 0x04 ;
			}else if( UcClrFil & 0x02 ){
				UcClrBit = 0x02 ;
			}else if( UcClrFil & 0x01 ){
				UcClrBit = 0x01 ;
			}else{
				UcClrBit = 0x00 ;
			}
				
			UcClrFil &= ~UcClrBit ;

		}else{
			UcClrBit = 0x00 ;
		}
		
		
		RegWriteA( GRAMDLYMOD	, UcClrBit ) ;	
												
												

		
		RegWriteA( GRAMINITON	, UcClrMod ) ;	
		
		
		do{
			RegReadA( GRAMINITON, &UcRamClr );
			UcRamClr &= 0x03;
		}while( UcRamClr != 0x00 );

		if(( UcClrMod != CLR_GYR_DLY_RAM ) || ( UcClrFil == 0x00 )){
			break ;
		}
	}
}


void	DrvSw( unsigned char UcDrvSw )
{

	if( UcDrvSw == ON )
	{
		if( UcPwmMod == PWMMOD_CVL ) {
			RegWriteA( DRVFC	, 0xE3 );			
		}else{
#ifdef	LOWCURRENT
		RegWriteA( DRVFC	, 0x03 );			
#else
		RegWriteA( DRVFC	, 0xC3 );			
#endif
		}
	}
	else
	{
		RegWriteA( DRVFC	, 0x00 );				
	}
}


