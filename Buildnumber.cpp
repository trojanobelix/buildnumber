#define VERSION "V2.3"
#define DEFAULTFILENAME "buildNumber.h" 

#define ERR_MAXVER      -4
#define ERR_PARSE       -3
#define ERR_FILENAME    -2
#define ERR_FOPEN       -1
#define ERR_NONE         0

// BuildNumber.cpp : Defines the entry point for the console application.

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

    Options:
      -h -? /h /?   Show this screen.
      path          OPTIONAL: path in which the header is to be changed or created. This is required, for example, if the MPLABX macro ${ProjectDir} (or $(ProjectDir) for VS/GCC) is used to determine the project directory.
      max           OPTIONAL: Maximum build number. Default 999. MINOR and MINOR are incremented if build number is greater than max. If max = 0 MAJOR, MINOR and BUILD are reseted to 0.0.000. 
      file          file name of the header file to be modified or created. Default file name is)"},
{R"(

    Example:
      c:\microchip\buildnumber.exe ${ProjectDir} ".\buildnumber.h" 99   // Modifies or creates buildnumber.h in the MPLABX project directory with maximum 99 as build number. If greater MINOR version is incremented.

    Format of the header file:
      #define BUILDNUMBER BBB                     // Build number as Integer (0...max, max default = 999)
      #define BUILDNUMBER_STR "BBB"               // Build number as String (with leading "0" up to the string length of the parameter "max")
      #define VER_MAJOR MAJOR                     // Major version number as Integer (0...99)
      #define VER_MINOR MINOR                     // Minor version number as Integer
      #define VERSION_STR "MAJOR.MINOR.BBB"       // Version number as String
      #define BUILDDATE_STR "YYYY-MM-DD hh:mm:ss" // Creation Date as String 
    )"} };


