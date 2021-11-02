/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#pragma once
#define PLUGIN_TYPE_REDRAW        0x01  // creates pixel values from settings, else
                                        // alters those effect settings before drawing

                                        // any combination of these is valid:
#define PLUGIN_TYPE_DIRECTION     0x08  // changing direction changes effect
#define PLUGIN_TYPE_TRIGGER       0x10  // triggering changes the effect
#define PLUGIN_TYPE_USEFORCE      0x20  // trigger force is used in effect
#define PLUGIN_TYPE_NEGFORCE      0x40  // negative trigger force is used
#define PLUGIN_TYPE_SENDFORCE     0x80  // sends trigger force to other plugins

