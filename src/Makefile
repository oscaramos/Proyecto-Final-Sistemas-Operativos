SHELL := /bin/bash
all: p1.exe p2.exe p3.exe p4.exe
	@echo "Done!"	

p1.exe: P1.cpp 
	g++ P1.cpp -o p1.exe -std=c++11 -lpthread

p2.exe: P2.cpp
	g++ P2.cpp -o p2.exe -std=c++11

p3.exe: P3.cpp
	g++ P3.cpp -o p3.exe -pthread -std=c++11

p4.exe: P4.cpp
	g++ P4.cpp -o p4.exe -pthread -std=c++11

N = 10# 1 millon
PID = 0
massive_kills: # Ex: make massive_kills N=100 PID=2028
	for i in {1..$(N)}; do kill -2 $(PID); sleep 0.1; done

clear:
	rm p1.exe p2.exe p3.exe p4.exe
