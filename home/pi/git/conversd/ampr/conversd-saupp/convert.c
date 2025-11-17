/* convert() - converts machine dependend character standards */

/* This is convert version 0.10PL0 - 930221  Written by Thomas Osterried, DL9SAU.
 * Files: iso.c, iso.h and iso.doc.
 *
 *      Copyright 1993 by Thomas Osterried, DL9SAU.
 *      Permission granted for non-commercial use and copying, provided
 *      that this notice is retained.
 *      2001: GPL (changed by dl9sau)
 *      (c) 1993-2001 dl9sau GPL
 *
 * Important notice:
 * -----------------
 *   - this convert tool can easily be implemented in other software
 *     with only including iso.h and compiling together with iso.c
 *   - before convert() is usable, charset_init() has to be called
 *   - convert() does not return dynamic buffers; for that, MAXBUF
 *     is defined (below) which must not be less than the input string
 *   - convert() cannot verify if it exceeds the input buffer dimension.
 *     For that it's strongly recommended, that the input buffer is just
 *     filled up to half (to stand the worst case).
 *   - If you'd like to establish new standards, don't forget to increase
 *     the CHARSETS definition in iso.h
 *
 * About compiling a standalone convert program:
 *   cc -DSTANDALONE -O -s -o convert iso.c
 *   ln -s convert TEXtoISO; ..
 *
 * Enjoy it!    - Thommy
 *   DL9SAU @ DB0SAO.DEU.EU
 *   <thomas@x-berg.in-berlin.de>
 */


#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "convert.h"
#include "hmalloc.h"

static struct charsets charsets[] =
{
	{"iso", ISO_STRIPED},
	{"iso-raw", ISO},
	{"iso-8859-1", ISO_STRIPED},
	{"iso-8859-15", ISO_STRIPED},
	{"ansi", ISO_STRIPED},
	{"8bit", ISO},
	{"latin1", ISO_STRIPED},
	{"ibmpc", IBMPC},
	{"winlatin1", CP1252},
	{"cp1252", CP1252},
	{"cp850", IBMPC},
	{"cp437", IBMPC},
	{"html", HTML},
	{"dumb", dumb},
	{"ascii", dumb},
	{"none", dumb},
	{"us", dumb},
	{"tex", TeX},
	{"ibm7bit", IBM7},
	{"7bit", IBM7},
	{"commodore", IBM7},
	{"c64", IBM7},
	{"digicom", IBM7},
	{"roman8", ROMAN8},
	{"pc", IBMPC},
	{"at", IBMPC},
	{"xt", IBMPC},
	{"atari", ATARI},
	{"binary", BIN},
	{"image", BIN},
	{"macintosh", MAC},		/* by HB9JNX */
	{"mac", MAC},			/* by HB9JNX */
	{"postscript", POSTSCRIPT},	/* by HB9JNX */
	{"ps", POSTSCRIPT},		/* by HB9JNX */
	{(char *) 0, -1}
};

static void init_iso(void);
static void init_iso_striped(void);
static void init_winlatin1(void);
static void init_dumb(void);
static void init_tex(void);
static void init_ibm7(void);
static void init_roman8(void);
static void init_ibmpc(void);
static void init_atari(void);
static void init_html(void);
static void init_mac(void);
static void init_postscript(void);

#define MAXBUF  2048

#define  uchar(x)  ((x) & 0xff)
#define isupperalphauchar(x)  (isalpha(uchar(x)) && isupper(uchar(x)))

#define  CP(x, y) {          \
  sprintf(buf, "%c", y);     \
  cpp[x] = hstrdup(buf);      \
}

static char *charset[CHARSETS][CHARS];

static void init_iso()
{
	char **cpp;
	char buf[2];
	int i;

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	cpp = charset[ISO];

	for (i = 0; i < 96; i++) {
		sprintf(buf, "%c", 160 + i);
		cpp[i] = hstrdup(buf);
	}
}

/*---------------------------------------------------------------------------*/

static void init_iso_striped()
{
	char **cpp;
	char buf[2];
	int i;

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	cpp = charset[ISO_STRIPED];

	for (i = 0; i < 96; i++) {
		sprintf(buf, "%c", 160 + i);
		cpp[i] = hstrdup(buf);
	}
}

/*---------------------------------------------------------------------------*/

