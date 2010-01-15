# Makefile - for zipdump
#
# Project Home: http://code.google.com/p/win32cmdx/
# Code license: New BSD License
#-------------------------------------------------------------------------
# MACROS
#
TARGET=Release\zipdump.exe
MANUAL=html\zipdump-manual.html
DOXYINDEX=html\index.html
SRC=Makefile *.sln *.vcproj *.vsprops src/*

#-------------------------------------------------------------------------
# MAIN TARGET
#
all:	build

rel:	rebuild $(MANUAL) zip

#-------------------------------------------------------------------------
# COMMANDS
#
cleanall: clean
	-del *.zip *.ncb *.user *.dat *.cache *.bak *.tmp $$*
	-del src\*.aps src\*.bak
	-del /q html\*.* Release\*.* Debug\*.*

clean:
	vcbuild /clean   zipdump.sln

rebuild:
	vcbuild /rebuild zipdump.sln

build: $(TARGET)

zip:
	svn status
	zip win32cmdx-src.zip $(SRC) Doxyfile *.pl test/* -x *.aps
	zip win32dmdx-exe.zip -j Release/*.exe $(MANUAL) html/*.css

install: $(TARGET) $(MANUAL)
	copy $(TARGET) \home\bin
	copy $(MANUAL)  docs
	copy html\*.css docs

man: $(MANUAL)
	start $**

doxy: $(DOXYINDEX)
	start $**

verup:
	svn up
	perl -i.bak version-up.pl src/zipdump.cpp src/zipdump.*

#.........................................................................
# BUILD
#
$(TARGET): $(SRC)
	vcbuild zipdump.sln

#.........................................................................
# DOCUMENT
#
$(DOXYINDEX): src/*.cpp Doxyfile usage.tmp example.tmp
	doxygen

usage.tmp: $(TARGET) Makefile
	-$(TARGET) -h 2>$@

example.tmp: $(TARGET) Makefile
	zip test.zip *.pl
	-echo zipdump -s test.zip >$@
	-$(TARGET) -s test.zip >>$@

$(MANUAL): $(DOXYINDEX) Makefile
	perl -n << html\main.html >$@
		next if /^<div class="navigation"/.../^<\/div>/;		# navi-bar ‚ğœ‹‚·‚é.
		s/<img class="footer" src="doxygen.png".+\/>/doxygen/;	# footer ‚Ì doxygenƒƒS‚ğtext‚É’u‚«Š·‚¦‚é.
		print;
<<

#.........................................................................
# TEST
#
testrun: sorry_none.test

# Makefile - end
