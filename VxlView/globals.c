#include "buildcfg.h"
#include <KexComm.h>

//
// These are defined normally here but in the header file they are externed
// as CONST. This is to prevent them from being edited by random code by
// accident.
//
HWND MainWindow = NULL;
HWND ListViewWindow = NULL;
HWND StatusBarWindow = NULL;
HWND DetailsWindow = NULL;
HWND FilterWindow = NULL;