/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASICPROPERTYPAGE_H_INCLUDED
#define _BASICPROPERTYPAGE_H_INCLUDED

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "resource.h"
#include <citk.h>
using namespace citkTypes;
using namespace citk;

//#include "BasicPropertySheet.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class ClassDescPP ;

class CBasicPropertyPage 
{
public:
	
	virtual bool		SetObject( CTBasic* object, int verbose ) = 0;
	virtual HPROPSHEETPAGE CreatePropPage() = 0;
};



template <class _T>
class CBasicPropertyPageImpl : 
	public CBasicPropertyPage, 
	public CPropertyPageImpl<_T>
{
public:
	virtual HPROPSHEETPAGE CreatePropPage() { return Create(); }
};


/////////////////////////////////////////////////////////////////////////////
// PropertyPage ClassDesc
// Works much like the original citk ClassDesc, but used for Win32 Property Pages

class ClassDescPP 
{
public:
	
	long	Tag;
	
	virtual CBasicPropertyPage*		Create() = 0;
	virtual void					Delete( CBasicPropertyPage* ) = 0;
	
	virtual int						GetVerbose() = 0;
	virtual cdid_t					GetClassID() = 0;
	virtual cstr_t					GetName() = 0;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Defines for declaring/implementinga custom property page's ClassDesc
//
// __class			The MFC PP class (derived from CBasicPropertyPage)
// __citkclass		The citk class (derived from CTBasic)
// __verbose		A verbose number; determines if the page is displayed

#define DECLARE_PROPERTYPAGE(__class)										\
extern ClassDescPP* __cdecl ClassDescPP_##__class(void);

#define IMPLEMENT_PROPERTYPAGE(__class,__citkclass,__verbose)				\
	class PPCD_##__class : public ClassDescPP	{							\
public:																	\
	virtual CBasicPropertyPage* Create() {								\
	CBasicPropertyPage* pp = new __class;							\
	return pp; }													\
	virtual int		GetVerbose() {										\
	return __verbose ; }											\
	virtual cstr_t	GetName() {											\
	return #__class ; }												\
	virtual cdid_t	GetClassID() {										\
	return CDID( __citkclass ); }									\
	virtual void	Delete( CBasicPropertyPage* pp ) {					\
	delete pp; }													\
} _PPCD_##__class;														\
	extern ClassDescPP* __cdecl ClassDescPP_##__class(void)	{				\
return &_PPCD_##__class; }



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Containter for property page ClassDescs

struct DPropertyPages
{
	ArrayPtr<ClassDescPP> pages;
	
	int				GetPropertyPageCount() const { return pages.Count(); }
	ClassDescPP*	GetPropertyPageNo( int no ) const { return pages.GetNo(no); }
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Use this define in the main source file for your property pages project
// Declares the DLL's entry point (citk_PropertyPages)
// You only need two pairs of braces filled with ADDPROPERTYPAGE(...)

#define IMPLEMENT_PROPERTYPAGES(__name)										\
	static struct PropertyPages_##__name : DPropertyPages {					\
	void DoPropertyPages();												\
} _propertypages;														\
	extern "C" DLL_EXPORT void* __cdecl citk_PropertyPages( void ) {		\
	if (_propertypages.GetPropertyPageCount()==0)						\
	_propertypages.DoPropertyPages();								\
	return (void*)&_propertypages; }									\
void PropertyPages_##__name::DoPropertyPages()

// Use this define in YourPages::DoPropertyPages
// Right after IMPLEMENT_PROPERTYPAGES
#define ADDPROPERTYPAGE(__class) 											\
	DECLARE_PROPERTYPAGE(__class)											\
pages.Add( ClassDescPP_##__class () );


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#endif // !_BASICPROPERTYPAGE_H_INCLUDED
