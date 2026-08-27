#define VERSION "V3.2"
#define DEFAULTFILENAME "buildnumber.h"

#define ERR_MAXVER -4
#define ERR_PARSE -3
#define ERR_FILENAME -2
#define ERR_FOPEN -1
#define ERR_NONE 0

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char USAGE[2][2000] = {{R"(
BuildNumber %s

Automatically increments version and build numbers in a C header file.
If the file does not exist, it will be created.

Usage:
  BuildNumber [path] [file] -pre
  BuildNumber [path] [file] -post

VERSION_FORMAT determines:
  - field formatting
  - maximum build number
  - maximum minor version
  - maximum major version

Parameters:
  path      Optional target directory.
  file      Header file name. Default: %s
  -pre		Pre-build step. Increment version before compilation.
  -post		Post-build step. Prepare version for next build.
  -h		this usage information.

-pre or -post is mandatory

Version Patterns:
VER_MAJOR, VER_MINOR and BUILDNUMBER contain the current numeric version values.
SOFTWARE_DIS  Optional SAP Software DIS as Long, e.g. 09xxxxxxxxxx

VERSION_FORMAT defines how VERSION_STR and BUILDNUMBER_STR are generated.

  The number of digits in a format string defines the minimum number of numeric positions and printed as leading zeros, if necessary.
  The number of digits plus the number of the '_' character defines the maximum number of numeric positions.
  e.g ___0, for example, defines a maximum of 4 digits, but a minimum of 1 digit.
      0000, for example, defines a maximum of 4 digits and a minimum of 4 digits, printed with leading zeros.

Version Rollover:
When BUILDNUMBER exceeds the maximum value defined by the BUILD field in VERSION_FORMAT, VER_MINOR is incremented.
When VER_MINOR exceeds the maximum value defined by the MINOR field in VERSION_FORMAT, VER_MAJOR is incremented.
)"},
									{R"(

Examples:
VER_MAJOR      0
VER_MINOR      1
BUILDNUMBER    1

VERSION_FORMAT "_0.00.000"

Generated VERSION_STR:
  0.01.001

Pattern Limits:
  _0    -> range 0..99
  00    -> range 00..99
  000   -> range 000..999
  _000  -> range 000..9999

)"}};

void ToBcdArrayString(uint64_t value, char* buffer, size_t bufferSize){
	char digits[13];

	sprintf_s(digits, sizeof(digits), "%012" PRIu64, value);

	buffer[0] = '\0';

	for (int i = 0; i < 12; i += 2) {
		char tmp[16];

		sprintf_s(tmp, sizeof(tmp), "0x%c%c", digits[i], digits[i + 1]);
		strcat_s(buffer, bufferSize, tmp);

		if (i < 10) {
			strcat_s(buffer, bufferSize, ", ");
		}
	}
}

void FormatValue(char* buffer, size_t size, unsigned int value,
				 const char* pattern){
	int minDigits = 0;

	for (int i = 0; pattern[i]; i++) {
		if (pattern[i] != '_')
			minDigits++;
	}

	int maxDigits = (int) strlen(pattern);

	char tmp[32];

	sprintf_s(tmp, sizeof(tmp), "%0*u", minDigits, value);

	if ((int) strlen(tmp) > maxDigits) {
		strcpy_s(buffer, size, "#OVERFLOW");
		return;
	}

	strcpy_s(buffer, size, tmp);
}

void BuildVersionString(char* versionString, size_t versionStringSize,
						unsigned int major, unsigned int minor,
						unsigned int build, const char* versionFormat){
	char formatCopy[64];
	strcpy_s(formatCopy, versionFormat);

	char* majorFmt = strtok(formatCopy, ".");
	char* minorFmt = strtok(NULL, ".");
	char* buildFmt = strtok(NULL, ".");

	if (!majorFmt || !minorFmt || !buildFmt) {
		strcpy_s(versionString, versionStringSize, "#INVALID_FORMAT");

		return;
	}

	char majorStr[32];
	char minorStr[32];
	char buildStr[32];

	FormatValue(majorStr, sizeof(majorStr), major, majorFmt);

	FormatValue(minorStr, sizeof(minorStr), minor, minorFmt);

	FormatValue(buildStr, sizeof(buildStr), build, buildFmt);

	sprintf_s(versionString, versionStringSize, "%s.%s.%s", majorStr, minorStr,
			  buildStr);
}

unsigned int GetPatternMaxValuePart(const char* pattern){
	unsigned int maxValue = 0;

	for (size_t i = 0; i < strlen(pattern); i++) {
		maxValue = maxValue * 10 + 9;
	}

	return maxValue;
}

bool GetFormatParts(const char* versionFormat, char* majorFmt,
					size_t majorFmtSize, char* minorFmt, size_t minorFmtSize,
					char* buildFmt, size_t buildFmtSize){
	char tmp[64];

	strcpy_s(tmp, sizeof(tmp), versionFormat);

	char* pMajor = strtok(tmp, ".");
	char* pMinor = strtok(NULL, ".");
	char* pBuild = strtok(NULL, ".");

	if (!pMajor || !pMinor || !pBuild)
		return false;

	strcpy_s(majorFmt, majorFmtSize, pMajor);
	strcpy_s(minorFmt, minorFmtSize, pMinor);
	strcpy_s(buildFmt, buildFmtSize, pBuild);

	return true;
}

void IncrementVersion(unsigned int* major,
					  unsigned int* minor,
					  unsigned int* build,
					  unsigned int maxMajor,
					  unsigned int maxMinor,
					  unsigned int maxBuild){
	if (++(*build) > maxBuild) {
		*build = 0;

		if (*minor >= maxMinor) {
			*minor = 0;
			(*major)++;

			if (*major > maxMajor) {
				printf("Maximum version reached\n");
				exit(ERR_MAXVER);
			}
		} else {
			(*minor)++;
		}
	}
}

int main(int argc, char* argv[]){
	char fileName[FILENAME_MAX] = DEFAULTFILENAME;
	char filePath[FILENAME_MAX] = "";
	typedef enum
	{
		MODE_NONE = 0,
		MODE_PRE,
		MODE_POST
	} BUILD_MODE;

	/* Parameter auswerten */

	bool resetVersion = false;
	BUILD_MODE mode = MODE_NONE;
	BUILD_MODE lastMode = MODE_NONE;

	/* Nicht-Optionsparameter sammeln */
	char* params[2];
	int paramCount = 0;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-pre")) {
			if (mode != MODE_NONE) {
				printf("Only one mode allowed (-pre or -post)\n");
				return ERR_PARSE;
			}

			mode = MODE_PRE;
		} else if (!strcmp(argv[i], "-post")) {
			if (mode != MODE_NONE) {
				printf("Only one mode allowed (-pre or -post)\n");
				return ERR_PARSE;
			}

			mode = MODE_POST;
		} else if (!strcmp(argv[i], "0")) {
			resetVersion = true;
			printf("Reset version number\n");
		} else if (!strcmp(argv[i], "/?") ||
				   !strcmp(argv[i], "/h") ||
				   !strcmp(argv[i], "-?") ||
				   !strcmp(argv[i], "-h")) {
			printf(USAGE[0], VERSION, DEFAULTFILENAME);
			printf("%s", USAGE[1]);
			return ERR_NONE;
		} else {
			if (paramCount >= 2) {
				printf("Illegal number of parameters\n");
				return ERR_PARSE;
			}

			params[paramCount++] = argv[i];
		}
	}

	if (mode == MODE_NONE) {
		printf("Missing mode parameter (-pre or -post)\n");
		return ERR_PARSE;
	}

	printf("Build mode: %s\n",
	   mode == MODE_PRE ? "PRE" : "POST");

	switch (paramCount)
	{
		case 0:
			/* Default: buildnumber.h */
			break;

		case 1:
			/* BuildNumber test.h */
			strcpy_s(fileName, sizeof(fileName), params[0]);
			break;

		case 2:
			/* BuildNumber path test.h */
			strcat_s(filePath, sizeof(filePath), params[0]);

			{
				size_t len = strlen(filePath);

				if (len > 0 &&
					filePath[len - 1] != '\\' &&
					filePath[len - 1] != '/')
				{
					strcat_s(filePath, sizeof(filePath), "\\");
				}
			}

			strcpy_s(fileName, sizeof(fileName), params[1]);
			break;

		default:
			printf("Illegal number of parameters\n");
			return ERR_PARSE;
	}

	strcat_s(filePath, sizeof(filePath), fileName);

	printf("Using file: %s\n", filePath);

	FILE* fBuildNumber;
	char versionFormat[64] = "_0.00.000";
	unsigned int major = 0;
	unsigned int minor = 0;
	unsigned int build = 0;
	uint64_t sap_dis = 0;
	bool use_sap_dis = false;

	char majorFmt[32];
	char minorFmt[32];
	char buildFmt[32];

	fBuildNumber = fopen(filePath, "r");

	if (fBuildNumber != NULL) {
		char fileLine[256];

		while (fgets(fileLine, sizeof(fileLine), fBuildNumber)) {
			char* token = strtok(fileLine, " \t\r\n");

			if (token == NULL)
				continue;

			if (!strcmp(token, "#define")) {
				token = strtok(NULL, " \t\r\n");

				if (token == NULL)
					continue;

				if (!strcmp(token, "VER_MAJOR")) {
					token = strtok(NULL, " \t\r\n");

					if (token) {
						major = (unsigned int) atoi(token);
					}
				} else if (!strcmp(token, "VER_MINOR")) {
					token = strtok(NULL, " \t\r\n");

					if (token) {
						minor = (unsigned int) atoi(token);
					}
				} else if (!strcmp(token, "BUILDNUMBER")) {
					token = strtok(NULL, " \t\r\n");

					if (token) {
						build = (unsigned int) atoi(token);
					}
				} else if (!strcmp(token, "VERSION_FORMAT")) {
					token = strtok(NULL, "\"");

					if (token) {
						strcpy_s(versionFormat, sizeof(versionFormat), token);
					}
				} else if (!strcmp(token, "BUILD_MODE_PRE")) {
					lastMode = MODE_PRE;
				} else if (!strcmp(token, "BUILD_MODE_POST")) {
					lastMode = MODE_POST;
				} else if (!strcmp(token, "SOFTWARE_DIS")) {
					token = strtok(NULL, " \t\r\n");

					if (token) {
						char* endptr;

						sap_dis = (uint64_t) strtoull(token, &endptr, 0);

						if (endptr == token) {
							printf("Invalid SOFTWARE_DIS\n");
						} else {
							use_sap_dis = true;
						}
					}
				}
			}
		}
		fclose(fBuildNumber);
	} else {
		printf("File %s not found. Try to create a new file.\n", filePath);
		resetVersion = true;
	}

	char versionString[64];

	if (!GetFormatParts(versionFormat, majorFmt, sizeof(majorFmt), minorFmt,
		sizeof(minorFmt), buildFmt, sizeof(buildFmt))) {
		printf("Invalid VERSION_FORMAT\n");
		return ERR_PARSE;
	}

	unsigned int maxMajor = GetPatternMaxValuePart(majorFmt);
	unsigned int maxMinor = GetPatternMaxValuePart(minorFmt);
	unsigned int maxBuild = GetPatternMaxValuePart(buildFmt);

	BuildVersionString(versionString, sizeof(versionString), major, minor, build,
					   versionFormat);

	printf("Old build version: %s => ", versionString);

	unsigned int oldMajor = major;
	unsigned int oldMinor = minor;
	unsigned int oldBuild = build;
	char actualVersionString[64];

	BuildVersionString(actualVersionString,
					   sizeof(actualVersionString),
					   oldMajor,
					   oldMinor,
					   oldBuild,
					   versionFormat);

	printf("%s-build increment. Actual firmware version: %s\n",
		mode == MODE_PRE ? "PRE" : "POST",
		actualVersionString);

	/*
	 * BUILD_MODE Bedeutung:
	 *
	 * PRE  -> Version wird im Pre-Build erhöht
	 * POST -> Version wird im Post-Build erhöht
	 *
	 * Übergänge:
	 * PRE  -> PRE   : erhöhen
	 * PRE  -> POST  : erhöhen Kompilierung wurde bereits mit BUILDNUMBER gebaut. Nächste freie Buildnummer reservieren.
	 * POST -> PRE   : nicht erhöhen  Letzte Buildnummer wurde bereits im Post-Build reserviert. Nicht nochmals erhöhen.
	 * POST -> POST  : erhöhen
	 * NONE -> PRE   : erhöhen
	 * NONE -> POST  : erhöhen
	 */

	bool incrementVersion = false;

	switch (lastMode)
	{
		case MODE_PRE:
			incrementVersion = (mode == MODE_PRE) || (mode == MODE_POST);
			break;
		case MODE_POST:
			incrementVersion = (mode == MODE_POST);
			break;
		case MODE_NONE:
			incrementVersion = false;
			break;
		default:
			incrementVersion = false;
			break;
	}

	unsigned int nextMajor = major;
	unsigned int nextMinor = minor;
	unsigned int nextBuild = build;

	if (resetVersion)
	{
		nextMajor = 0;
		nextMinor = 0;
		nextBuild = 1;
	} else if (incrementVersion)
	{
		IncrementVersion(&nextMajor,
						 &nextMinor,
						 &nextBuild,
						 maxMajor,
						 maxMinor,
						 maxBuild);
	}

	/* Ab hier wird ausschließlich die neue Version verwendet */

	major = nextMajor;
	minor = nextMinor;
	build = nextBuild;

	fBuildNumber = fopen(filePath, "w");

	if (fBuildNumber == NULL) {
		printf("Could not create file %s\n", filePath);
		return ERR_FOPEN;
	}

	time_t t = time(NULL);
	struct tm tm;
	tm = *localtime(&t);

	char buildString[32];
	char minorString[32];
	char majorString[32];
	char sapDisStr[32];
	char sapDisBcdArray[128];

	BuildVersionString(versionString,
				   sizeof(versionString),
				   major,
				   minor,
				   build,
				   versionFormat);

	FormatValue(buildString, sizeof(buildString), build, buildFmt);
	FormatValue(minorString, sizeof(minorString), minor, minorFmt);
	FormatValue(majorString, sizeof(majorString), major, majorFmt);

	if (use_sap_dis) {
		sprintf_s(sapDisStr, sizeof(sapDisStr), "%" PRIu64, sap_dis);
		ToBcdArrayString(sap_dis, sapDisBcdArray, sizeof(sapDisBcdArray));
	}

	fprintf(fBuildNumber, "/* Generated by Buildnumber %s */\n\n", VERSION);
	fprintf(fBuildNumber,
			"/*\n"
			"Version Patterns \n"
			"VER_MAJOR, VER_MINOR, and BUILDNUMBER contain the current numeric "
			"version values.\n"
			"SOFTWARE_DIS  Optional SAP Software DIS as Long, e.g. 09xxxxxxxxxx\n\n"
			"VERSION_FORMAT defines how VERSION_STR and BUILDNUMBER_STR are "
			"generated.\n\n"
			"Digits define the minimum number of numeric positions and are "
			"printed as leading zeros if necessary.\n"
			"Digits plus the number of '_' characters define the maximum number "
			"of numeric positions.\n\n"
			"\"___1\", for example, defines a maximum of 4 digits but a minimum "
			"of 1 digit.\n"
			"\"0001\", for example, defines a maximum of 4 digits and a minimum "
			"of 4 digits, printed with leading zeros.\n\n"
			"VERSION_FORMAT \"_0.00.000\" generates VERSION_STR values from "
			"\"0.00.001\" to \"99.99.999\".\n"
			"VERSION_FORMAT \"_0._0.__0\" generates VERSION_STR values from "
			"\"0.0.1\" to \"99.99.999\".\n"
			"*/ \n\n");
	fprintf(fBuildNumber, "#ifndef BUILD_NUMBER_H\n");
	fprintf(fBuildNumber, "#define BUILD_NUMBER_H\n\n");
	fprintf(fBuildNumber, "#include <stdint.h>\n\n");
	fprintf(fBuildNumber, "/* Configuration of generated version string */\n");
	fprintf(fBuildNumber, "#define VERSION_FORMAT \"%s\"\n\n", versionFormat);
	fprintf(fBuildNumber, "#define BUILD_MODE_%s\n\n", mode == MODE_PRE ? "PRE" : "POST");

	fprintf(fBuildNumber, "/* Customising the current version */\n");
	fprintf(fBuildNumber,
			"/* %s-build increment. Actual firmware version: %s */\n",
			mode == MODE_PRE ? "PRE" : "POST",
			actualVersionString);
	fprintf(fBuildNumber,
			"#define VER_MAJOR %u\t\t\t\t\t\t\t// Major version number as Integer\n",
			major);

	fprintf(fBuildNumber,
			"#define VER_MINOR %u\t\t\t\t\t\t\t// Minor version number as Integer\n",
			minor);

	fprintf(fBuildNumber,
			"#define BUILDNUMBER %u\t\t\t\t\t\t// Build number as Integer\n\n",
			build);

	if (use_sap_dis) {
		fprintf(fBuildNumber,
				"#define SOFTWARE_DIS %" PRIu64
				"ULL\t\t\// SAP Software DIS as Long\n\n",
				sap_dis);
	} else{
		fprintf(fBuildNumber,
		"// #define SOFTWARE_DIS 09xxxxxxxxxxULL\t\t// SAP Software DIS as Long\n\n");
	}

	fprintf(fBuildNumber, "/* "
						  "------------------------------------------------------"
						  "--------------------------- */\n"
						  "/* ******************* Generated version values. DO "
						  "NOT MODIFY! ******************** */\n"
						  "/* "
						  "------------------------------------------------------"
						  "--------------------------- */\n");
	fprintf(fBuildNumber,
			"#define MAJORMINOR %u%s\t\t\t\t\t\t// MajorMinor version number as "
			"Integer\n",
			major, minorString);
	fprintf(fBuildNumber,
			"#define MAJOR_STR  \"%s\"\t\t\t\t\t\t// Major version as String\n",
			majorString);
	fprintf(fBuildNumber,
			"#define MINOR_STR  \"%s\"\t\t\t\t\t\t// Minor version as String\n",
			minorString);
	fprintf(fBuildNumber,
			"#define MAJORMINOR_STR  \"%s%s\"\t\t\t\t// MajorMinor number as "
			"String\n",
			majorString, minorString);
	fprintf(fBuildNumber,
			"#define BUILDNUMBER_STR \"%s\"\t\t\t\t// Build number as String\n",
			buildString);
	fprintf(fBuildNumber,
			"#define VERSION_STR \"%s\"\t\t\t\t// Version number as String\n\n",
			versionString);
	if (use_sap_dis) {
		fprintf(
			fBuildNumber,
			"#define SOFTWARE_DIS_STR \"%12s\"\t\t// SAP software DIS as string\n",
			sapDisStr);
		fprintf(fBuildNumber,
				"#define SOFTWARE_DIS_BCD  {%s}\t// SAP software DIS as BCD Array\n\n",
				sapDisBcdArray);
	}
	fprintf(fBuildNumber,
			"#define BUILDDATE_STR \"%d-%02d-%02d %02d:%02d:%02d\"\t// Creation "
			"Date as String\n",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
			tm.tm_sec);
	fprintf(fBuildNumber, "\n");
	fprintf(fBuildNumber, "\n#endif\n");

	fclose(fBuildNumber);

	printf("\nFile %s updated.\n", filePath);
	printf("New build version : %s\n", versionString);

	return ERR_NONE;
}