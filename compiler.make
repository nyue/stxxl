## compiler versions

GCCVER =

MAKE = gmake

LARGE_FILE = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
NO_OVR = #  -DNO_OVERLAPPING
PROF = # -DSTXXL_THREAD_PROFILING -pg
OPT = -O9
DEBUG = -DNDEBUG #-g # -DNDEBUG # -g 

GCC=g++-3.2 -w $(LARGE_FILE) $(NO_OVR) $(OPT) $(PROF) $(DEBUG)

STXXL_OBJ = ../io/diskqueue.o -lpthread
