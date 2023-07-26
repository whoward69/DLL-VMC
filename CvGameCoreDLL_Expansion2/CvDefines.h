/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

#ifndef CVDEFINES_H
#define CVDEFINES_H

#if defined(MOD_GLOBAL_CITY_WORKING)
#define CITY_HOME_PLOT										(0)
#define MIN_CITY_RADIUS										(2)
// MAX_CITY_RADIUS should be the same as MAXIMUM_ACQUIRE_PLOT_DISTANCE in the XML, ie 5
// If you want to increase this value, you must also add to the arrays aiCityPlotX, aiCityPlotY, aiCityPlotPriority and aaiXYCityPlot in CvGlobals.cpp
#define AVG_CITY_RADIUS										(3)
#define MAX_CITY_RADIUS										(6)
#define MAX_CITY_PLOTS										((6 * (1+MAX_CITY_RADIUS) * MAX_CITY_RADIUS / 2) + 1)
#define AVG_CITY_PLOTS										(37)

// #define CITY_PLOTS_RADIUS(iPlayer)							((iPlayer != NO_PLAYER) ? GET_PLAYER(iPlayer).getBuyWorkDistance() : GC.getMAXIMUM_WORK_PLOT_DISTANCE())
// #define NUM_CITY_PLOTS(iPlayer)								((6 * (1+CITY_PLOTS_RADIUS(iPlayer)) * CITY_PLOTS_RADIUS(iPlayer) / 2) + 1)
#else
#define NUM_CITY_PLOTS										(37)
#define CITY_HOME_PLOT										(0)
#define CITY_PLOTS_RADIUS									(3)
#define CITY_PLOTS_DIAMETER									((CITY_PLOTS_RADIUS*2) + 1)
#endif

#define CIV5_WBMAP_EXT										".Civ5Map"

#define MAP_TRANSFER_EXT									"_t"

#define NEWLINE												"\n"
#define SEPARATOR											"\n-----------------------"

#endif	// CVDEFINES_H
