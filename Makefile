SOURCES=main.c
HEADERS=gpu-collector.h

gpu-collector: $(SOURCES) $(HEADERS)
	gcc $(SOURCES) -o $@ -g
