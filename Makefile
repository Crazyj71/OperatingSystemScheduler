C=gcc
CFLAGS= -Wall -std=c99
SOURCE1=oss.c
SOURCE2=user.c
TARGET1=oss
TARGET2=user
TARGETS= oss user

all:
	$(C) $(CFLAGS) $(SOURCE1) -o $(TARGET1)
	$(C) $(CFLAGS) $(SOURCE2) -o $(TARGET2)

clean:
	rm $(TARGETS)

