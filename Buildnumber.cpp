#define VERSION "V3.1"
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

static const char USAGE[2][1400] = {{R"(
BuildNumber %s

Automatically increments version and build numbers in a C header file.
If the file does not exist, it will be created.

Usage:
  BuildNumber [path] [file]

VERSION_FORMAT determines:
  - field formatting
  - maximum build number
  - maximum minor version
  - maximum major version

Parameters:
  path       Optional target directory.
  file       Header file name. Default: %s

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
int main(int argc, char* argv[]){
	char fileName[FILENAME_MAX] = DEFAULTFILENAME;
	char filePath[FILENAME_MAX] = "";

	bool resetVersion = false;

	/* Help */

	if (argc > 1) {
		if (!strcmp(argv[1], "/?") || !strcmp(argv[1], "/h") ||
			!strcmp(argv[1], "-?") || !strcmp(argv[1], "-h")) {
			printf(USAGE[0], VERSION, DEFAULTFILENAME);
			printf("%s", USAGE[1]);

			return ERR_NONE;
		}
	}

	/* Parameter auswerten */

	if (argc > 1) {
		if (!strcmp(argv[argc - 1], "0")) {
			resetVersion = true;
			printf("Reset version number\n");
		}

		if (argc == 2) {
			/* Buildnumber.exe test.h */

			strcpy_s(fileName, sizeof(fileName), argv[1]);
		} else if (argc == 3) {
			/* BuildNumber path file
			   oder
			   BuildNumber file 0 */

			if (resetVersion) {
				strcpy_s(fileName, sizeof(fileName), argv[1]);
			} else {
				strcat_s(filePath, sizeof(filePath), argv[1]);

				size_t len = strlen(filePath);

				if (len > 0 && filePath[len - 1] != '\\' && filePath[len - 1] != '/') {
					strcat_s(filePath, sizeof(filePath), "\\");
				}

				strcpy_s(fileName, sizeof(fileName), argv[2]);
			}
		} else if (argc == 4) {
			if (!resetVersion) {
				printf("Illegal number of parameters\n");
				return ERR_PARSE;
			}

			/* BuildNumber path file 0 */

			strcat_s(filePath, sizeof(filePath), argv[1]);

			size_t len = strlen(filePath);

			if (len > 0 && filePath[len - 1] != '\\' && filePath[len - 1] != '/') {
				strcat_s(filePath, sizeof(filePath), "\\");
			}

			strcpy_s(fileName, sizeof(fileName), argv[2]);
		} else if (argc > 1) {
			printf("Illegal number of parameters\n");
			return ERR_PARSE;
		}
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

	printf("Old build version: %s\n", versionString);

	if (resetVersion) {
		major = 0;
		minor = 0;
		build = 0;
	} else if (++build > maxBuild) {
		build = 0;

		/* Prüfen, ob minor breiter werden würde */
		if (minor >= maxMinor) {
			minor = 0;
			major++;

			if (major > maxMajor) {
				printf("Maximum version reached\n");
				return ERR_MAXVER;
			}
		} else {
			minor++;
		}
	}

	fBuildNumber = fopen(filePath, "w");

	if (fBuildNumber == NULL) {
		printf("Could not create file %s\n", filePath);
		return ERR_FOPEN;
	}

	time_t t = time(NULL);
	struct tm tm;
	tm = *localtime(&t);

	BuildVersionString(versionString, sizeof(versionString), major, minor, build,
					   versionFormat);

	char buildString[32];
	FormatValue(buildString, sizeof(buildString), build, buildFmt);
	char minorString[32];
	FormatValue(minorString, sizeof(minorString), minor, minorFmt);
	char majorString[32];
	FormatValue(majorString, sizeof(majorString), major, majorFmt);

	char sapDisStr[32];
	char sapDisBcdArray[128];
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

	fprintf(fBuildNumber, "/* Customising the current version */\n");
	fprintf(
		fBuildNumber,
		"#define VER_MAJOR %u\t\t\t\t\t\t\t// Major version number as Integer\n",
		major);
	fprintf(
		fBuildNumber,
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