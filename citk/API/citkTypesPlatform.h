//  Copyright (C) 1998-2002 Crystal Intertechnology BV.  All rights reserved.
//  
//  This file is part of citk.
//  
//  citk is distributed with NO WARRANTY OF ANY KIND.  No author or
//  distributor accepts any responsibility for the consequences of using it,
//  or for whether it serves any particular purpose or works at all, unless
//  he or she says so in writing.  Refer to the Crystal Intertechnology
//  License (the "License") for full details.
//  
//

#pragma once

#include <citkTypes.h>

///////////////////////////////////////////////////////////////////////////////

#if defined _XBOX

# include <Xtl.h>
# include <citkTypesWin32.h>

#elif defined _WIN32

# define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
# define Array __Array
# include <windows.h>
# undef Array
// Undefine some stupid windows defines..
# undef CreateFile
# undef RegisterClass
# undef GetClassName
# undef SendMessage
# undef GetObject
# undef GetUserName
# undef GetComputerName
# undef GetEnvironmentVariable
# include <citkTypesWin32.h>

#elif defined __unix__

#include <citkTypesLinux.h>

#endif
