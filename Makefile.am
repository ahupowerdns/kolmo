AM_CPPFLAGS += $(THREADFLAGS) $(BOOST_CPPFLAGS) -Icrow/include -Wno-strict-aliasing -Iext/luawrapper/include/ \
	-Iext/simplesocket -I/usr/include/lua5.2/ 

AM_LDFLAGS = $(THREADFLAGS)  \
	$(BOOST_LDFLAGS)

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = \
	ws


ws_SOURCES =  \
	ws.cc kolmoconf.cc kolmoconf.hh ext/simplesocket/comboaddress.cc

ws_LDFLAGS = \
	$(AM_LDFLAGS) $(BOOST_SYSTEM_LIBS) -lpthread -llua5.2

ws_LDADD = 

