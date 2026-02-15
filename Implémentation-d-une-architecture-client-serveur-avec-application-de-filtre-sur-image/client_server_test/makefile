CC = gcc


CFLAGS = -std=c2x -D_XOPEN_SOURCE=700 -D_POSIX_SOURCE -Wpedantic -Wall -Wextra \
         -Wconversion -Werror -fstack-protector-all -fpie -pie -O2 \
         -D_FORTIFY_SOURCE=2 -MMD \
         -I../include -I. -I../worker -I../image_ops


TARGETS = serv_prog cli_prog


SERVER_SRCS = server.c ../worker/worker.c ../image_ops/image_ops.c
CLIENT_SRCS = client.c

SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

LDFLAGS = -pthread
DEPS = $(SERVER_OBJS:.o=.d) $(CLIENT_OBJS:.o=.d)

all: $(TARGETS)


serv_prog: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


cli_prog: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@echo "Nettoyage en cours..."

	rm -f $(TARGETS)

	rm -f *.o *.d

	rm -f ../worker/*.o ../worker/*.d
	rm -f ../image_ops/*.o ../image_ops/*.d
