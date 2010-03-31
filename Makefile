# Makefile - for zipdump, clip, renamex, delx, dirdiff
#
# Project Home: http://code.google.com/p/win32cmdx/
# Code license: New BSD License
#-------------------------------------------------------------------------
# MACROS
#
TARGET  =zipdump.exe clip.exe renamex.exe delx.exe dirdiff.exe
MANUAL  =$(TARGET:.exe=-manual.html)
SOLUTION=$(TARGET:.exe=.sln)
VERUP   =$(TARGET:.exe=.verup)
DOXYINDEX=html\index.html
APPSRC  =myconfig.vsprops src/*.h src/mylib/*	$*.sln $*.vcproj src/$*.cpp src/$*.rc
ALLSRC  =myconfig.vsprops src/*.h src/mylib/*	*.sln  *.vcproj  src/*.cpp  src/*.rc

#-------------------------------------------------------------------------
# MAIN TARGET
#
all:	build

rel:	rebuild man zip

#-------------------------------------------------------------------------
# COMMANDS
#
cleanall: clean
	-del $(TARGET) $(MANUAL) *.zip *.ncb *.user *.dat *.cache *.bak *.tmp $$* *.usage *.example
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
	zip win32cmdx-src.zip $(ALLSRC) Makefile Doxyfile *.pl test/* -x *.aps
	zip win32dmdx-exe.zip -j $(TARGET) $(MANUAL) html/*.css

install: $(TARGET)
	!copy $** \home\bin

man: $(MANUAL)
	!copy $** docs
	copy html\*.css docs

doxy: $(DOXYINDEX)
	start $**

#.........................................................................
# BUILD
#
$(TARGET:.exe=): $*.exe

$(TARGET): $(APPSRC)
	vcbuild /nologo $(@B).sln "Release|Win32"
	copy Release\$@ $@
	touch $@

$(VERUP):
	svn up
	perl -i.bak version-up.pl src/$*.cpp src/$*.*

#.........................................................................
# DOCUMENT
#
USAGE  =zipdump.usage clip.usage renamex.usage
EXAMPLE=zipdump.example

$(DOXYINDEX): Doxyfile src/*.h src/*.cpp src/mylib/*.cpp $(USAGE) $(EXAMPLE)
	doxygen

$(USAGE): $*.exe Makefile
	-$*.exe -h 2>$@

zipdump.example: zipdump.exe Makefile
	del test.zip
	zip test.zip *.pl
	-echo zipdump -s test.zip >$@
	-zipdump.exe -s test.zip >>$@

$(MANUAL): $(DOXYINDEX) Makefile
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
