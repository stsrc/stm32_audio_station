TEMPLATEROOT = ..

# compilation flags for gdb

CFLAGS  = -O0 -g3
ASFLAGS = -g3

# object files

OBJS= main.o DMA.o SPI.o tm_fonts.o tm_ili9341.o xpt2046.o $(STARTUP) $(FreeRTOS_obj)
OBJS+=fatfs/diskio.o fatfs/fatfs_sd.o fatfs/ff.o
OBJS+=I2C.o cs43l22.o RCC.o 

# include common make file

include $(TEMPLATEROOT)/Makefile.common


