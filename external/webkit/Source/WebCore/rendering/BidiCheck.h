#ifndef BidiCheck_h
#define BidiCheck_h

typedef unsigned short u16;
typedef unsigned char u8;


#define FALSE 0
#define TRUE 1
#define _URDU_ 1

// Arabic - Full unicode range - 0x06**, 0xFB**, 0xFC**, 0xFD**, 0xFE** series
#define LETTER_ARABIC_START							0x0600 /* 1536 */
#define LETTER_ARABIC_END							0x06FF /* 1791 */ 
#define LETTER_ARABIC_GLYPH_FBCD_START				0xFB50 /* 64336 */
#define LETTER_ARABIC_GLYPH_FBCD_END				0xFDFF /* 65023 */
#define LETTER_ARABIC_GLYPH_FE_START				0xFE70 /* 65136 */
#define LETTER_ARABIC_GLYPH_FE_END					0xFEFC /* 65276 */

//Arabic base characters
#define LETTER_ARABIC_BASE_CHAR_1_START				0x0621 /* 1569 */
#define LETTER_ARABIC_BASE_CHAR_1_END				0x065F /* 1631 - Changed from 1618 */
#define LETTER_ARABIC_BASE_CHAR_2_START				0x066E /* 1646 - Changed from 1649  */
#define LETTER_ARABIC_BASE_CHAR_2_END				0x06D3 /* 1747 */

//Tashkeel characters
#define LETTER_ARABIC_TASHKEEL_START				0x064B /* 1611 */
#define LETTER_ARABIC_TASHKEEL_END					0x065F /* 1618 */

#define LETTER_ARABIC_TASHKEEL_61X_START 			0x0610 /* 1552 */
#define LETTER_ARABIC_TASHKEEL_61X_END				0x061A /* 1562 */

#define LETTER_ARABIC_TASHKEEL_6DX_START 			0x06D6 /* 1750 */
#define LETTER_ARABIC_TASHKEEL_6DX_END				0x06ED /* 1773 */

#define LETTER_ARABIC_TASHKEEL_670					0x0670 /* 1648 */

//Ligatures from IME
#define LETTER_ARABIC_LIGATURE_FROM_IME_1			0xFEF5 /* 65269 */
#define LETTER_ARABIC_LIGATURE_FROM_IME_2			0xFEF7 /* 65271 */
#define LETTER_ARABIC_LIGATURE_FROM_IME_3			0xFEF9 /* 65273 */
#define LETTER_ARABIC_LIGATURE_FROM_IME_4			0xFEFB /* 65275 */

/*
#define LETTER_ARABIC_LIGATURE_LAM_ALEF_MADDA_ABOVE_ISOLATED		0xFEF5
#define LETTER_ARABIC_LIGATURE_LAM_ALEF_HAMZA_ABOVE_ISOLATED		0xFEF7
#define LETTER_ARABIC_LIGATURE_LAM_ALEF_HAMZA_BELOW_ISOLATED		0xFEF9
#define LETTER_ARABIC_LIGATURE_LAM_ALEF_ISOLATED					0xFEFB
*/

//0x0649 character
#define LETTER_ARABIC_ALEF_MAKSURA					0x0649 /* 1609 */

//Arabic numbers
#define LETTER_ARABIC_NUMBERS_START					0x0660 /* 1632 */
#define LETTER_ARABIC_NUMBERS_END					0x0669 /* 1641 */
#define LETTER_FARSI_NUMBERS_START					0x06F0 /* 1776 */
#define LETTER_FARSI_NUMBERS_END					0x06F9 /* 1785 */

//Arabic Symbols
#define LETTER_ARABIC_INDIC_CUBEROOT				0x0606
#define LETTER_ARABIC_INDIC_FOURTHROOT				0x0607
#define LETTER_ARABIC_RAY							0x0608
#define LETTER_ARABIC_INDIC_PER_MILLE_SIGN			0x0609
#define LETTER_ARABIC_INDIC_PER_TENTHOUSAND_SIGN	0x060A
#define LETTER_ARABIC_AFGHANI_SIGN					0x060B
#define LETTER_ARABIC_COMMA							0x060C /* 1548 */
#define LETTER_ARABIC_DATE_SEPARATOR				0x060D
#define LETTER_ARABIC_SEMICOLON						0x061B /* 1563 */
#define LETTER_ARABIC_TRIPE_DOT_PUNCTUATION_MARK	0x061E
#define LETTER_ARABIC_QUESTIONMARK					0x061F /* 1567 */

#define LETTER_ARABIC_PERCENT						0x066A
#define LETTER_ARABIC_DECIMAL_SEPARATOR				0x066B
#define LETTER_ARABIC_THOUSANDS_SEPARATOR			0x066C
#define LETTER_ARABIC_FIVE_STAR						0x066D
#define LETTER_ARABIC_FULLSTOP						0x06D4

//General punctuation marks
#define LETTER_GENERAL_PUNCTUATION_START			0x2000 /* 8192 */
#define LETTER_GENERAL_PUNCTUATION_END				0x206F /* 8303 */

//Currency symbols
#define LETTER_GENERAL_CURRENCY_START				0x20A0 /* 8352 */
#define LETTER_GENERAL_CURRENCY_END					0x20CF /* 8399 */ /* Values available only till 20B9, others are reserved */

//Korean won symbol - received from AxT9IME
//Might need to add other korean symbols from 0xFF00 to 0xFFEE
#define LETTER_GENERAL_CURRENCY_KOREANWON			0xFFE6 /* 65510 */ 

//Letterlike symbols
#define LETTER_GENERAL_LETTERLIKE_START				0x2100 /* 8448 */
#define LETTER_GENERAL_LETTERLIKE_END				0x214F /* 8527 */ 

//Other symbols used below
#define LETTER_ZERO_WIDTH_NON_JOINER				0x200C /* 8204 */
#define LETTER_LEFT_TO_RIGHT_OVERRIDE				0x202D /* 8237 */

//English numbers and symbols
#define LETTER_ENGLISH_NUMBERS_START				0x0030 /* 48 */
#define LETTER_ENGLISH_NUMBERS_END					0x0039 /* 57 */
#define LETTER_ENGLISH_SYMBOL_MINUS					0x002D /* 45 */


class BidiCheck {
private:
	int isSymbols(u16 testChar);
	int isGeneralSymbols(u16 testChar);
	int isArabicSymbols(u16 testChar);
	int isArabicNumber(u16 testChar);
	int isNumber(u16 testChar);
	int isArabicGlyphChar(u16 testChar);
	int isArabicChar(u16 testChar);


public:
	int applyRTLForString(u16 *inStr, int length);

};


#endif //BidiCheck_h
