# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/GP/config
IPLUG2_ROOT = ../../third_party/iPlug2OOS/iPlug2

include ../../third_party/iPlug2OOS/iPlug2/common-web.mk

SRC += $(PROJECT_ROOT)/GP.cpp

# WAM_SRC +=

# WAM_CFLAGS +=

WEB_CFLAGS += -DNO_IGRAPHICS -DSAMPLE_TYPE_FLOAT

WAM_LDFLAGS += -O3 -s EXPORT_NAME="'ModuleFactory'" -s ASSERTIONS=0

WEB_LDFLAGS += -O3 -s ASSERTIONS=0
