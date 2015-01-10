### This Makefile was written for nmake. ###
TERMUTIL_DIR        = termutil
TERMUTIL_REPOSITORY = https://github.com/koturn/$(TERMUTIL_DIR).git
TERMUTIL_LIBS_DIR   = $(TERMUTIL_DIR)/lib
TERMUTIL_LIB        = termutil$(DBG_SUFFIX).lib
TERMUTIL_LDLIBS     = /LIBPATH:$(TERMUTIL_LIBS_DIR) $(TERMUTIL_LIB)
TERMUTIL_INCS       = /Itermutil/include/

!if "$(CRTDLL)" == "true"
CRTLIB = /MD$(DBG_SUFFIX)
!else
CRTLIB = /MT$(DBG_SUFFIX)
!endif

!if "$(DEBUG)" == "true"
DBG_SUFFIX  = d
COPTFLAGS   = /Od /GS /Zi $(CRTLIB)
LDOPTFLAGS  = /Od /GS /Zi $(CRTLIB)
MSVC_MACROS = /D_DEBUG /D_CRTDBG_MAP_ALLOC /D_USE_MATH_DEFINES

!else
COPTFLAGS   = /Ox /GL $(CRTLIB)
LDOPTFLAGS  = /Ox /GL $(CRTLIB)
MSVC_MACROS = /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS \
              /D_USE_MATH_DEFINES
!endif


CC       = cl
RM       = del /F
MAKE     = $(MAKE) /nologo
GIT      = git
INCS     = $(TERMUTIL_INCS)
MACROS   = $(MSVC_MACROS)
CFLAGS   = /nologo $(COPTFLAGS) /W4 /c $(INCS) $(MACROS)
LDFLAGS  = /nologo $(LDOPTFLAGS)
LDLIBS   = /link $(TERMUTIL_LDLIBS)
TARGET   = tetris.exe
OBJ      = $(TARGET:.exe=.obj)
SRC      = $(OBJ:.obj=.c)
MAKEFILE = msvc.mk


.SUFFIXES: .c .obj .exe
.obj.exe:
	$(CC) $(LDFLAGS) $** /Fe$@ $(LDLIBS)
.c.obj:
	$(CC) $(CFLAGS) $** /Fo$@


all: $(TERMUTIL_LIBS_DIR)/$(TERMUTIL_LIB) $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $** /Fe$@ $(LDLIBS)

$(OBJ): $(SRC)

$(TERMUTIL_LIBS_DIR)/$(TERMUTIL_LIB):
	@if not exist $(@D)/NUL \
		$(GIT) clone $(TERMUTIL_REPOSITORY)
	cd $(TERMUTIL_DIR)  &  $(MAKE) /f $(MAKEFILE)  &  cd $(MAKEDIR)


clean:
	$(RM) $(TARGET) $(OBJ)
cleanobj:
	$(RM) $(OBJ)
