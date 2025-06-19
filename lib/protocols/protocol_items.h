#pragma once

#include "NiceFlo/NiceFloDecoder.h"
#include "Came/CameDecoder.h"
#include "Princeton/PrincetonDecoder.h"
#include "CameTwee/CameTweeDecoder.h"
#include "KeeLoq/KeeLoqDecoder.h"
#include "NeroSketch/NeroSketchDecoder.h"
#include "Hormann/HormannDecoder.h"
#include "NeroRadio/NeroRadioDecoder.h"
#include "GateTx/GateTxDecoder.h"
#include "Linear/LinearDecoder.h"
#include "LinearDelta3/LinearDelta3Decoder.h"
#include "MegaCode/MegaCodeDecoder.h"
#include "Holtek/HoltekDecoder.h"
#include "ChamberlainCode/ChamberlainCodeDecoder.h"
#include "PowerSmart/PowerSmartDecoder.h"
#include "Marantec/MarantecDecoder.h"
#include "Bett/BettDecoder.h"
#include "Doitrand/DoitrandDecoder.h"
#include "PhoenixV2/PhoenixV2Decoder.h"
#include "Honeywell/HoneywellDecoder.h"
#include "Magellan/MagellanDecoder.h"
#include "IntertechnoV3/IntertechnoV3Decoder.h"
#include "Ansonic/AnsonicDecoder.h"
#include "SMC5326/SMC5326Decoder.h"
#include "HoltekHT12x/HoltekHT12xDecoder.h"
#include "Dooya/DooyaDecoder.h"
#include "RawLogger/RawRecorder.h"

// Error in: GateTX, PowerSmart

const char *rf_protocols[] = {"NiceFlo", "Came", "Princeton", "CameTwee", "KeeLoq",
                              "NeroSketch", "Hormann", "NeroRadio", "GateTX", "Linear", 
                              "LinearD3", "MegaCode", "Holtek", "Chamberlain", "PowerSmart", 
                              "Marantec", "Bett", "DoitRand", "PhoenixV2", "Honeywell",
                                "Magellan", "Intertechno", "Clemsa", "Ansonic", "SMC5326",
                                "HoltekHT", "Dooya", "MasterCode", "DickertMAHS", "BinRaw"};

// #include "dickert_mahs.h"
// #include "clemsa.h"
// #include "mastercode.h"