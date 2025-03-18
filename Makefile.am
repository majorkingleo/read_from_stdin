bin_PROGRAMS=\
	read_from_stdin
		
read_from_stdin_SOURCES=\
		src/read_from_stdin.cc 
		
AM_CPPFLAGS = \
	-std=gnu++20 \
	-D_GLIBCXX_USE_CXX11_ABI=1 \
	-D_XOPEN_SOURCE=700 # for cyggwin fileno() \
	-fstack-protector

LIBS=
    
AM_LDFLAGS=
    
if MINGW
#AM_LDFLAGS += -mwindows
LIBS += -lws2_32
# with -std=c++11 WIN32 is disabled (_WIN32) would be the valid one, so we define it in mingw.
AM_CPPFLAGS += -DWIN32
else
#AM_LDFLAGS += -lX11
endif


    

