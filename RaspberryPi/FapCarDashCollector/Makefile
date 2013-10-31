

OBJS =		RedisClient.o

LIBS =		libhiredis.a

TARGET =	RedisClient

$(TARGET):	$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
