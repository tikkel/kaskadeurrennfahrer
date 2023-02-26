#
# General Compiler Settings
#

CC= g++ -std=c++17
#CC+= -g

# compiler flags
FLAGS= -O3 -march=native -pipe -fpermissive -Winit-self -DGL_GLEXT_PROTOTYPES -std=c++11
CFLAGS= -Wno-conversion-null -Wno-write-strings -ICommon
CFLAGS+= -DSOUND_OPENAL
LDFLAGS= -s

# Raspberry Pi
ifeq ($(RPI),1)
	FLAGS+= -DRPI
	FLAGS+= -DARM
endif

# openPandora
ifeq ($(PANDORA),1)
	FLAGS= -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -march=armv7-a -fsingle-precision-constant -mno-unaligned-access -fdiagnostics-color=auto -O3 -fsigned-char
	FLAGS+= -DPANDORA
	FLAGS+= -DARM
	LDFLAGS= -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
endif

ifeq ($(PYRA),1)
	FLAGS= -mcpu=cortex-a15 -mfpu=neon -mfloat-abi=hard -fsingle-precision-constant
	FLAGS+= -DPYRA
	FLAGS+= -DARM
	LDFLAGS= -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
endif

# ODroid
ifeq ($(ODROID),1)
	FLAGS= -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -fsingle-precision-constant -O3 -fsigned-char
	FLAGS+= -DODROID
	FLAGS+= -DARM
	LDFLAGS= -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard
endif

# ODroid N1
ifeq ($(ODROIDN1),1)
	FLAGS= -mcpu=cortex-a72.cortex-a53 -fsingle-precision-constant -O3 -fsigned-char -ffast-math
	FLAGS+= -DODROID
	FLAGS+= -DARM
	LDFLAGS= -mcpu=cortex-a72.cortex-a53
endif

# PocketCHIP
ifeq ($(CHIP),1)
	FLAGS= -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard -fsingle-precision-constant -O3 -fsigned-char
	FLAGS+= -DCHIP
	FLAGS+= -DARM
	LDFLAGS= -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard
endif

# headers
ifeq ($(OS),Windows_NT)
  # https://github.com/libsdl-org/SDL/releases/tag/release-2.26.2 --> SDL2-devel-2.26.2-VC.zip
  # https://github.com/libsdl-org/SDL_ttf/releases/tag/release-2.20.1 --> SDL2_ttf-devel-2.20.1-VC.zip
  # https://www.openal.org/downloads/ --> OpenAL11CoreSDK.zip
  # https://github.com/g-truc/glm/releases/tag/0.9.9.8 --> glm-0.9.9.8.zip
  # https://glew.sourceforge.net/index.html --> glew-2.1.0-win32.zip
CFLAGS+= -D_REENTRANT -I"C:\SDL2-2.26.2\include" -I"C:\SDL2_ttf-2.20.1\include"
CFLAGS+= -I"C:\Program Files (x86)\OpenAL 1.1 SDK\include"
CFLAGS+= -I"C:\glm-0.9.9.8\glm"
CFLAGS+= -I"C:\glew-2.1.0-win32\glew-2.1.0\include"
#CFLAGS+= -I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um"
#CFLAGS+= -I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared"
else
CFLAGS+= `pkg-config --cflags sdl2 SDL2_ttf gl glew openal`
endif

# libraries
ifeq ($(OS),Windows_NT)
  # copy C:\SDL2-2.26.2\lib\x64\SDL2.dll to %SystemRoot%\system32
LIB+= -L"C:\SDL2-2.26.2\lib\x64" -lSDL2
  # copy C:\SDL2_ttf-2.20.1\lib\x64\SDL2_ttf to %SystemRoot%\system32
LIB+= -L"C:\SDL2_ttf-2.20.1\lib\x64" -lSDL2_ttf
  # copy C:\glew-2.1.0-win32\glew-2.1.0\bin\Release\x64\glew32.dll to %SystemRoot%\system32
LIB+= -L"C:\glew-2.1.0-win32\glew-2.1.0\lib\Release\x64" -lglew32 -lopengl32
  # Laufzeit installieren: C:\Program Files (x86)\OpenAL 1.1 SDK\redist\oalinst.exe
LIB+= -L"C:\Program Files (x86)\OpenAL 1.1 SDK\libs\Win64" -lOpenAL32
else
LIB+= `pkg-config --libs sdl2 SDL2_ttf gl glew openal`
endif

ifeq ($(OS),Windows_NT)
BIN=KaskadeurRennfahrer.exe
else
BIN=kaskadeurrennfahrer
endif

INC=$(wildcard *.h)
SRC=$(wildcard *.cpp)
OBJ=$(patsubst %.cpp,%.o,$(SRC))

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(FLAGS) $(CFLAGS) $(LDFLAGS) $(LIB)

$(OBJ): $(INC)

%.o: %.cpp
	$(CC) -o $@ -c $< $(FLAGS) $(CFLAGS)

%.bc: %.cpp
	$(CC) -o $@ -c $< $(FLAGS) $(CFLAGS)

ifeq ($(OS),Windows_NT)
RM = DEL /Q /F /S /Q 2>NUL
else
RM = rm -rf
endif
clean:
	-$(RM) $(OBJ) $(BIN)

check:
	@echo
	@echo "INC = $(INC)"
	@echo
	@echo "SRC = $(SRC)"
	@echo
	@echo "OBJ = $(OBJ)"
	@echo
	@echo "RPI = $(RPI)"
	@echo "PANDORA = $(PANDORA)"
	@echo "PYRA = $(PYRA)"
	@echo "ODROID = $(ODROID)"
	@echo "ODROIDN1 = $(ODROIDN1)"
	@echo "CHIP = $(CHIP)"
	@echo
	@echo "CC = $(CC)"
	@echo "BIN = $(BIN)"
	@echo "FLAGS = $(FLAGS)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "LIB = $(LIB)"
	@echo
