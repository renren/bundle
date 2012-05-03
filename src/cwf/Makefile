
.PHONY: 

all: cwf libfcgi/libfcgi.a reply_test

ICEROOT=$(ICE_HOME)
MYSQLINC=-I/usr/include/mysql++/ -I/usr/include/mysql/
MYSQLLIB=-lmysqlpp -lmysqlclient

CXXFLAGS=-Wall -I.. -I../third-party/google-ctemplate/src -DPOSIX -g -O2 -I$(ICEROOT)/include \
 $(MYSQLINC) -DBOOST_TEST_MAIN -DBOOST_TEST_DYN_LINK

libfcgi/libfcgi.a:
	make -C ./libfcgi


DEP=../base/libbase.a ../third-party/google-ctemplate/.libs/libctemplate.a libfcgi/libfcgi.a \
../feed/libfeedslice.a ../oldslice/liboldsliceslice.a -ltcmalloc
OBJS = frame.o main.o stream.o http.o connect.o dynalib.o \
  replyaction.o site_xiaonei.o
DEPICE = -L$(ICEROOT)/lib -lIce -lIceUtil -lIceBox
# = dynalib.o
SLICEDEP = ../account/libaccountslice.a

cwf: $(OBJS)
	g++ -o cwf $(OBJS) $(DEP) $(SLICEDEP) $(DEPICE) -lboost_system -lboost_thread -lpthread \
 $(MYSQLLIB)

TESTOBJS = frame.o stream.o http.o connect.o dynalib.o \
  replyaction.o site_xiaonei.o reply_test.o

reply_test: $(TESTOBJS)
	g++ -o reply_test $(TESTOBJS) $(DEP) $(SLICEDEP) $(DEPICE) -lboost_system -lboost_thread -lpthread \
 $(MYSQLLIB) -lboost_unit_test_framework


clean:
	rm *.o cwf -f