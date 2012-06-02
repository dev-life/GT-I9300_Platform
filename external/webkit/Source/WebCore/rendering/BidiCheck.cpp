/****************************************************************************
* File name : BidiCheck.c
* Project :   
* Module :    
* Date :      
* Version :   1
*----------------------------------------------------------------------------*
*----------------------------------------------------------------------------*
*                                    DESCRIPTION
*
* SAMSUNG INDIA SOFTWARE OPERATIONS
*
*----------------------------------------------------------------------------*
*----------------------------------------------------------------------------*
*
*----------------------------------------------------------------------------*
*----------------------------------------------------------------------------*
*                                    EVOLUTION
*----------------------------------------------------------------------------*
* Date		| Author		| Description
*----------------------------------------------------------------------------*
* 07/01/2011| SISO	| Creation (Version 1.0)

***************************************************************************/

#include "BidiCheck.h"
//#include "ArabicGlyphDebug.h"
#include <stdlib.h>
#include <string.h>

int BidiCheck::isArabicChar(u16 testChar) 
{
	/* 0x0621 to 0x065F and 0x066E to 0x06D3 */
	if ( (testChar >= LETTER_ARABIC_BASE_CHAR_1_START && testChar <= LETTER_ARABIC_BASE_CHAR_1_END) ||
		 (testChar >= LETTER_ARABIC_BASE_CHAR_2_START && testChar <= LETTER_ARABIC_BASE_CHAR_2_END) ) {
		return TRUE;
	}
	else if((testChar >= 0x0590 && testChar <= 0x05FF) || (testChar >= 0xFB00  && testChar <= 0xFB4F)){
		return TRUE;
	}

	return FALSE;
}

int BidiCheck::isArabicGlyphChar(u16 testChar)
{
	/* 64336 to 65023 and 65136 to 65276*/
	if( (testChar >= LETTER_ARABIC_GLYPH_FBCD_START && testChar <= LETTER_ARABIC_GLYPH_FBCD_END) ||
		(testChar >= LETTER_ARABIC_GLYPH_FE_START && testChar <= LETTER_ARABIC_GLYPH_FE_END) )
		return TRUE;
	return FALSE;
}

int BidiCheck::isNumber(u16 testChar)
{
	/* 0x0030 to 0x0039 */
	if(testChar >= LETTER_ENGLISH_NUMBERS_START && testChar <= LETTER_ENGLISH_NUMBERS_END)
		return TRUE;
	return FALSE;
}

int BidiCheck::isArabicNumber(u16 testChar)
{
	/* 0x0660 to 0x0669 and 0x06F0 to 0x06F9 */
	if( (testChar >= LETTER_ARABIC_NUMBERS_START && testChar <= LETTER_ARABIC_NUMBERS_END) ||
		(testChar >= LETTER_FARSI_NUMBERS_START && testChar <= LETTER_FARSI_NUMBERS_END) )
		return TRUE;
	return FALSE;
}

int BidiCheck::isArabicSymbols(u16 testChar)
{
	switch(testChar)
	{
		case LETTER_ARABIC_INDIC_CUBEROOT:
		case LETTER_ARABIC_INDIC_FOURTHROOT:
		case LETTER_ARABIC_RAY:
		case LETTER_ARABIC_INDIC_PER_MILLE_SIGN:
		case LETTER_ARABIC_INDIC_PER_TENTHOUSAND_SIGN:
		case LETTER_ARABIC_AFGHANI_SIGN:
		case LETTER_ARABIC_COMMA:
		case LETTER_ARABIC_DATE_SEPARATOR:
		//case LETTER_ARABIC_SEMICOLON:
		case LETTER_ARABIC_TRIPE_DOT_PUNCTUATION_MARK:
		//case LETTER_ARABIC_QUESTIONMARK:
		case LETTER_ARABIC_PERCENT:
		case LETTER_ARABIC_DECIMAL_SEPARATOR:
		case LETTER_ARABIC_THOUSANDS_SEPARATOR:
		case LETTER_ARABIC_FIVE_STAR:
		case LETTER_ARABIC_FULLSTOP:
			return TRUE;
		default:
			return FALSE;
	}
	return FALSE;
}

int BidiCheck::isGeneralSymbols(u16 testChar)
{
	/*	
		8192 to 8303 - General punctuation marks 
		8352 to 8399 - Currency symbols
		8448 TO 8527 - Letterlike symbols (TM)
		65510 - Korean won
	*/
	if( (testChar >= LETTER_GENERAL_PUNCTUATION_START && testChar <= LETTER_GENERAL_PUNCTUATION_END) ||
		(testChar >= LETTER_GENERAL_CURRENCY_START && testChar <= LETTER_GENERAL_CURRENCY_END) ||
		(testChar >= LETTER_GENERAL_LETTERLIKE_START && testChar <= LETTER_GENERAL_LETTERLIKE_END) ||
		(testChar == LETTER_GENERAL_CURRENCY_KOREANWON) )
		return TRUE;
		
	return FALSE;
}

int BidiCheck::isSymbols(u16 testChar)
{
	if( (testChar >= 32 && testChar <= 47) || 
		(testChar >= 58 && testChar <= 64) || 
		(testChar >= 91 && testChar <= 95) || 
		(testChar >= 123 && testChar <= 126) ||
		/*(testChar == 171 || testChar == 187)*/
		(testChar >= 128 && testChar <= 254) ) /* Extended Ascii table*/	
			return TRUE;
	
	/*	163, 164, 165, 166, 171, 172, 176, 177*/
	return FALSE;
}


int BidiCheck::applyRTLForString(u16 *inStr, int length)
{
	int i=0;
	while(i < length)
	{
		if( isSymbols(inStr[i]) || isNumber(inStr[i]) || isArabicSymbols(inStr[i]) || isGeneralSymbols(inStr[i]))
		{
		}
		else
		{
			if( isArabicChar(inStr[i]) || isArabicGlyphChar(inStr[i]) ||
				inStr[i] == LETTER_ARABIC_SEMICOLON || inStr[i] == LETTER_ARABIC_QUESTIONMARK )	
				return TRUE;
			else
				return FALSE;
		}
		i++;
	}
	return FALSE;
}

