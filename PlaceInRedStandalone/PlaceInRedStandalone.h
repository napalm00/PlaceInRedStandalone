// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the PLACEINREDSTANDALONE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// PLACEINREDSTANDALONE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef PLACEINREDSTANDALONE_EXPORTS
#define PLACEINREDSTANDALONE_API __declspec(dllexport)
#else
#define PLACEINREDSTANDALONE_API __declspec(dllimport)
#endif

// This class is exported from the PlaceInRedStandalone.dll
class PLACEINREDSTANDALONE_API CPlaceInRedStandalone {
public:
	CPlaceInRedStandalone(void);
	// TODO: add your methods here.
};

extern PLACEINREDSTANDALONE_API int nPlaceInRedStandalone;

PLACEINREDSTANDALONE_API int fnPlaceInRedStandalone(void);
