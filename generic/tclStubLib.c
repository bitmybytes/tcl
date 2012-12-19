/*
 * tclStubLib.c --
 *
 *	Stub object that will be statically linked into extensions that want
 *	to access Tcl.
 *
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Copyright (c) 1998 Paul Duffin.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

MODULE_SCOPE const TclStubs *tclStubsPtr;
MODULE_SCOPE const TclPlatStubs *tclPlatStubsPtr;
MODULE_SCOPE const TclIntStubs *tclIntStubsPtr;
MODULE_SCOPE const TclIntPlatStubs *tclIntPlatStubsPtr;

const TclStubs *tclStubsPtr = NULL;
const TclPlatStubs *tclPlatStubsPtr = NULL;
const TclIntStubs *tclIntStubsPtr = NULL;
const TclIntPlatStubs *tclIntPlatStubsPtr = NULL;

static const TclStubs *
HasStubSupport(
    Tcl_Interp *interp,
    const char *tclversion,
    int magic)
{
    Interp *iPtr = (Interp *) interp;

    if (iPtr->stubTable && iPtr->stubTable->magic == magic) {
	return iPtr->stubTable;
    }
    /* This can never be executed in a Tcl > 8 interpreter, because
     * the magic values are kept the same among versions. */
    iPtr->objResultPtr = (Tcl_Obj *)
	    "interpreter uses an incompatible stubs mechanism";
    iPtr->emptyObjPtr = 0; /* TCL_STATIC */
    return NULL;
}

/*
 * Use our own isdigit to avoid linking to libc on windows
 */

static int isDigit(const int c)
{
    return (c >= '0' && c <= '9');
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitStubs --
 *
 *	Tries to initialise the stub table pointers and ensures that the
 *	correct version of Tcl is loaded.
 *
 * Results:
 *	The actual version of Tcl that satisfies the request, or NULL to
 *	indicate that an error occurred.
 *
 * Side effects:
 *	Sets the stub table pointers.
 *
 *----------------------------------------------------------------------
 */
MODULE_SCOPE const char *
TclInitStubs(
    Tcl_Interp *interp,
    const char *version,
    int exact,
    const char *tclversion,
    int magic)
{
    const char *actualVersion = NULL;
    ClientData pkgData = NULL;
    const TclStubs *stubsPtr;
    int major;

    /*
     * We can't optimize this check by caching tclStubsPtr because that
     * prevents apps from being able to load/unload Tcl dynamically multiple
     * times. [Bug 615304]
     */

    stubsPtr = HasStubSupport(interp, tclversion, magic);
    if (!stubsPtr) {
	return NULL;
    }

    actualVersion = stubsPtr->tcl_PkgRequireEx(interp, "Tcl", version, 0, &pkgData);
    if (actualVersion == NULL) {
	return NULL;
    }
    if (exact&1) {
	const char *p = version;
	int count = 0;

	while (*p) {
	    count += !isDigit(*p++);
	}
	if (count == 1) {
	    const char *q = actualVersion;

	    p = version;
	    while (*p && (*p == *q)) {
		p++; q++;
	    }
	    if (*p || isDigit(*q)) {
		/* Construct error message */
		stubsPtr->tcl_PkgRequireEx(interp, "Tcl", version, 1, NULL);
		return NULL;
	    }
	} else {
	    actualVersion = stubsPtr->tcl_PkgRequireEx(interp, "Tcl", version, 1, NULL);
	    if (actualVersion == NULL) {
		return NULL;
	    }
	}
    }

    if (sizeof(size_t) != sizeof(int)) {
	stubsPtr->tcl_GetVersion(&major, NULL, NULL, NULL);
	if ((exact & TCL_STUB_COMPAT_MASK) != ((major>8)?TCL_STUB_COMPAT:0)) {
	    char *msg = stubsPtr->tcl_Alloc(64 + strlen(tclversion) + strlen(version));

	    strcpy(msg, "incompatible stub library: have ");
	    strcat(msg, tclversion);
	    strcat(msg, ", need ");
	    if (major > 8) {
		strcat(msg, "9");
	    } else {
		/* Take "version", but strip off everything after '-' */
		char *p = msg + strlen(msg);
		while (*version && *version != '-') {
		    *p++ = *version++;
		}
		*p = '\0';
	    }
	    stubsPtr->tcl_SetObjResult(interp, stubsPtr->tcl_NewStringObj(msg, -1));
	    stubsPtr->tcl_Free(msg);
	    return NULL;
	}
    }

    tclStubsPtr = (TclStubs *)pkgData;

    if (tclStubsPtr->hooks) {
	tclPlatStubsPtr = tclStubsPtr->hooks->tclPlatStubs;
	tclIntStubsPtr = tclStubsPtr->hooks->tclIntStubs;
	tclIntPlatStubsPtr = tclStubsPtr->hooks->tclIntPlatStubs;
    } else {
	tclPlatStubsPtr = NULL;
	tclIntStubsPtr = NULL;
	tclIntPlatStubsPtr = NULL;
    }

    return actualVersion;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
