bin_PROGRAMS = kxsldbg xsldbg

kxsldbg_SOURCES = kxsldbg.cpp main.cpp kxsldbgif.skel
noinst_HEADERS = kxsldbg.h
kxsldbg_DEPENDENCIES  = ./kxsldbgpart/libqtnotfier/libqtnotfier.la ./kxsldbgpart/libkxsldbgcommon.la ./kxsldbgpart/libxsldbg/libxsldbg.la 
kxsldbg_LDADD   = ./kxsldbgpart/libqtnotfier/libqtnotfier.la ./kxsldbgpart/libkxsldbgcommon.la ./kxsldbgpart/libxsldbg/libxsldbg.la  $(LIB_KDEUI) $(LIBXSLT_LIBS) $(LIBXML_LIBS) $(LIB_KPARTS)

SUBDIRS = kxsldbgpart

xdg_apps_DATA = kxsldbg.desktop

# Disable building xsldbg application as documentation for it is not ready
#
xsldbg_SOURCES = xsldbgmain.cpp

xsldbg_CFLAGS = $(XSLDBG_CFLAGS) $(all_includes)

xsldbg_DEPENDENCIES  = kxsldbgpart/libxsldbg/libxsldbg.la

# use of configure defined libraries
xsldbg_LDADD = kxsldbgpart/libxsldbg/libxsldbg.la $(LIBXSLT_LIBS) $(LIBXML_LIBS) $(LIB_KDEUI)
#
# the library search path.
xsldbg_LDFLAGS = $(all_libraries) $(XSLDBG_EXTRA_LIBS)

# These paths are KDE specific. Use them:
# kde_appsdir         Where your application's menu entry (.desktop) should go to.
# kde_icondir         Where your icon should go to - better use KDE_ICON.
# kde_sounddir        Where your sounds should go to.
# kde_htmldir         Where your docs should go to. (contains lang subdirs)
# kde_datadir         Where you install application data. (Use a subdir)
# kde_locale          Where translation files should go to. (contains lang subdirs)
# kde_cgidir          Where cgi-bin executables should go to.
# kde_confdir         Where config files should go to (system-wide ones with default values).
# kde_mimedir         Where mimetypes .desktop files should go to.
# kde_servicesdir     Where services .desktop files should go to.
# kde_servicetypesdir Where servicetypes .desktop files should go to.
# kde_toolbardir      Where general toolbar icons should go to (deprecated, use KDE_ICON).
# kde_wallpaperdir    Where general wallpapers should go to.
# kde_templatesdir    Where templates for the "New" menu (Konqueror/KDesktop) should go to.
# kde_bindir          Where executables should go to. Use bin_PROGRAMS or bin_SCRIPTS.
# kde_libdir          Where shared libraries should go to. Use lib_LTLIBRARIES.
# kde_moduledir       Where modules (e.g. parts) should go to. Use kde_module_LTLIBRARIES.
# kde_styledir        Where Qt/KDE widget styles should go to (new in KDE 3).
# kde_designerdir     Where Qt Designer plugins should go to (new in KDE 3).

# set the include path for X, qt and KDE
INCLUDES= $(LIBXML_CFLAGS)  $(LIBXSLT_CFLAGS) $(all_includes)

METASOURCES = AUTO

# the application source, library search path, and link libraries
kxsldbg_LDFLAGS = $(KDE_RPATH) $(all_libraries)

# this is where the shell's XML-GUI resource file goes
shellrcdir   = $(kde_datadir)/kxsldbg
shellrc_DATA = kxsldbg_shell.rc

messages: rc.cpp
	LIST=`find . -name \*.ui -o -name \*.rc`; \
	$(EXTRACTRC) $$LIST >> rc.cpp ;\
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/kxsldbg.pot; \
	fi

