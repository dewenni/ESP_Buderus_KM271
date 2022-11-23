#pragma once

// ======================================================
// includes
// ======================================================
#include <config.h>
#include <Arduino.h>

//*****************************************************************************
// Defines
//*****************************************************************************

// data struct for Flashstorage
  struct {
    long oilcounter = 0;
  } data;

// ======================================================
// Prototypes
// ======================================================
void sendOilmeter();
void cmdSetOilmeter(long setvalue);
void setupOilmeter();
void cyclicOilmeter();
void cmdStoreOilmeter();
