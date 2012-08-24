/* Copyright (C) 2004 J.F.Dockes
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef _PROPLIST_H_INCLUDED_
#define _PROPLIST_H_INCLUDED_


/** 
 * A subset of Unicode chars that we consider word breaks when we
 * split text in words. 
 *
 * This is used as a quick fix to the ascii-based code, and is not correct.
 * the correct way would be to do what http://www.unicode.org/reports/tr29/ 
 * says. 
 * 
 * Data from:
# PropList-4.0.1.txt
# Date: 2004-03-02, 02:42:40 GMT [MD]
#
# Unicode Character Database
# Copyright (c) 1991-2004 Unicode, Inc.
# For terms of use, see http://www.unicode.org/terms_of_use.html
# For documentation, see UCD.html
*/
static const unsigned int uniign[] = {
    0x0021, /*  ; Terminal_Punctuation # Po       EXCLAMATION MARK*/
    0x002C, /*  ; Terminal_Punctuation # Po       COMMA*/
    0x002D, /*  ; Dash # Pd       HYPHEN-MINUS*/
    0x002E, /*  ; Terminal_Punctuation # Po       FULL STOP*/
    0x003A, /*  ; Terminal_Punctuation # Po   [2] COLON..SEMICOLON*/
    0x003B, /*  ; Terminal_Punctuation # Po   [2] COLON..SEMICOLON*/
    0x003F, /*  ; Terminal_Punctuation # Po       QUESTION MARK*/
    0x0085, /* NEXT LINE NEL;Cc */
    0x00A0, /* NO-BREAK SPACE; Zs */
    0x00A1, /* INVERTED EXCLAMATION MARK;Po */
    0x00A2, /* CENT SIGN;Sc */
    0x00A3, /* POUND SIGN;Sc; */
    0x00A4, /* CURRENCY SIGN;Sc; */
    0x00A5, /* YEN SIGN;Sc; */
    0x00A6, /* BROKEN BAR;So */
    0x00A7, /* SECTION SIGN;So; */
    0x00A9, /* COPYRIGHT SIGN;So */
    0x00AB, /*  ; Quotation_Mark # Pi       LEFT-POINTING DOUBLE ANGLE QUOTATION MARK*/
    0x00AC, /* NOT SIGN;Sm */
    0x00AD, /*  ; Hyphen # Cf       SOFT HYPHEN*/
    0x00AE, /* registered sign */
    0x00B0, /* DEGREE SIGN;So;0;ET;;;;;N;;;;; */
    0x00B1, /* PLUS-MINUS SIGN;Sm;0;ET;;;;;N;PLUS-OR-MINUS SIGN;;;;*/
    0x00B7, /* MIDDLE DOT;Po;0;ON;;;;;N;;;;;*/
    0x00BB, /*  ; Quotation_Mark # Pf       RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK*/
    0x00BF, /*                   INVERTED QUESTION MARK;Po */
    0x00D7, /* MULTIPLICATION SIGN;Sm;0;ON;;;;;N;;;;; */
    0x037E, /*  ; Terminal_Punctuation # Po       GREEK QUESTION MARK*/
    0x0387, /*  ; Terminal_Punctuation # Po       GREEK ANO TELEIA*/
    0x055C, /*  ; STerm # Po       ARMENIAN EXCLAMATION MARK*/
    0x055E, /*  ; STerm # Po       ARMENIAN QUESTION MARK*/
    0x0589, /*  ; STerm # Po       ARMENIAN FULL STOP*/
    0x0589, /*  ; Terminal_Punctuation # Po       ARMENIAN FULL STOP*/
    0x058A, /*  ; Dash # Pd       ARMENIAN HYPHEN*/
    0x058A, /*  ; Hyphen # Pd       ARMENIAN HYPHEN*/
    0x05C3, /*  ; Terminal_Punctuation # Po       HEBREW PUNCTUATION SOF PASUQ*/
    0x060C, /*  ; Terminal_Punctuation # Po       ARABIC COMMA*/
    0x061B, /*  ; Terminal_Punctuation # Po       ARABIC SEMICOLON*/
    0x061F, /*  ; STerm # Po       ARABIC QUESTION MARK*/
    0x061F, /*  ; Terminal_Punctuation # Po       ARABIC QUESTION MARK*/
    0x06D4, /*  ; STerm # Po       ARABIC FULL STOP*/
    0x06D4, /*  ; Terminal_Punctuation # Po       ARABIC FULL STOP*/
    0x166E, /*  ; STerm # Po       CANADIAN SYLLABICS FULL STOP*/
    0x1680, /*  ; White_Space # Zs       OGHAM SPACE MARK*/
    0x16EB, /* RUNIC SINGLE PUNCTUATION;Po;0;L;;;;;N;;;;;*/
    0x16EC, /* RUNIC MULTIPLE PUNCTUATION;Po;0;L;;;;;N;;;;;*/
    0x16ED, /* RUNIC CROSS PUNCTUATION;Po;0;L;;;;;N;;;;; */
    0x1803, /*  ; STerm # Po       MONGOLIAN FULL STOP*/
    0x1806, /*  ; Hyphen # Pd       MONGOLIAN TODO SOFT HYPHEN*/
    0x1809, /*  ; STerm # Po       MONGOLIAN MANCHU FULL STOP*/
    0x180E, /*  ; White_Space # Zs       MONGOLIAN VOWEL SEPARATOR*/
    0x2000, /*  ; White_Space # Zs  [11] EN QUAD..HAIR SPACE*/
    0x2001, /*  ; White_Space # Zs  [11] EN QUAD..HAIR SPACE*/
    0x2002, /*  ; White_Space # Zs  [11] EN QUAD..HAIR SPACE*/
    0x2003, /*  ; White_Space # Zs  [11] EN QUAD..HAIR SPACE*/
    0x2004, /*  ; White_Space # Zs  [11] EN QUAD..HAIR SPACE*/
    0x2005, /*  ; White_Space # Zs  [11] EN QUAD..HAIR SPACE*/
    0x2006, /*  ; White_Space # Zs  [11] EN QUAD..HAIR SPACE*/
    0x2007, /*  ; White_Space # Zs  [11] EN QUAD..HAIR SPACE*/
    0x2008, /*  ; White_Space # Zs  [11] EN QUAD..HAIR SPACE*/
    0x2009, /*  ; White_Space # Zs  [11] EN QUAD..HAIR SPACE*/
    0x200A, /*  ; White_Space # Zs  [11] EN QUAD..HAIR SPACE*/
    0x2010, /*  ; Hyphen # Pd   [2] HYPHEN..NON-BREAKING HYPHEN*/
    0x2011, /*  ; Hyphen # Pd   [2] HYPHEN..NON-BREAKING HYPHEN*/
    0x2012, /*  ; Dash # Pd   [6] HYPHEN..HORIZONTAL BAR*/
    0x2013, /*  ; Dash # Pd   [6] HYPHEN..HORIZONTAL BAR*/
    0x2014, /*  ; Dash # Pd   [6] HYPHEN..HORIZONTAL BAR*/
    0x2015, /*  ; Dash # Pd   [6] HYPHEN..HORIZONTAL BAR*/
    0x2018, /*  ; Quotation_Mark # Pi       LEFT SINGLE QUOTATION MARK*/
    0x2019, /*  ; Quotation_Mark # Pf       RIGHT SINGLE QUOTATION MARK*/
    0x201A, /*  ; Quotation_Mark # Ps       SINGLE LOW-9 QUOTATION MARK*/
    0x201B, /*  ; Quotation_Mark # Pi       SINGLE HIGH-REVERSED-9 QUOTATION MARK*/
    0x201C, /*  ; Quotation_Mark # Pi       LEFT DOUBLE QUOTATION MARK*/
    0x201D, /*  ; Quotation_Mark # Pf       RIGHT DOUBLE QUOTATION MARK*/
    0x201E, /*  ; Quotation_Mark # Ps       DOUBLE LOW-9 QUOTATION MARK*/
    0x201F, /*  ; Quotation_Mark # Pi       DOUBLE HIGH-REVERSED-9 QUOTATION MARK*/
    0x2022, /* BULLET;Po;0;ON;;;;;N;;;;; */
    0x2023, /* TRIANGULAR BULLET;Po;0;ON;;;;;N;;;;;*/
    0x2024, /* ONE DOT LEADER;Po;0;ON;<compat> 002E;;;;N;;;;;*/
    0x2025, /* TWO DOT LEADER;Po;0;ON;<compat> 002E 002E;;;;N;;;;; */
    0x2026, /* HORIZONTAL ELLIPSIS;Po;0;ON;<compat> 002E 002E 002E;;;;N;;;;; */
    0x2028, /*  ; White_Space # Zl       LINE SEPARATOR*/
    0x2029, /*  ; White_Space # Zp       PARAGRAPH SEPARATOR*/
    0x202F, /*  ; White_Space # Zs       NARROW NO-BREAK SPACE*/
    0x2032, /* PRIME;Po;0;ET;;;;;N;;;;;*/
    0x2039, /*  ; Quotation_Mark # Pi       SINGLE LEFT-POINTING ANGLE QUOTATION MARK*/
    0x203A, /*  ; Quotation_Mark # Pf       SINGLE RIGHT-POINTING ANGLE QUOTATION MARK*/
    0x203C, /*  ; STerm # Po   [2] DOUBLE EXCLAMATION MARK..INTERROBANG*/
    0x203D, /*  ; STerm # Po   [2] DOUBLE EXCLAMATION MARK..INTERROBANG*/
    0x2047, /*  ; Terminal_Punctuation # Po   [3] DOUBLE QUESTION MARK..EXCLAMATION QUESTION MARK*/
    0x2048, /*  ; Terminal_Punctuation # Po   [3] DOUBLE QUESTION MARK..EXCLAMATION QUESTION MARK*/
    0x2049, /*  ; Terminal_Punctuation # Po   [3] DOUBLE QUESTION MARK..EXCLAMATION QUESTION MARK*/
    0x2053, /*  ; Dash # Po       SWUNG DASH*/
    0x205F, /*  ; White_Space # Zs       MEDIUM MATHEMATICAL SPACE*/
    0x207B, /*  ; Dash # Sm       SUPERSCRIPT MINUS*/
    0x208B, /*  ; Dash # Sm       SUBSCRIPT MINUS*/
    0x20A0, /* EURO-CURRENCY SIGN */
    0x20A1, /* COLON SIGN */
    0x20A2, /* CRUZEIRO SIGN */
    0x20A3, /* FRENCH FRANC SIGN */
    0x20A4, /* LIRA SIGN */
    0x20A5, /* MILL SIGN */
    0x20A6, /* NAIRA SIGN */
    0x20A7, /* PESETA SIGN */
    0x20A8, /* RUPEE SIGN */
    0x20A9, /* WON SIGN */
    0x20AA, /* NEW SHEQEL SIGN */
    0x20AB, /* DONG SIGN */
    0x20AC, /* EURO SIGN */
    0x20AD, /* KIP SIGN */
    0x20AE, /* TUGRIK SIGN */
    0x20AF, /* DRACHMA SIGN */
    0x20B0, /* GERMAN PENNY SIGN */
    0x20B1, /* PESO SIGN */
    0x20B2, /* GUARANI SIGN */
    0x20B3, /* AUSTRAL SIGN */
    0x20B4, /* HRYVNIA SIGN */
    0x20B5, /* CEDI SIGN */
    0x20B6, /* LIVRE TOURNOIS SIGN */
    0x20B7, /* SPESMILO SIGN */
    0x20B8, /* TENGE SIGN */
    0x20B9, /* INDIAN RUPEE SIGN */
    0x2117, /* SOUND RECORDING COPYRIGHT;So */
    0x2122, /* TRADE MARK SIGN;So; */
    0x2192, /* RIGHTWARDS ARROW;Sm;0;ON;;;;;N;RIGHT ARROW;;;;*/
    0x2212, /*  ; Dash # Sm       MINUS SIGN*/
    0x2E2E, /* REVERSED QUESTION MARK;Po;0;ON;;;;;N;;;;; */
    0x3000, /*  ; White_Space # Zs       IDEOGRAPHIC SPACE*/
    0x3002, /*  ; STerm # Po       IDEOGRAPHIC FULL STOP*/
    0x300C, /*  ; Quotation_Mark # Ps       LEFT CORNER BRACKET*/
    0x300D, /*  ; Quotation_Mark # Pe       RIGHT CORNER BRACKET*/
    0x300E, /*  ; Quotation_Mark # Ps       LEFT WHITE CORNER BRACKET*/
    0x300F, /*  ; Quotation_Mark # Pe       RIGHT WHITE CORNER BRACKET*/
    0x301C, /*  ; Dash # Pd       WAVE DASH*/
    0x301D, /*  ; Quotation_Mark # Ps       REVERSED DOUBLE PRIME QUOTATION MARK*/
    0x301E, /*  ; Quotation_Mark # Pe       LOW DOUBLE PRIME QUOTATION MARK*/
    0x3030, /*  ; Dash # Pd       WAVY DASH*/
    0x30FB, /*  ; Hyphen # Pc       KATAKANA MIDDLE DOT*/
    0xC2B6, /* PILCROW SIGN;So;0;ON;;;;;N;PARAGRAPH SIGN;;;; */
    0xC3B7, /* DIVISION SIGN;Sm;0;ON;;;;;N;;;;; */
    0xFE31, /*  ; Dash # Pd       PRESENTATION FORM FOR VERTICAL EM DASH*/
    0xFE32, /*  ; Dash # Pd       PRESENTATION FORM FOR VERTICAL EN DASH*/
    0xFE41, /*  ; Quotation_Mark # Ps       PRESENTATION FORM FOR VERTICAL LEFT CORNER BRACKET*/
    0xFE42, /*  ; Quotation_Mark # Pe       PRESENTATION FORM FOR VERTICAL RIGHT CORNER BRACKET*/
    0xFE43, /*  ; Quotation_Mark # Ps       PRESENTATION FORM FOR VERTICAL LEFT WHITE CORNER BRACKET*/
    0xFE44, /*  ; Quotation_Mark # Pe       PRESENTATION FORM FOR VERTICAL RIGHT WHITE CORNER BRACKET*/
    0xFE50, /*  ; Terminal_Punctuation # Po   [3] SMALL COMMA..SMALL FULL STOP*/
    0xFE51, /*  ; Terminal_Punctuation # Po   [3] SMALL COMMA..SMALL FULL STOP*/
    0xFE52, /*  ; STerm # Po       SMALL FULL STOP*/
    0xFE52, /*  ; Terminal_Punctuation # Po   [3] SMALL COMMA..SMALL FULL STOP*/
    0xFE54, /*  ; Terminal_Punctuation # Po   [4] SMALL SEMICOLON..SMALL EXCLAMATION MARK*/
    0xFE55, /*  ; Terminal_Punctuation # Po   [4] SMALL SEMICOLON..SMALL EXCLAMATION MARK*/
    0xFE56, /*  ; Terminal_Punctuation # Po   [4] SMALL SEMICOLON..SMALL EXCLAMATION MARK*/
    0xFE57, /*  ; Terminal_Punctuation # Po   [4] SMALL SEMICOLON..SMALL EXCLAMATION MARK*/
    0xFE58, /*  ; Dash # Pd       SMALL EM DASH*/
    0xFE63, /*  ; Hyphen # Pd       SMALL HYPHEN-MINUS*/
    0xFF01, /* FULLWIDTH EXCLAMATION MARK;Po;0;ON;<wide> 0021;;;;N;;;;; */
    0xFF02, /* FULLWIDTH QUOTATION MARK;Po;0;ON;<wide> 0022;;;;N;;;;; */
    0xFF03, /* FULLWIDTH NUMBER SIGN;Po;0;ET;<wide> 0023;;;;N;;;;; */
    0xFF04, /* FULLWIDTH DOLLAR SIGN;Sc;0;ET;<wide> 0024;;;;N;;;;; */
    0xFF05, /* FULLWIDTH PERCENT SIGN;Po;0;ET;<wide> 0025;;;;N;;;;; */
    0xFF06, /* FULLWIDTH AMPERSAND;Po;0;ON;<wide> 0026;;;;N;;;;; */
    0xFF07, /* FULLWIDTH APOSTROPHE;Po;0;ON;<wide> 0027;;;;N;;;;; */
    0xFF08, /* FULLWIDTH LEFT PARENTHESIS;Ps;0;ON;<wide> 0028;;;;Y;FULLWIDTH OPENIN*/
    0xFF09, /* FULLWIDTH RIGHT PARENTHESIS;Pe;0;ON;<wide> 0029;;;;Y;FULLWIDTH CLOS*/
    0xFF0A, /* FULLWIDTH ASTERISK;Po;0;ON;<wide> 002A;;;;N;;;;; */
    0xFF0B, /* FULLWIDTH PLUS SIGN;Sm;0;ES;<wide> 002B;;;;N;;;;; */
    0xFF0C, /* FULLWIDTH COMMA;Po;0;CS;<wide> 002C;;;;N;;;;; */
    0xFF0D, /* FULLWIDTH HYPHEN-MINUS;Pd;0;ES;<wide> 002D;;;;N;;;;; */
    0xFF0E, /* FULLWIDTH FULL STOP;Po;0;CS;<wide> 002E;;;;N;FULLWIDTH PERIOD;;;; */
    0xFF0F, /* FULLWIDTH SOLIDUS;Po;0;CS;<wide> 002F;;;;N;FULLWIDTH SLASH;;;; */
    0xFF1A, /*  ; Terminal_Punctuation # Po   [2] FULLWIDTH COLON..FULLWIDTH SEMICOLON*/
    0xFF1B, /*  ; Terminal_Punctuation # Po   [2] FULLWIDTH COLON..FULLWIDTH SEMICOLON*/
    0xFF1F, /*  ; Terminal_Punctuation # Po       FULLWIDTH QUESTION MARK*/
    0xFF61, /*  ; Terminal_Punctuation # Po       HALFWIDTH IDEOGRAPHIC FULL STOP*/
    0xFF62, /*  ; Quotation_Mark # Ps       HALFWIDTH LEFT CORNER BRACKET*/
    0xFF63, /*  ; Quotation_Mark # Pe       HALFWIDTH RIGHT CORNER BRACKET*/
    0xFF64, /*  ; Terminal_Punctuation # Po       HALFWIDTH IDEOGRAPHIC COMMA*/
    0xFF65, /*  ; Hyphen # Pc       HALFWIDTH KATAKANA MIDDLE DOT*/
};