int main(int argc, char* argv[]) {
    char fileName[FILENAME_MAX] = DEFAULTFILENAME;
    char filePath[FILENAME_MAX] = "";
    int maxBuildnumber = 999;
    int lenght_maxBuildnumber = 3;
    bool resetVersion = false;
    bool MaxVersion = false;
    //Get filename if it's passed
    if (argc > 1) {
        if (!strcmp(argv[1], "/?") || !strcmp(argv[1], "/h") || !strcmp(argv[1], "-?") || !strcmp(argv[1], "-h")) {
            printf("buildnumber %s\n%s %s %s", VERSION, USAGE[0], DEFAULTFILENAME,USAGE[1]);
            return ERR_NONE;
        }
    }
    else { 
        strcpy_s(fileName, FILENAME_MAX, argv[1]); 
    }
    

    //Get max Buildnumber if it's passed
    if (argc > 1) {

        if (!strcmp(argv[argc - 1], "0")) {
         resetVersion = true;
         MaxVersion = true;
         printf("Reset version number\n");
        }
        else if (!strcmp(argv[argc - 1], "/?") || !strcmp(argv[argc - 1], "/h") || !strcmp(argv[argc - 1], "-?") || !strcmp(argv[argc - 1], "-h")) {
            printf("buildnumber %s\n%s %s %s", VERSION, USAGE[0], DEFAULTFILENAME, USAGE[1]);
            return ERR_NONE;
        }
        else {

            maxBuildnumber = atoi(argv[argc - 1]);

            if (maxBuildnumber > 0 && !errno && maxBuildnumber <= INT_MAX)
            {
                MaxVersion = true;
                lenght_maxBuildnumber = strlen(argv[argc - 1]);
                //printf("All parameters must be a positive integers. Could not convert % s", argv[argc - 1]);
                //return ERR_PARSE;

            }
            else {
                MaxVersion = false; // last parmeter is not a number
                maxBuildnumber = 999;
                lenght_maxBuildnumber = 3;
            }

            int offset = MaxVersion ? 1 : 0;
            
             if (argc == 3)  { 
                 strcpy_s(fileName, FILENAME_MAX, argv[2-offset]);
                 if(!MaxVersion)
                    strcat_s(filePath, FILENAME_MAX, argv[1-offset]);
            }
            else if (argc == 4 && MaxVersion)  {
                strcpy_s(fileName, FILENAME_MAX, argv[2]);
                strcat_s(filePath, FILENAME_MAX, argv[1]);
            }
            else {

                printf("Illegal extra parameters: %s", argv[4]); 
            }

        }
    }


    //Create the path to the executable folder
    char* prevToken;
    char* curToken;

    //curToken = strtok(argv[0], "\\");

    //do {
    //    prevToken = curToken;
    //    curToken = strtok(NULL, "\\");

    //    if (curToken != NULL) {
    //        strcat_s(filePath, FILENAME_MAX, prevToken);
    //        strcat_s(filePath, FILENAME_MAX, "\\");
    //    }
    //} while (curToken != NULL);


    //Create File Path
    int nMax = FILENAME_MAX - strlen(filePath) - strlen(fileName);
    strncat_s(filePath, FILENAME_MAX, fileName, FILENAME_MAX - nMax);

    //File Generation
    FILE* fBuildNumber;
    unsigned int major = 0;
    unsigned int minor = 0;
    unsigned int build = 0;

    //Check if file exists
    fBuildNumber = fopen(filePath, "r");
    if (fBuildNumber != NULL ) {
        //File exists. Parse it to read the file
        char* eof;
        char fileLine[100];

        eof = fgets(fileLine, sizeof(fileLine), fBuildNumber);
        while (eof != NULL) {
            char* token;
            token = strtok(fileLine, " ");
            if (strcmp(token, "#define") == 0) {
                //Define
                token = strtok(NULL, " ");
                if (strcmp(token, "VER_MAJOR") == 0) {
                    //Get Ver Major
                    token = strtok(NULL, " ");
                    if (token != NULL) {
                        if (sscanf_s(token, "%u", &major) == 0) {
                            //Error parsing VER_MAJOR
                            printf("Error while parsing VER_MAJOR. Check if integer\n");
                            return ERR_PARSE;
                        }
                    }
                }
                else if (strcmp(token, "VER_MINOR") == 0) {
                    //Get Ver Minor
                    token = strtok(NULL, " ");
                    if (token != NULL) {
                        if (sscanf_s(token, "%u", &minor) == 0) {
                            //Error parsing VER_MINOR
                            printf("Error while parsing VER_MINOR. Check if integer\n");
                            return ERR_PARSE;
                        }
                    }
                }
                else if (strcmp(token, "BUILDNUMBER") == 0) {
                    //Get Ver Build
                    token = strtok(NULL, " ");
                    if (token != NULL) {
                        if (sscanf_s(token, "%u", &build) == 0) {
                            //Error parsing BUILDNUMBER
                            printf("Error while parsing BUILDNUMBER. Check if integer\n");
                            return ERR_PARSE;
                        }

    
                    }
                }
            }

            eof = fgets(fileLine, sizeof(fileLine), fBuildNumber);
        }

        fclose(fBuildNumber);
    }
    else {
        printf("File % s not found. Try to create a new file.\n", fileName);
        resetVersion = true;
    }



    //Increment build number
    printf("Old build version: %u.%u.%u\n", major, minor, build);

    if (resetVersion) {
        major = 0;
        minor = 0;
        build = 0;

    }else if (++build > maxBuildnumber) {
        //Max build number reached. Set to zero and increment minorm
        printf("maximum Build number %d reached. Increment minor version\n", maxBuildnumber);
        build = 0;
        minor++;
    if (minor >= 100) {
            //Max minor number reached. Set to zero and increment major
            printf("Maximum minor number reached. Increment major version\n");
            minor = 0;
            if (++major >= 100) {
                //Max version number reached. User should reset. Return ERROR
                printf("Maximum build number reached. Please reset to zero\n");
                return ERR_MAXVER;
            }
        }
    }
    //Create the new file
    fBuildNumber = fopen(filePath, "w");
    if (fBuildNumber != NULL) {


        time_t t;
        t = time(NULL);
        struct tm tm;
        tm = *localtime(&t);

        //File Created. Write the content
        fprintf(fBuildNumber, "/* Generated by BuildNumber %s */ \n\n", VERSION);
        fprintf(fBuildNumber, "#ifndef BUILD_NUMBER_H\n");
        fprintf(fBuildNumber, "#define BUILD_NUMBER_H\n");
        fprintf(fBuildNumber, "\n");
        fprintf(fBuildNumber, "#define BUILDNUMBER %d\t\t\t\t\t\t// Build number as Integer\n", build);
        fprintf(fBuildNumber, "#define BUILDNUMBER_STR \"%0*d\"\t\t\t\t// Build number as String\n", lenght_maxBuildnumber, build);
        fprintf(fBuildNumber, "#define VER_MAJOR %u\t\t\t\t\t\t\t// Major version number as Integer\n", major);
        fprintf(fBuildNumber, "#define VER_MINOR %02u\t\t\t\t\t\t// Minor version number as Integer\n", minor);
        fprintf(fBuildNumber, "#define MAJORMINOR_STR  \"%d%02d\"\t\t\t\t// MajorMinor number as String\n", major, minor);
        fprintf(fBuildNumber, "#define MAJORMINOR %u%02u\t\t\t\t\t\t// MajorMinor version number as Integer\n", major, minor);
        fprintf(fBuildNumber, "#define VERSION_STR \"%u.%02u.%0*u\"\t\t\t\t// Version number as String\n", major, minor, lenght_maxBuildnumber, build);
        fprintf(fBuildNumber, "#define BUILDDATE_STR \"%d-%02d-%02d %02d:%02d:%02d\"\t// Creation Date as String\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        fprintf(fBuildNumber, "\n");
        fprintf(fBuildNumber, "#endif\n");

        fflush(fBuildNumber);
        fclose(fBuildNumber);

        //Report to console
        printf("\nFile %s updated.\nNew build version : %u.%02u.%0*u\n", filePath, major, minor, lenght_maxBuildnumber, build);
        return ERR_NONE;
    }
    else {
        //Error in file creation.
        printf("Could not create file %s\n", filePath);
        return ERR_FOPEN;
    }
}
