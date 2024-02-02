CC = gcc

SRCDIR = src
COMMON_SRC = $(wildcard $(SRCDIR)/*.c)
OBJDIR = obj
COMMON_OBJ = $(COMMON_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
INCLUDEDIR = include
COMMON_INCLUDE = $(wildcard $(INCLUDEDIR)/*.h)
CFLAGS = -Wall -Werror -pthread -std=gnu17

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(COMMON_INCLUDE)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(INCLUDEDIR)


SERVER_SRCDIR = server/$(SRCDIR)
SERVER_OBJDIR = server/$(OBJDIR)
SERVER_INCLUDEDIR = $(INCLUDEDIR) server/$(INCLUDEDIR)
SERVER_SRC = $(wildcard $(SERVER_SRCDIR)/*.c)
SERVER_OBJ = $(SERVER_SRC:$(SERVER_SRCDIR)/%.c=$(SERVER_OBJDIR)/%.o)
SERVER_INCLUDE = $(COMMON_INCLUDE) $(wildcard $(SERVER_INCLUDEDIR)/*.h)

$(SERVER_OBJDIR)/%.o: $(SERVER_SRCDIR)/%.c $(SERVER_INCLUDE)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) $(SERVER_INCLUDEDIR:%=-I%)

bin/server: $(SERVER_OBJ) $(COMMON_OBJ)
	@mkdir -p $(@D)
	$(CC) -o $@ $^ $(CFLAGS)


CLIENT_SRCDIR = client/$(SRCDIR)
CLIENT_OBJDIR = client/$(OBJDIR)
CLIENT_INCLUDEDIR = $(INCLUDEDIR) client/$(INCLUDEDIR)
CLIENT_SRC = $(wildcard $(CLIENT_SRCDIR)/*.c)
CLIENT_OBJ = $(CLIENT_SRC:$(CLIENT_SRCDIR)/%.c=$(CLIENT_OBJDIR)/%.o)
CLIENT_INCLUDE = $(COMMON_INCLUDE) $(wildcard $(CLIENT_INCLUDEDIR)/*.h)

$(CLIENT_OBJDIR)/%.o: $(CLIENT_SRCDIR)/%.c $(CLIENT_INCLUDE)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) $(CLIENT_INCLUDEDIR:%=-I%)

bin/client: $(CLIENT_OBJ) $(COMMON_OBJ)
	@mkdir -p $(@D)
	$(CC) -o $@ $^ $(CFLAGS)


all: bin/server bin/client

test: bin/server bin/client
	python ./test.py

clean:
	rm -rf obj server/obj client/obj bin trace

