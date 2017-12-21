// PlaceInRedStandalone.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "PlaceInRedStandalone.h"


// This is an example of an exported variable
PLACEINREDSTANDALONE_API int nPlaceInRedStandalone=0;

// This is an example of an exported function.
PLACEINREDSTANDALONE_API int fnPlaceInRedStandalone(void)
{
    return 42;
}

// This is the constructor of a class that has been exported.
// see PlaceInRedStandalone.h for the class definition
CPlaceInRedStandalone::CPlaceInRedStandalone()
{
    return;
}
