// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


// TODO: reference additional headers your program requires here

#ifndef _XBOX
//#define CITK_IMPORTS
#else
#include <xtl.h>
#endif

#ifdef CITK_IMPORTS
#error
#endif

#include <citk.h>
using namespace citk;
