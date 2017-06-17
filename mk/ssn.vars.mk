
DPDK_CFLAGS = -I$(RTE_SDK)/$(RTE_TARGET)/include
DPDK_LDFLAGS = -L$(RTE_SDK)/$(RTE_TARGET)/lib -ldpdk -lpthread -ldl -lrt

SSN_CXXFLAGS += $(DPDK_CFLAGS) -std=c++11
SSN_LDFLAGS  += $(DPDK_LDFLAGS) -llthread

SLANKLLIB_PATH = /home/slank/git/slankdev/libslankdev
SSN_CXXFLAGS += -I$(SLANKLLIB_PATH)

SSNLIB_PATH = $(SSN_SDK)/lib/
SSN_CXXFLAGS += -I$(SSNLIB_PATH)
SSN_LDFLAGS  += -L$(SSNLIB_PATH) -lsusanow

