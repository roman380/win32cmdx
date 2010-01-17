# Makefile - for zipdump, clip
#
# Project Home: http://code.google.com/p/win32cmdx/
# Code license: New BSD License
#-------------------------------------------------------------------------
# MACROS
#
SOLUTION=zipdump.sln clip.sln
TARGET=Release\zipdump.exe Release\clip.exe
MANUAL=docs\zipdump-manual.html docs\clip-manual.html
DOXYINDEX=html\index.html
SRC=Makefile *.sln *.vcproj *.vsprops src/*

#-------------------------------------------------------------------------
# MAIN TARGET
#
all:	build

rel:	rebuild build.man zip

#-------------------------------------------------------------------------
# COMMANDS
#
cleanall: clean
	-del *.zip *.ncb *.user *.dat *.cache *.bak *.tmp $$* *.usage *.example
	-del src\*.aps
	-del /q html\*.* Release\*.* Debug\*.*

clean: $(SOLUTION)
	!vcbuild /clean /nologo $**
	del /s *.bak $$*

rebuild: $(SOLUTION)
	!vcbuild /rebuild /nologo $**

build: $(TARGET)

build.man: $(MANUAL)

zip:
	svn status
	del win32cmdx-???.zip
	zip win32cmdx-src.zip $(SRC) Doxyfile *.pl test/* -x *.aps
	zip win32dmdx-exe.zip -j $(TARGET) $(MANUAL) html/*.css

install: $(TARGET)
	!copy $** \home\bin

man: $(MANUAL)
	!start $**

doxy: $(DOXYINDEX)
	start $**

verup:
	svn up
	perl -i.bak version-up.pl src/zipdump.cpp src/zipdump.*
	perl -i.bak version-up.pl src/clip.cpp src/clip.*

#.........................................................................
# BUILD
#
$(TARGET): $(SRC)
	vcbuild /nologo $(@B).sln "Release|Win32"

#.........................................................................
# DOCUMENT
#
USAGE  =zipdump.usage clip.usage
EXAMPLE=zipdump.example
$(DOXYINDEX): src/*.cpp Doxyfile $(USAGE) $(EXAMPLE)
	doxygen

$(USAGE): Release\$*.exe Makefile
	-Release\$*.exe -h 2>$@

zipdump.example: Release\$*.exe Makefile
	del test.zip
	zip test.zip *.pl
	-echo zipdump -s test.zip >$@
	-$*.exe -s test.zip >>$@

$(MANUAL): $(DOXYINDEX) Makefile
	copy html\*.css docs
	!perl -n << html\$(@F) >$@
		next if /^<div class="navigation"/.../^<\/div>/;		# navi-bar ‚ğœ‹‚·‚é.
		s/<img class="footer" src="doxygen.png".+\/>/doxygen/;	# footer ‚Ì doxygenƒƒS‚ğtext‚É’u‚«Š·‚¦‚é.
		print;
<<

#.........................................................................
# TEST
#
testrun: sorry_none.test

# Makefile - end
