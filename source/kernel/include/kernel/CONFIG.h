// =============================================================
// reduceOS internal configuration file
// =============================================================
// SECTIONS OF THIS FILE ARE AUTOMATED BY BUILDSCRIPTS. DO NOT MODIFY ANY SECTIONS MARKED AS SO.
// This file contains configuration data for all of reduceOS, such as the release codename, build no. & date, debug/release config, etc.


#ifdef _KERNEL_CONFIGURATION_FILE
#define CODENAME "Cherry Blossom"
#define VERSION "1.4"

// DO NOT MODIFY THE BELOW LINES!!!
#define BUILD_NUMBER "11686"
#define BUILD_DATE "10/09/24, 17:00:28"
#define BUILD_CONFIGURATION "DEBUG"
#else
extern char *__kernel_version;
extern char *__kernel_codename;
extern char *__kernel_configuration;
extern char *__kernel_build_number;
extern char *__kernel_build_date;
extern char *__kernel_compiler;

#endif