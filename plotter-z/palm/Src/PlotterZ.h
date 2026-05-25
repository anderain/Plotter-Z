/*
 * PlotterZ.h
 *
 * header file for PlotterZ
 *
 * This wizard-generated code is based on code adapted from the
 * stationery files distributed as part of the Palm OS SDK 4.0.
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */
 
#ifndef PLOTTERZ_H_
#define PLOTTERZ_H_

/*********************************************************************
 * Internal Structures
 *********************************************************************/

typedef struct PlotterZPreferenceType
{
	Boolean pref1;
	char pref2[256];
} PlotterZPreferenceType;

/*********************************************************************
 * Global variables
 *********************************************************************/

extern PlotterZPreferenceType g_prefs;

/*********************************************************************
 * Internal Constants
 *********************************************************************/

#define appFileCreator			'KUKI'
#define appName					"PlotterZ"
#define appVersionNum			0x01
#define appPrefID				0x00
#define appPrefVersionNum		0x01

#endif /* PLOTTERZ_H_ */
