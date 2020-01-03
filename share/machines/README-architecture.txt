-*-Mode: markdown;-*- [outline]
=============================================================================

http://www.agner.org/optimize/
http://www.agner.org/optimize/microarchitecture.pdf

https://www.7-cpu.com/

=============================================================================
Intel Xeon Phi
=============================================================================

Knights Landing: Intel Xeon Phi Gen2
----------------------------------------

https://doi.org/10.1109/MM.2016.25

=============================================================================
Intel Xeon
=============================================================================


Ice Lake (shrink of SkyLake)
----------------------------------------

IceLake Server:
  - Process: 10 nm
  

Cascade Lake (optimization): 
----------------------------------------
- https://en.wikichip.org/wiki/intel/microarchitectures/cascade_lake


SkyLake Server (arch): Intel x86-64 Core Gen6
----------------------------------------

- https://en.wikichip.org/wiki/intel/microarchitectures/skylake_(server)
- https://en.wikipedia.org/wiki/Skylake_(microarchitecture)

SkyLake Server:
  - Process: 14 nm
  - AVX-512
  - 6 channels DDR4
  - Cache structure
	- Change ratio of L2 and L3:
	  - more high-speed private cache; less slow shared/contended cache
      - L1 and L2 are core-private
      - 4x increase size for L2, double L2 bandwidth
	  - reduce size of L3 (shared)
    - L2 remains non-inclusive (e.g., prefetch data into L1 that is not in L2)
    - L3 is non-inclusive (which is distinct from strict exclusive)
	  Something in L1 or L2 may/may not be in L3
  - Sub-NUMA clustering
  - UPI (mesh interconnect vs. prior ring interconnect)
  - Optane, Omni-Path
  
  32 cores/64 threads


Broadwell (shrink of Haswell): Intel x86-64 Core Gen5
----------------------------------------

Broadwell EP ()
  - Process: 14 nm
  - AVX (256)
  - 4 channels DDR4
- https://en.wikipedia.org/wiki/Broadwell_(microarchitecture)
- http://www.tomshardware.com/reviews/intel-xeon-e5-2600-v4-broadwell-ep,4514.html


Haswell-E (arch): Intel x86-64 Core Gen4
----------------------------------------

- https://en.wikipedia.org/wiki/Haswell_(microarchitecture)
- http://www.tomshardware.com/reviews/intel-xeon-e5-2600-v3-haswell-ep,3932.html

- AVX2 (256 bits)


IvyBridge-E (shrink of SandyBridge): Intel x86-64 Core Gen3
----------------------------------------

- https://en.wikipedia.org/wiki/Ivy_Bridge_(microarchitecture)
- http://www.tomshardware.com/reviews/xeon-e5-2600-v2-ivy-bridge-ep-benchmarks,3714.html
  

SandyBridge (arch): Intel x86-64 Core Gen2
----------------------------------------

- https://en.wikipedia.org/wiki/Sandy_Bridge
- http://www.tomshardware.com/reviews/sandy-bridge-core-i7-2600k-core-i5-2500k,2833.html
- http://www.tomshardware.com/reviews/xeon-e5-2687w-benchmark-review,3149.html

- AVX (128 bits)

- Cache: (Intel manual 2.4.5)
    L2 is *non*-inclusive
	L3 is inclusive (contains all data in L1 and L2)




Nehalem (arch) / Westmere (shrink): Intel x86-64 Core Gen1
----------------------------------------
