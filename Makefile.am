AM_CPPFLAGS += $(THREADFLAGS) $(BOOST_CPPFLAGS) -Icrow/include -Wno-strict-aliasing -Iext/luawrapper/include/ \
	-Iext/simplesocket -I/usr/include/lua5.2/  -Iext/CLI11/include/ -Iext/json/src/

AM_LDFLAGS = $(THREADFLAGS)  \
	$(BOOST_LDFLAGS)

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = \
	ws kolctl


ws_SOURCES =  \
	ws.cc http.cc kolmoconf.cc kolmoconf.hh ext/simplesocket/comboaddress.cc kolmoweb.cc

ws_LDFLAGS = \
	$(AM_LDFLAGS) $(BOOST_SYSTEM_LIBS) -lpthread -llua5.2 -lssl -lcrypto

ws_LDADD = 

kolctl_SOURCES =  \
	kolctl.cc http.cc kolmoconf.cc kolmoconf.hh ext/simplesocket/comboaddress.cc \
	minicurl.cc minicurl.hh kolmoweb.cc

kolctl_LDFLAGS = \
	$(AM_LDFLAGS) $(BOOST_SYSTEM_LIBS) -lpthread -llua5.2 -lssl -lcrypto -lcurl

kolctl_LDADD = 

