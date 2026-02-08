
TARGET=680x
TARGET_DEF=TARGET_680X

OBJPATH=../../obj/$(TARGET)
INCADD=$(OBJPATH)/680x/ins.h $(OBJPATH)/680x/gen.h
YACCADD=$(OBJPATH)/680x/mnem.lex

include ../Makefile.template

$(OBJPATH)/680x/ins.h $(OBJPATH)/680x/gen.h $(OBJPATH)/680x/mnem.lex &: $(SRCPATH)/680x/6809.dat
	mkdir -p $(OBJPATH)/680x
	python3 $(SRCPATH)/680x/6809.py $^ -o $(OBJPATH)/680x

