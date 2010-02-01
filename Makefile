# Makefile - for zipdump, clip, renamex
#
# Project Home: http://code.google.com/p/win32cmdx/
# Code license: New BSD License
#-------------------------------------------------------------------------
# MACROS
#
TARGET  =zipdump.exe clip.exe renamex.exe
SOLUTION=zipdump.sln clip.sln renamex.sln
MANUAL  =docs\zipdump-manual.html docs\clip-manual.html docs\renamex-manual.html
DOXYINDEX=html\index.html
SRC=Makefile *.sln *.vcproj *.vsprops src/*

#-------------------------------------------------------------------------
# MAIN TARGET
#
all:	build

rel:	rebuild man zip

#-------------------------------------------------------------------------
# COMMANDS
#
cleanall: clean
	-del $(TARGET) *.zip *.ncb *.user *.dat *.cache *.bak *.tmp $$* *.usage *.example
	-del src\*.aps
	-del /q html\*.* Release\*.* Debug\*.*

clean: $(SOLUTION)
	!vcbuild /clean /nologo $**
	del /s *.bak $$*

rebuild: $(SOLUTION)
	!vcbuild /rebuild /nologo $**

build: $(TARGET)

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

zipdump.verup clip.verup renamex.verup:
	svn up
	perl -i.bak version-up.pl src/$*.cpp src/$*.*

#.........................................................................
# BUILD
#
$(TARGET): $(SRC)
	vcbuild /nologo $(@B).sln "Release|Win32"
	copy Release\$@ $@
	touch $@

#.........................................................................
# DOCUMENT
#
USAGE  =zipdump.usage clip.usage renamex.usage
EXAMPLE=zipdump.example
$(DOXYINDEX): src/*.cpp Doxyfile $(USAGE) $(EXAMPLE)
	doxygen

$(USAGE): $*.exe Makefile
	-$*.exe -h 2>$@

zipdump.example: zipdump.exe Makefile
	del test.zip
	zip test.zip *.pl
	-echo zipdump -s test.zip >$@
	-zipdump.exe -s test.zip >>$@

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
