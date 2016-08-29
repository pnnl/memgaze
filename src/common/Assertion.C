// Copyright (c) 2000 Rice University

// $Id: Assertion.C,v 1.1 2001/06/18 12:13:03 johnmc Exp $

// -+- C++ -+-
//***************************************************************************
//                          All Rights Reserved
//***************************************************************************
 
//***************************************************************************
//
// File:
//    Assertion.C
//
// Purpose:
//    See Assertion.h for information.
//
// Description:
//    See Assertion.h for information.
//
// History:
//    ??/?? - Originated as some code written by Elana Granston
//            (granston@cs.rice.edu)
//    94/07 - Rewritten, extended and ported to C++ by Mark Anderson
//            (marka@cs.rice.edu)
//    95/08 - Revised with a view to installation in the D System by Mark
//            Anderson (marka) and Kevin Cureton (curetonk)
//
//***************************************************************************


//************************** System Include Files ***************************

#include <stdlib.h>

#include <fstream>
#include <iostream>
#if !defined(__DECCXX)
 using namespace std;
#endif

#include <string.h>

//*************************** User Include Files ****************************

//#include "general.h"
#include "Assertion.h"

//************************** Variable Definitions ***************************

// Global debugging flag initially defaults to local
GlobalAssertMsgsType globalAssertMsgsEnabled = GLOBAL_ASSERT_MSG_LOCAL;

const char* ASSERTION_FAIL_STRING = "Assert Failed";

const int ASSERTION_RETURN_VALUE = 101;

// Used to handle a possible external logfile.
static ofstream logfile;

// is external logging on??
static bool externLog = false;

// full assertion trace
int FullAssertionTrace = false; 

//*********************** Static Function Prototypes ************************

static void DumpAssertionInfo(ostream & out, 
                              const char * expr,
                              const char * errorDesc, 
                              const char * section, 
                              const char * fileVersion, 
                              const char * fileName,
                              int lineNo); 
 
static void Quit(AssertionExitType action, int status);
 

//**************************** External Functions ***************************


////////////////////////////////////////////////////////////////////////////
// Provides convenient external breakpoint for grabbing assertions in debugger
///////////////////////////////////////////////////////////////////////////
void AssertionStopHere(AssertionExitType quitAction)
{
  // Empty code to allow user to select which type to stop on
  switch (quitAction) {
  case ASSERT_ABORT:	
    break;  
  case ASSERT_EXIT:	
    break;
  case ASSERT_NOTIFY:
    break;
  case ASSERT_CONTINUE:      
    break;
  default:
    break;
  }    
}


////////////////////////////////////////////////////////////////////////////
// NoteFailureThenDie() Handle printing of error message, then goto
// the exit routines
//////////////////////////////////////////////////////////////////////////// 

void NoteFailureThenDie(const char * expr, 
                        const char * errorDesc, 
			const char * fileVersion, 
			const char * fileName, 
                        int lineNo, 
			AssertionExitType quitAction, 
			int status) 
{
  AssertionStopHere(quitAction);

    if ((quitAction == ASSERT_CONTINUE) || 
	((!FullAssertionTrace) && (quitAction == ASSERT_NOTIFY))) return;
    
    DumpAssertionInfo(cerr, expr, errorDesc, (const char*)NULL, 
                      fileVersion, fileName, lineNo);

    if (externLog) {
	DumpAssertionInfo(logfile, expr, errorDesc, (const char*)NULL, 
			  fileVersion, fileName, lineNo);
    }

    Quit(quitAction, status);

    if (localAssertMsgsEnabled)  // dummy test to get rid of unused variable warning
       return;

} // NoteFailureAndDie

////////////////////////////////////////////////////////////////////////////
// NoteFailureAndSectionThenDie() Like above, but prints section
//////////////////////////////////////////////////////////////////////////// 

void NoteFailureAndSectionThenDie(const char * expr, 
                                  const char * errorDesc,
 				  const char * section, 
				  const char * fileVersion, 
				  const char * fileName, 
                                  int lineNo, 
				  AssertionExitType quitAction, 
				  int status) 
{
  AssertionStopHere(quitAction);
    if ((quitAction == ASSERT_CONTINUE) || 
	((!FullAssertionTrace) && (quitAction == ASSERT_NOTIFY))) return;
  	
    DumpAssertionInfo(cerr, expr, errorDesc, section,
		      fileVersion, fileName, lineNo);

    if (externLog) {
	DumpAssertionInfo(logfile, expr, errorDesc, section,
			  fileVersion, fileName, lineNo);
    }

    Quit(quitAction, status);

    return;

} // NoteFailureAndDie


//***************************** Static Functions ****************************

////////////////////////////////////////////////////////////////////////////
// DumpAssertionInfo()
//////////////////////////////////////////////////////////////////////////// 

static void DumpAssertionInfo(ostream & out, 
                              const char * expr, 
                              const char * errorDesc, 
                              const char * section, 
		              const char * fileVersion,  
                              const char * fileName, 
		              int lineNo)
{
    if (errorDesc != 0 && *errorDesc != 0) {
	out << "ASSERTION FAILED: " << errorDesc << endl;
    }	
    else {
        out << "ASSERTION FAILED:" << endl;
    }

    out << "    File: \"" << fileName << "\", line " << lineNo << endl; 

    out << "    File Version: " << fileVersion << endl;    

    if (section != 0 && *section != 0) {
	out << "    Section: " << section << endl;
    }	

    out << "    Assertion Expression: " << expr << endl;

    out.flush();  			   

    return;
}			           

////////////////////////////////////////////////////////////////////////////
// Quit()
//////////////////////////////////////////////////////////////////////////// 

static void Quit(AssertionExitType action, int status) 
{
    switch (action) 
    {
        case ASSERT_ABORT:	
            abort();
        break;  

        case ASSERT_EXIT:	
            exit(status);
        break;

        case ASSERT_NOTIFY:
            // Do nothing for this selection
        break;

        case ASSERT_CONTINUE:      
            // Should never reach this point
            cerr << "Assertion.C: Internal Error in Quit()." 
                 << endl;
        break;

        default:   
            // Should never reach this point
            cerr << "Assertion.C: Internal Error in Quit()." 
                 << endl;
            if (externLog) 
            {
                logfile << "Assertion.C: Internal Error in Quit()." 
                        << endl;
            }
        break;
    }

    return;

}  // end Quit
