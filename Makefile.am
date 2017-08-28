AM_CPPFLAGS += $(THREADFLAGS) $(BOOST_CPPFLAGS) -Icrow/include -Wstrict-aliasing

AM_LDFLAGS = $(THREADFLAGS)  \
	$(BOOST_LDFLAGS)

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = \
	ws


ws_SOURCES =  \
	ws.cc 

ws_LDFLAGS = \
	$(AM_LDFLAGS) $(BOOST_SYSTEM_LIBS) -lpthread

ws_LDADD = 

