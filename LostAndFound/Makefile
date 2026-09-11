CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS_SERVER = -pthread

# ---- console app ----
TARGET = lostfound
SRCS = main.cpp Item.cpp User.cpp Claim.cpp LostAndFoundSystem.cpp
OBJS = $(SRCS:.cpp=.o)

# ---- web server (adds server/server_main.cpp, reuses the same
#      Item/User/Claim/LostAndFoundSystem objects as the console app) ----
SERVER_TARGET = lostfound_server
SERVER_ONLY_OBJS = server/server_main.o
SERVER_OBJS = $(SERVER_ONLY_OBJS) Item.o User.o Claim.o LostAndFoundSystem.o

# `make` alone builds only the console app.
all: $(TARGET)

# `make web` (or `make lostfound_server`) builds the browser-based version.
web: $(SERVER_TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJS) $(LDFLAGS_SERVER)

server/server_main.o: server/server_main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(SERVER_ONLY_OBJS) $(TARGET) $(SERVER_TARGET)

.PHONY: all web clean