static void init_winlatin1()
{
	char **cpp;
	char buf[2];
	int i;

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	cpp = charset[CP1252];

	for (i = 0; i < 96; i++) {
		sprintf(buf, "%c", 160 + i);
		cpp[i] = hstrdup(buf);
	}
	cpp[4] = "\200";
}

/*---------------------------------------------------------------------------*/

static void init_dumb()
{
	char **cpp;

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	cpp = charset[dumb];

	cpp[4] = "Euro";
	cpp[36] = "Ae";
	cpp[54] = "Oe";
	cpp[60] = "Ue";
	cpp[63] = "ss";
	cpp[68] = "ae";
	cpp[86] = "oe";
	cpp[92] = "ue";
}

/*---------------------------------------------------------------------------*/

static void init_html(void)
{
	char **cpp;

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	cpp = charset[HTML];

	cpp[4] = "&euro;";
	cpp[36] = "&Auml;";
	cpp[54] = "&Ouml;";
	cpp[60] = "&Uuml;";
	cpp[63] = "&szlig;";
	cpp[68] = "&auml;";
	cpp[86] = "&ouml;";
	cpp[92] = "&uuml;";
}

/*---------------------------------------------------------------------------*/

static void init_tex()
{
	char **cpp;

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	cpp = charset[TeX];

	cpp[4] = "\\euro";
	cpp[36] = "\"A";
	cpp[54] = "\"O";
	cpp[60] = "\"U";
	cpp[63] = "\"s";
	cpp[68] = "\"a";
	cpp[86] = "\"o";
	cpp[92] = "\"u";
}

/*---------------------------------------------------------------------------*/

static void init_ibm7()
{
	char **cpp;

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	cpp = charset[IBM7];

	cpp[4] = "Euro";
	cpp[36] = "[";
	cpp[54] = "\\";
	cpp[60] = "]";
	cpp[63] = "~";
	cpp[68] = "{";
	cpp[86] = "|";
	cpp[92] = "}";
}

/*---------------------------------------------------------------------------*/

static void init_roman8()
{
	char **cpp;
	char buf[2];

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	cpp = charset[ROMAN8];

	CP(0, 255);
	CP(1, 184);
	CP(2, 191);
	CP(3, 187);
	CP(4, 186);
	CP(5, 188);
	CP(7, 189);
	CP(8, 171);
	CP(10, 249);
	CP(11, 251);
	CP(13, 246);
	CP(15, 176);
	CP(16, 179);
	CP(17, 254);
	CP(20, 168);
	CP(26, 250);
	CP(27, 253);
	CP(28, 247);
	CP(29, 248);
	CP(31, 185);
	CP(32, 161);
	CP(33, 224);
	CP(34, 162);
	CP(35, 225);
	CP(36, 216);
	CP(37, 208);
	CP(38, 211);
	CP(39, 180);
	CP(40, 163);
	CP(41, 220);
	CP(42, 164);
	CP(43, 165);
	CP(44, 230);
	CP(45, 229);
	CP(46, 166);
	CP(47, 167);
	CP(48, 227);
	CP(49, 182);
	CP(50, 232);
	CP(51, 231);
	CP(52, 223);
	CP(53, 233);
	CP(54, 218);
	CP(56, 210);
	CP(57, 173);
	CP(58, 237);
	CP(59, 174);
	CP(60, 219);
	CP(62, 240);
	CP(63, 222);
	CP(64, 200);
	CP(65, 196);
	CP(66, 192);
	CP(67, 226);
	CP(68, 204);
	CP(69, 212);
	CP(70, 215);
	CP(71, 181);
	CP(72, 201);
	CP(73, 197);
	CP(74, 193);
	CP(75, 205);
	CP(76, 217);
	CP(77, 213);
	CP(78, 209);
	CP(79, 221);
	CP(80, 228);
	CP(81, 183);
	CP(82, 202);
	CP(83, 198);
	CP(84, 194);
	CP(85, 234);
	CP(86, 206);
	CP(88, 214);
	CP(89, 203);
	CP(90, 199);
	CP(91, 195);
	CP(92, 207);
	CP(94, 241);
	CP(95, 239);

	CP(96, 252);
}

/*---------------------------------------------------------------------------*/

