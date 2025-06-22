#pragma once

#include "NiceFlo/NiceFloDecoder.h"
#include "Came/CameDecoder.h"
#include "CameTwee/CameTweeDecoder.h"
#include "NeroSketch/NeroSketchDecoder.h"
#include "Hormann/HormannDecoder.h"
#include "NeroRadio/NeroRadioDecoder.h"
#include "GateTx/GateTxDecoder.h"
#include "Linear/LinearDecoder.h"
#include "LinearDelta3/LinearDelta3Decoder.h"
#include "Holtek/HoltekDecoder.h"
#include "ChamberlainCode/ChamberlainCodeDecoder.h"
#include "PowerSmart/PowerSmartDecoder.h"
#include "Marantec/MarantecDecoder.h"
#include "Doitrand/DoitrandDecoder.h"
#include "PhoenixV2/PhoenixV2Decoder.h"
#include "Ansonic/AnsonicDecoder.h"
#include "SMC5326/SMC5326Decoder.h"
#include "HoltekHT12x/HoltekHT12xDecoder.h"
#include "RawLogger/RawRecorder.h"

// Error in: GateTX, PowerSmart

const char *rf_protocols[] = {"NiceFlo", "Came", "CameTwee", "NeroSketch", "Hormann", 
                              "NeroRadio", "GateTX", "Linear", "LinearD3", "Holtek", 
                              "Chamberlain", "PowerSmart", "Marantec", "DoitRand", "PhoenixV2", 
                              "Ansonic", "SMC5326", "HoltekHT", "Raw"};