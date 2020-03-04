3DODEV	= C:/3DODev/
ARMDEV	= C:/arm251/

# Project specific settings
NAME	= LaunchMe
STACKSIZE = 4096
BANNER	= Banner.bmp
FILESYSTEM	= CD

CC		= $(ARMDEV)bin/armcc
AS 		= $(ARMDEV)bin/armas
LD		= $(ARMDEV)bin/armlink
RM		= $(3DODEV)bin/rm
MODBIN	= $(3DODEV)bin/modbin
MAKEBANNER	= tools/banner/MakeBanner

CCFLAGS = -O2 -Otime -Wd -bi -za1 -d DEBUG=0 -cpu ARM60 $(INCPATH)
ASFLAGS =
INCPATH	= -J$(3DODEV)includes -J$(ARMDEV)Include -I./3DO -I./lame6502 -I. -I./lib

LDFLAGS = -reloc -nodebug -remove -ro-base 0x80 
LIBPATH	= $(3DODEV)libs/
STARTUP = $(LIBPATH)cstartup.o
LIBS 	= $(LIBPATH)exampleslib.lib $(LIBPATH)Lib3DO.lib $(LIBPATH)audio.lib $(LIBPATH)music.lib $(LIBPATH)operamath.lib \
	$(LIBPATH)filesystem.lib $(LIBPATH)graphics.lib $(LIBPATH)input.lib $(LIBPATH)clib.lib
ARMLIB	= $(ARMDEV)lib
ARMINC	= $(ARMDEV)inc

SRC_S		= $(wildcard *.s)
SRC_C		= $(wildcard *.c) $(wildcard lame6502/*.c) $(wildcard 3DO/*.c) $(wildcard lib/*.c)

OBJ	+= $(SRC_S:.s=.o)
OBJ	+= $(SRC_C:.c=.o)

all: $(NAME)

$(NAME): $(OBJ) BannerScreen
	$(LD) -dupok -o $(FILESYSTEM)/$(NAME). $(LDFLAGS) $(STARTUP) $(LIBS) $(OBJ)
	$(MODBIN) $(STACKSIZE) CD/$(NAME)

%.o: %.c
	$(CC) $(INCPATH) $(CCFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(INCPATH) $(ASFLAGS) $< -o $@
	
BannerScreen: $(BANNER)
	$(MAKEBANNER) $(BANNER) $(FILESYSTEM)/BannerScreen

clean:
	$(RM) -f $(OBJ)
	$(RM) -f $(FILESYSTEM)/$(NAME)
	$(RM) -f $(FILESYSTEM)/BannerScreen