static void init_ibmpc()
{
	char **cpp;
	char buf[2];

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	cpp = charset[IBMPC];

	CP(0, 255);
	CP(1, 173);
	CP(2, 155);
	CP(3, 156);
	CP(4, 213);
	CP(5, 157);
	CP(10, 166);
	CP(11, 174);
	CP(12, 170);
	CP(13, 196);
	CP(16, 248);
	CP(17, 241);
	CP(18, 253);
	CP(21, 230);
	CP(23, 249);
	CP(26, 167);
	CP(27, 175);
	CP(28, 172);
	CP(29, 171);
	CP(31, 168);
	CP(36, 142);
	CP(37, 143);
	CP(38, 146);
	CP(39, 128);
	CP(40, 144);
	CP(49, 165);
	CP(54, 153);
	CP(60, 154);
	CP(63, 225);
	CP(64, 133);
	CP(65, 160);
	CP(66, 131);
	CP(68, 132);
	CP(69, 134);
	CP(70, 145);
	CP(71, 135);
	CP(72, 138);
	CP(73, 130);
	CP(74, 136);
	CP(75, 137);
	CP(76, 141);
	CP(77, 161);
	CP(78, 140);
	CP(79, 139);
	CP(80, 229);
	CP(81, 164);
	CP(82, 149);
	CP(83, 162);
	CP(84, 147);
	CP(86, 148);
	CP(87, 246);
	CP(88, 237);
	CP(89, 151);
	CP(90, 163);
	CP(91, 150);
	CP(92, 129);
	CP(95, 152);

	CP(96, 254);
}

/*---------------------------------------------------------------------------*/

static void init_atari()
{
	int i;
	char buf[2];

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	/* Since we need the data from init_ibmpc, that function checks
	 * if it has been called before
	 */

	init_ibmpc();

	/* IBM and ATARI are mainly identical - nevertheless, there's one exception */
	for (i = 0; i < CHARS; i++)
		charset[ATARI][i] = charset[IBMPC][i];

	sprintf(buf, "%c", 0x9e);
	charset[ATARI][63] = hstrdup(buf);
}

/*---------------------------------------------------------------------------*/

/* This charset converts the characters from 0240 to 0377 to the 
 * character set used in Apple's Macintosh class computers
 * (or those computers that run MacOS)
 * This was derived from the Table T-40 (p.248) of "Inside Macintosh
 * Volume I", which is quite old and may have been extended since...
 * Added by Tom Sailer, HB9JNX, 18.6.95
 */

#define CPM(x,y) CP(x-0240,y)

static void
init_mac ()
{
	char **cpp;
	char buf[2];

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	cpp = charset[MAC];

	CPM (0304, 0x80);
	CPM (0305, 0x81);
	CPM (0307, 0x82);
	CPM (0311, 0x83);
	CPM (0321, 0x84);
	CPM (0326, 0x85);
	CPM (0334, 0x86);
	CPM (0341, 0x87);
	CPM (0340, 0x88);
	CPM (0342, 0x89);
	CPM (0344, 0x8a);
	CPM (0343, 0x8b);
	CPM (0345, 0x8c);
	CPM (0347, 0x8d);
	CPM (0351, 0x8e);
	CPM (0350, 0x8f);

	CPM (0352, 0x90);
	CPM (0353, 0x91);
	CPM (0355, 0x92);
	CPM (0354, 0x93);
	CPM (0356, 0x94);
	CPM (0357, 0x95);
	CPM (0361, 0x96);
	CPM (0363, 0x97);
	CPM (0362, 0x98);
	CPM (0364, 0x99);
	CPM (0366, 0x9a);
	CPM (0365, 0x9b);
	CPM (0372, 0x9c);
	CPM (0371, 0x9d);
	CPM (0373, 0x9e);
	CPM (0374, 0x9f);


	CPM (0260, 0xa1);
	CPM (0242, 0xa2);
	CPM (0243, 0xa3);
	CPM (0247, 0xa4);
	CPM (0266, 0xa6);
	CPM (0337, 0xa7);
	CPM (0256, 0xa8);
	CPM (0251, 0xa9);
	CPM (0264, 0xab);
	CPM (0250, 0xac);
	CPM (0306, 0xae);
	CPM (0330, 0xaf);

	CPM (0261, 0xb1);
	CPM (0245, 0xb4);
	CPM (0265, 0xb5);
	CPM (0252, 0xbb);
	CPM (0272, 0xbc);
	CPM (0346, 0xbe);
	CPM (0370, 0xbf);

	CPM (0277, 0xc0);
	CPM (0241, 0xc1);
	CPM (0254, 0xc2);
	CPM (0253, 0xc7);
	CPM (0273, 0xc8);
	CPM (0300, 0xcb);
	CPM (0303, 0xcc);
	CPM (0325, 0xcd);

	CPM (0252, 0xd2);
	CPM (0272, 0xd3);
	CPM (0367, 0xd6);
	CPM (0377, 0xd8);

}

