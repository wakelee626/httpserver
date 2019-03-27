bin=httpserver
cc=g++
LDFLAGS=-lpthread

.PHONY:all
all:$(bin) cgi

$(bin):httpserver.cc
	$(cc) -o $@ $^ $(LDFLAGS) -std=c++11

.PHONY:cgi
cgi:
	g++ -o Cal cal.cc

.PHONY:clean
clean:
	rm -f $(bin) testcgi
