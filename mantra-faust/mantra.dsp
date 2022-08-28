
declare name "Mantra v.0 Ring Modulation";

/* ======== DESCRIPTION ==========
- Ring modulation consists in modulating a sound by multiplying it with a sine wave
- Head = no modulation
- Downward = modulation, varying the modulating frequency
*/

import("stdfaust.lib");


process = ringmod;

ringmod = _<:_,*(os.oscs(freq)):drywet
		with {
            freq = hslider ( "[1]Modulation Frequency[style:knob][acc:1 0 -10 0 10][scale:log]", 220,5,1000,0.001):si.smooth(0.999);
            drywet(x,y) = (1-c)*x + c*y;
            c = hslider("[2]Modulation intensity[style:knob][unit:%][acc:1 0 -10 0 10]", 90,0,100,0.01)*(0.01):si.smooth(0.999);
        };