/*---------------------------------------------------------------------------*/

/* This charset converts the characters from 0240 to 0377 to the names used
 * in Postscript(R) Fonts
 * derived from the table in Appendix E (p.596ff) of the "Postscript language
 * reference manual, 2nd edition" by Adobe Systems Incorporated
 * Added by Tom Sailer, HB9JNX, 18.6.95
 */

static void
init_postscript ()
{
	char **cpp;

	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	cpp = charset[POSTSCRIPT];

	cpp[0306 - 0240] = "AE";
	cpp[0301 - 0240] = "Aacute";
	cpp[0302 - 0240] = "Acircumflex";
	cpp[0304 - 0240] = "Adieresis";
	cpp[0300 - 0240] = "Agrave";
	cpp[0305 - 0240] = "Aring";
	cpp[0303 - 0240] = "Atilde";
	cpp[0307 - 0240] = "Ccedilla";
	cpp[0311 - 0240] = "Eacute";
	cpp[0312 - 0240] = "Ecircumflex";
	cpp[0313 - 0240] = "Edieresis";
	cpp[0310 - 0240] = "Egrave";
	cpp[0320 - 0240] = "Eth";
	cpp[0315 - 0240] = "Iacute";
	cpp[0316 - 0240] = "Icircumflex";
	cpp[0317 - 0240] = "Idieresis";
	cpp[0314 - 0240] = "Igrave";
	cpp[0321 - 0240] = "Ntilde";
	cpp[0323 - 0240] = "Oacute";
	cpp[0324 - 0240] = "Ocircumflex";
	cpp[0326 - 0240] = "Odieresis";
	cpp[0322 - 0240] = "Ograve";
	cpp[0330 - 0240] = "Oslash";
	cpp[0325 - 0240] = "Otilde";
	cpp[0336 - 0240] = "Thorn";
	cpp[0332 - 0240] = "Uacute";
	cpp[0333 - 0240] = "Ucircumflex";
	cpp[0334 - 0240] = "Udieresis";
	cpp[0331 - 0240] = "Ugrave";
	cpp[0335 - 0240] = "Yacute";
	cpp[0341 - 0240] = "aacute";
	cpp[0342 - 0240] = "acircumflex";
	cpp[0264 - 0240] = "acute";
	cpp[0344 - 0240] = "adieresis";
	cpp[0340 - 0240] = "agrave";
	cpp[0345 - 0240] = "aring";
	cpp[0246 - 0240] = "brokenbar";
	cpp[0347 - 0240] = "ccedilla";
	cpp[0270 - 0240] = "cedilla";
	cpp[0242 - 0240] = "cent";
	cpp[0251 - 0240] = "copyright";
	cpp[0244 - 0240] = "currency";
	cpp[0260 - 0240] = "degree";
	cpp[0250 - 0240] = "dieresis";
	cpp[0367 - 0240] = "divide";
	cpp[0351 - 0240] = "eacute";
	cpp[0352 - 0240] = "ecircumflex";
	cpp[0353 - 0240] = "edieresis";
	cpp[0350 - 0240] = "egrave";
	cpp[0360 - 0240] = "eth";
	cpp[0241 - 0240] = "exclamdown";
	cpp[0337 - 0240] = "germandbls";
	cpp[0253 - 0240] = "guillemotleft";
	cpp[0273 - 0240] = "guillemotright";
	cpp[0255 - 0240] = "hyphen";
	cpp[0355 - 0240] = "iacute";
	cpp[0356 - 0240] = "icircumflex";
	cpp[0357 - 0240] = "idieresis";
	cpp[0354 - 0240] = "igrave";
	cpp[0254 - 0240] = "logicalnot";
	cpp[0257 - 0240] = "macron";
	cpp[0265 - 0240] = "mu";
	cpp[0327 - 0240] = "multiply";
	cpp[0361 - 0240] = "ntilde";
	cpp[0363 - 0240] = "oacute";
	cpp[0364 - 0240] = "ocircumflex";
	cpp[0366 - 0240] = "odieresis";
	cpp[0362 - 0240] = "ograve";
	cpp[0275 - 0240] = "onehalf";
	cpp[0274 - 0240] = "onequarter";
	cpp[0271 - 0240] = "onesuperior";
	cpp[0252 - 0240] = "ordfeminine";
	cpp[0272 - 0240] = "ordmasculine";
	cpp[0370 - 0240] = "oslash";
	cpp[0365 - 0240] = "otilde";
	cpp[0266 - 0240] = "paragraph";
	cpp[0267 - 0240] = "periodcentered";
	cpp[0261 - 0240] = "plusminus";
	cpp[0277 - 0240] = "questiondown";
	cpp[0256 - 0240] = "registered";
	cpp[0247 - 0240] = "section";
	cpp[0243 - 0240] = "sterling";
	cpp[0376 - 0240] = "thorn";
	cpp[0276 - 0240] = "threequarters";
	cpp[0263 - 0240] = "threesuperior";
	cpp[0262 - 0240] = "twosuperior";
	cpp[0372 - 0240] = "uacute";
	cpp[0373 - 0240] = "ucircumflex";
	cpp[0374 - 0240] = "udieresis";
	cpp[0371 - 0240] = "ugrave";
	cpp[0375 - 0240] = "yaccute";
	cpp[0377 - 0240] = "ydieresis";
	cpp[0245 - 0240] = "yen";
}

