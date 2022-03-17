/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#pragma once

#if defined(PARTICLE)
#include <Particle.h>
#include <math.h>
#else
#include <Arduino.h>
#endif

#include "config.h" // app configuration

#include "core/PixelNutSupport.h"
#include "core/PixelNutPlugin.h"
#include "core/PixelNutEngine.h"
