#define VERSION "V2.5"
#define DEFAULTFILENAME "buildNumber.h"

#define ERR_MAXVER      -4
#define ERR_PARSE       -3
#define ERR_FILENAME    -2
#define ERR_FOPEN       -1
#define ERR_NONE         0

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

static const char USAGE[2][1000] =
{ {R"(
Increments the version and build number in a c-header file or creates the c-Header file if it does not exist.

Usage:
  Buildnumber [<path>] <file> [max]
  Buildnumber (-h | -? | /h | /? )
)"},
{R"(
Example:
  Buildnumber . buildnumber.h 99
)"} };

int main(int argc, char* argv[])
{
    char fileName[FILENAME_MAX] = DEFAULTFILENAME;
    char filePath[FILENAME_MAX] = "";

    int maxBuildnumber = 999;
    int lenght_maxBuildnumber = 3;

    bool resetVersion = false;
    bool MaxVersion = false;
    
    int majorWidth = 1;
    int minorWidth = 1;
    
    /* Help */

    if (argc > 1)
    {
        if (!strcmp(argv[1], "/?") ||
            !strcmp(argv[1], "/h") ||
            !strcmp(argv[1], "-?") ||
            !strcmp(argv[1], "-h"))
        {
            printf("buildnumber %s\n%s %s %s",
                VERSION,
                USAGE[0],
                DEFAULTFILENAME,
                USAGE[1]);

            return ERR_NONE;
        }
    }

    /* Parameter auswerten */

    if (argc > 1)
    {
        if (!strcmp(argv[argc - 1], "0"))
        {
            resetVersion = true;
            MaxVersion = true;
            printf("Reset version number\n");
        }
        else
        {
            char* endptr = nullptr;
            long v = strtol(argv[argc - 1], &endptr, 10);

            if (*endptr == '\0' && v > 0)
            {
                MaxVersion = true;
                maxBuildnumber = (int)v;
                lenght_maxBuildnumber = (int)strlen(argv[argc - 1]);
            }
        }

        int offset = MaxVersion ? 1 : 0;

        if (argc == 2)
        {
            /* Buildnumber.exe test.h */

            strcpy_s(fileName, sizeof(fileName), argv[1]);
        }
        else if (argc == 3)
        {
            /* Buildnumber.exe path file
               oder
               Buildnumber.exe file max */

            if (MaxVersion)
            {
                strcpy_s(fileName, sizeof(fileName), argv[1]);
            }
            else
            {
                strcat_s(filePath, sizeof(filePath), argv[1]);

                size_t len = strlen(filePath);

                if (len > 0 &&
                    filePath[len - 1] != '\\' &&
                    filePath[len - 1] != '/')
                {
                    strcat_s(filePath, sizeof(filePath), "\\");
                }

                strcpy_s(fileName, sizeof(fileName), argv[2]);
            }
        }
        else if (argc == 4 && MaxVersion)
        {
            /* Buildnumber.exe path file max */

            strcat_s(filePath, sizeof(filePath), argv[1]);

            size_t len = strlen(filePath);

            if (len > 0 &&
                filePath[len - 1] != '\\' &&
                filePath[len - 1] != '/')
            {
                strcat_s(filePath, sizeof(filePath), "\\");
            }

            strcpy_s(fileName, sizeof(fileName), argv[2]);
        }
        else if (argc > 1)
        {
            printf("Illegal number of parameters\n");
            return ERR_PARSE;
        }
    }

    strcat_s(filePath, sizeof(filePath), fileName);

    printf("Using file: %s\n", filePath);

    FILE* fBuildNumber;

    unsigned int major = 0;
    unsigned int minor = 0;
    unsigned int build = 0;

    fBuildNumber = fopen(filePath, "r");

    if (fBuildNumber != NULL)
    {
        char fileLine[256];

        while (fgets(fileLine, sizeof(fileLine), fBuildNumber))
        {
            char* token = strtok(fileLine, " \t\r\n");

            if (token == NULL)
                continue;

            if (!strcmp(token, "#define"))
            {
                token = strtok(NULL, " \t\r\n");

                if (token == NULL)
                    continue;

                if (!strcmp(token, "VER_MAJOR"))
                {
                    token = strtok(NULL, " \t\r\n");

                    if (token != NULL)
                    {
                        majorWidth = (int)strlen(token);

                        if (sscanf_s(token, "%u", &major) == 0)
                        {
                            printf("Error while parsing VER_MAJOR\n");
                            return ERR_PARSE;
                        }
                    }
                }
                else if (!strcmp(token, "VER_MINOR"))
                {
                    token = strtok(NULL, " \t\r\n");

                    if (token != NULL)
                    {
                        minorWidth = (int)strlen(token);

                        if (sscanf_s(token, "%u", &minor) == 0)
                        {
                            printf("Error while parsing VER_MINOR\n");
                            return ERR_PARSE;
                        }
                    }
                }
                else if (!strcmp(token, "BUILDNUMBER"))
                {
                    token = strtok(NULL, " \t\r\n");

                    if (token)
                        sscanf_s(token, "%u", &build);
                }
            }
        }

        fclose(fBuildNumber);
    }
    else
    {
        printf("File %s not found. Try to create a new file.\n", filePath);
        resetVersion = true;
    }

    printf("Old build version: %0*u.%0*u.%0*u\n",
        majorWidth,
        major,
        minorWidth,
        minor,
        lenght_maxBuildnumber,
        build);

    if (resetVersion)
    {
        major = 0;
        minor = 0;
        build = 0;
    }
    else if (++build > (unsigned int)maxBuildnumber)
    {
        unsigned int oldMajor = major;
        unsigned int oldMinor = minor;

        build = 0;

        /* Prüfen, ob minor breiter werden würde */
        int newMinorWidth = snprintf(NULL, 0, "%u", minor + 1);

        if (newMinorWidth > minorWidth)
        {
        //    printf("WARNING: VER_MINOR width overflow (%0*u -> %u). "
        //        "Incrementing VER_MAJOR instead.\n", minorWidth, oldMinor, minor + 1);

            minor = 0;
            major++;

            int newMajorWidth = snprintf(NULL, 0, "%u", major);

            if (newMajorWidth > majorWidth)
            {
                printf("WARNING: VER_MAJOR changed string length (%0*u -> %u)\n",
                    majorWidth,
                    oldMajor,
                    major);

                majorWidth = newMajorWidth;
            }
        }
        else
        {
            minor++;
        }
    }

    fBuildNumber = fopen(filePath, "w");

    if (fBuildNumber == NULL)
    {
        printf("Could not create file %s\n", filePath);
        return ERR_FOPEN;
    }

    time_t t = time(NULL);
    struct tm tm;
    tm = *localtime(&t);

    fprintf(fBuildNumber, "/* Generated by BuildNumber %s */\n\n", VERSION);

    fprintf(fBuildNumber, "#ifndef BUILD_NUMBER_H\n");
    fprintf(fBuildNumber, "#define BUILD_NUMBER_H\n\n");

    fprintf(fBuildNumber, "#define VER_MAJOR %0*u\n", majorWidth, major);

    fprintf(fBuildNumber, "#define VER_MINOR %0*u\n", minorWidth, minor);
    fprintf(fBuildNumber, "#define BUILDNUMBER %u\n\n", build);

    fprintf(fBuildNumber, "#define BUILDNUMBER_STR \"%0*d\"\n", lenght_maxBuildnumber, build);

    fprintf(fBuildNumber,
        "#define VERSION_STR \"%0*u.%0*u.%0*u\"\n",
        majorWidth,
        major,
        minorWidth,
        minor,
        lenght_maxBuildnumber,
        build);

    fprintf(fBuildNumber,
        "#define BUILDDATE_STR \"%d-%02d-%02d %02d:%02d:%02d\"\n",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec);

    fprintf(fBuildNumber, "\n#endif\n");

    fclose(fBuildNumber);

    printf("\nFile %s updated.\n", filePath);
    printf("New build version : %0*u.%0*u.%0*u\n",
        majorWidth,
        major,
        minorWidth,
        minor,
        lenght_maxBuildnumber,
        build);

    return ERR_NONE;
}