/*---------------------------------------------------------------------------*/

void charset_init()
{
	static int twice = 0;

	if (twice)
		return;
	twice = 1;

	init_iso();
	init_iso_striped();
	init_winlatin1();
	init_dumb();
	init_tex();
	init_ibm7();
	init_roman8();
	init_ibmpc();
	init_atari();
	init_html();
	init_mac();
	init_postscript();
}

/*---------------------------------------------------------------------------*/

int get_charset_by_name(buf)
char *buf;
{

	int len;
	struct charsets *p_charset;

	if (!(len = strlen(buf)))
		return ISO;

	for (p_charset = charsets; p_charset->name; p_charset++)
		if (!strncmp(p_charset->name, buf, len))
			return p_charset->ind;

	return -1;

}

/*---------------------------------------------------------------------------*/

char *get_charset_by_ind(ind)
int ind;
{

	struct charsets *p_charset;

	for (p_charset = charsets; p_charset->name; p_charset++)
		if (p_charset->ind == ind)
			return p_charset->name;

	return (char *) 0;

}

/*---------------------------------------------------------------------------*/

char *list_charsets()
{

	static char buf[2048];
	char tmp[2048];
	int i;
	struct charsets *p_charset;

	static char *p = (char *) 0;

	if (p)
		return p;

	*buf = '\0';
	for (i = 0; i < CHARSETS; i++) {
		*tmp = '\0';
		for (p_charset = charsets; p_charset->name; p_charset++) {
			if (i != p_charset->ind)
				continue;
			if (*tmp)
				strcat(tmp, ", ");
			strcat(tmp, p_charset->name);
		}
		strcat(tmp, "\n");
		strcat(buf, tmp);
	}

	/* No good solution - but BIN is not part of CHARSERTS */
	*tmp = '\0';
	for (p_charset = charsets; p_charset->name; p_charset++) {
		if (p_charset->ind != BIN)
			continue;
		if (*tmp)
			strcat(tmp, ", ");
		strcat(tmp, p_charset->name);
	}
	strcat(tmp, "\n");
	strcat(buf, tmp);

	return (p = hstrdup(buf));

}

/*---------------------------------------------------------------------------*/