/* Things that would visibly break a block of text, rendering obvious the need
 * of quotation if a phrase search is wanted */
static const unsigned int avsbwht[] = {
    0x0009, /* CHARACTER TABULATION */
    0x000A, /* LINE FEED */
    0x000D, /* CARRIAGE RETURN */
    0x0020, /* SPACE;Zs;0;WS */
    0x00A0, /* NO-BREAK SPACE;Zs;0;CS */
    0x1680, /* OGHAM SPACE MARK;Zs;0;WS */
    0x180E, /* MONGOLIAN VOWEL SEPARATOR;Zs;0;WS */
    0x2000, /* EN QUAD;Zs;0;WS */
    0x2001, /* EM QUAD;Zs;0;WS */
    0x2002, /* EN SPACE;Zs;0;WS */
    0x2003, /* EM SPACE;Zs;0;WS */
    0x2004, /* THREE-PER-EM SPACE;Zs;0;WS */
    0x2005, /* FOUR-PER-EM SPACE;Zs;0;WS */
    0x2006, /* SIX-PER-EM SPACE;Zs;0;WS */
    0x2007, /* FIGURE SPACE;Zs;0;WS */
    0x2008, /* PUNCTUATION SPACE;Zs;0;WS */
    0x2009, /* THIN SPACE;Zs;0;WS */
    0x200A, /* HAIR SPACE;Zs;0;WS */
    0x202F, /* NARROW NO-BREAK SPACE;Zs;0;CS */
    0x205F, /* MEDIUM MATHEMATICAL SPACE;Zs;0;WS */
    0x3000, /* IDEOGRAPHIC SPACE;Zs;0;WS */
};

#endif // _PROPLIST_H_INCLUDED_