char *convert(in, out, string)
int in;
int out;
char *string;
{

	char buf[MAXBUF];
	register char *p, *q, *curr_set = 0;

	int pos;

	if (in == BIN || in == dumb || in == out)
		return string;
	if (in < 0 || out < 0 || in > CHARSETS - 1 || out > CHARSETS - 1)
		return string;

	for (p = string, q = buf; *p && (q - buf < sizeof(buf)-1) ; p++) {

		/* win$ user's (cp850) can't read. that's why they don't use /charset.
		 * because latin1 does not assign 0x80..0x09f, we could try to
		 * auto-detect this most-happening case.
		 * cave: this may clash with some characters of winlatin1
		 */
		if (in == ISO && (*p & 0xff) > 0x80 && (*p & 0xff) < 0xa0)
			in = IBMPC;

		/* Escaped parentheses? */
		if (in == TeX && !strncmp(p, "\\\"", 2)) {
			p++;
			continue;
		}
		if (!(in == TeX && *p != '\"')) {
			/* Look for correspondings */
			for (pos = 0; pos < CHARS; pos++) {
				curr_set = charset[in][pos];
				if (curr_set && !strncmp(p, curr_set, strlen(curr_set))) {
					curr_set = charset[out][pos];
					break;
				}
			}
			/* Something found? */
			if (pos < CHARS && curr_set) {
				if (in == TeX) {
					p++;
					if (!*p)
						goto out;
				}
				if (in == POSTSCRIPT || in == HTML) {
					int len = strlen(charset[in][pos]);
					if (--len > 0)
					  p += len;	/* by HB9JNX */
				}	
				while (*curr_set) {
					*q++ = *curr_set++;
					// avoid buffer overflows
					if (q - buf > sizeof(buf) - 2)
						goto out;
				}
				if (out == dumb &&
				    (isupperalphauchar(p[1]) ||
				     ((q - 2 == buf ||
				       (q - 2 > buf && isupperalphauchar(*(q - 2)) && isupperalphauchar(*(q - 3)))) &&
				      !isalnum(uchar(p[1])))))
					*(q - 1) = toupper(uchar(*(curr_set - 1)));
				continue;
			}
		}
		*q++ = *p;

	}

out:
	*q = '\0';

	// copy back
	for (p = buf, q = string; *p; p++) {
		/* unix latin1 has troubles with characters like 141, 154, ..
		 * as well as with PC drawing characters like ^N
		 */
		if (out == ISO_STRIPED && (((*p & 0xff) < ' ' && (*p & 0xff) != 0x07 && (*p & 0xff) >= 2) || ((*p & 0xff) > 0x80 && (*p & 0xff) < 0xa0))) {
			switch (*p & 0xff) {
			case '\r':
			case '\n':
			case ' ':
				*q++ = *p;
				break;
			case '\t':
				*q++ = ' ';
				break;
			default:
				*q++ = '.';
				break;
			}
			continue;
		}
		*q++ = *p;
	}
	*q = '\0';

	return string;
}

/*---------------------------------------------------------------------------*/

#ifdef	STANDALONE

static void pexit(char *mesg);
static void doconvert(FILE * fp);

#define USAGE "usage: convert in out [filename]  or  ${in}to${out} [filename]"

static char *myname;
static int in, out;

/*---------------------------------------------------------------------------*/

static void pexit(mesg)
char *mesg;
{
	if (mesg)
		perror(mesg);
	else
		fprintf(stderr, "%s: invalid charset\n%s\n\nin/out charsets are:\n%s\n", myname, USAGE, list_charsets());
	exit(1);
}

/*---------------------------------------------------------------------------*/

static void doconvert(FILE * fp)
{
	char buf[MAXBUF / 2];

	while (fgets(buf, sizeof(buf), fp))
		fputs(convert(in, out, buf), stdout);

	if (ferror(fp))
		pexit("read");
}


/*---------------------------------------------------------------------------*/

main(int argc, char *argv[])
{

	FILE *fp;

	int ind;
	char *p;

	ind = 1;
	in = out = -1;

	if ((p = strrchr(argv[0], '/')
#ifdef	__TURBOC__
	     || p = strrchr(argv[0], '\\')
#endif
	    ) && *++p)
		myname = hstrdup(p);
	else
		myname = hstrdup(argv[0]);

#ifdef	__TURBOC__
	myname = strlwr(myname);
#else
	for (p = myname; *p; p++)
		*p = tolower(uchar(*p));
#endif

	if ((p = strstr(myname, "to")) && p[1]) {
		*p = '\0';
		p = &p[2];
		out = get_charset_by_name(p);
		in = get_charset_by_name(myname);
	}
	if (in < 0 || out < 0 && argc >= 3) {
		in = get_charset_by_name(argv[1]);
		out = get_charset_by_name(argv[2]);
		ind = 3;
	}
	if (in < 0 || out < 0)
		pexit(0);

	charset_init();

	if (ind < argc) {

		while (ind < argc) {
			p = argv[ind++];
			if (!(fp = fopen(p, "r")))
				pexit(p);
			doconvert(fp);
			fclose(p);
		}

	} else
		doconvert(stdin);

	exit(0);

	return 0;
}
#endif
