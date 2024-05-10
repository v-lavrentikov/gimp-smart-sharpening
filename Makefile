CC=gcc
FILE=smart-sharpening
APP_VER=2.10
DST_DIR=/Users/$(USER)/Library/Application\ Support/GIMP/$(APP_VER)/plug-ins/$(FILE)
APP_DIR=/Applications/GIMP.app
APP_RES=$(APP_DIR)/Contents/Resources

all: macos

gimptool:
	@gimptool-2.0 --install $(FILE).c

macos: clean
	@mkdir -p $(DST_DIR)

	@$(CC) -o $(DST_DIR)/$(FILE) $(FILE).c \
		-Wl,-rpath,$(APP_RES) \
		-L$(APP_RES)/lib \
		-L/opt/X11/lib \
		-I/opt/local/include/gimp-2.0 \
		-I/opt/local/include/gegl-0.4 \
		-I/opt/local/include \
		-I/opt/local/include/json-glib-1.0 \
		-I/opt/local/include/babl-0.1 \
		-I/opt/local/include/gtk-2.0 \
		-I/opt/local/lib/gtk-2.0/include \
		-I/opt/local/include/pango-1.0 \
		-I/opt/local/include/gio-unix-2.0 \
		-I/opt/local/include/cairo \
		-I/opt/local/include/atk-1.0 \
		-I/opt/local/include/gdk-pixbuf-2.0 \
		-I/opt/local/include/harfbuzz \
		-I/opt/local/include/fribidi \
		-I/opt/local/include/pixman-1 \
		-I/opt/local/include/glib-2.0 \
		-I/opt/local/lib/glib-2.0/include \
		-I/opt/local/include/freetype2 \
		-I/opt/local/include/libpng16 \
		-lgimpui-2.0.0 \
		-lgimpwidgets-2.0.0 \
		-lgimpmodule-2.0.0 \
		-lgimp-2.0.0 \
		-lgimpmath-2.0.0 \
		-lgimpconfig-2.0.0 \
		-lgimpcolor-2.0.0 \
		-lgimpbase-2.0.0 \
		-lgegl-0.4.0 \
		-lgegl-npd-0.4 \
		-lgmodule-2.0.0 \
		-ljson-glib-1.0.0 \
		-lbabl-0.1.0 \
		-lpangocairo-1.0.0 \
		-lXrender \
		-lXinerama \
		-lXi \
		-lXrandr \
		-lXcursor \
		-lXcomposite \
		-lXdamage \
		-lXfixes \
		-lX11 \
		-lXext \
		-latk-1.0.0 \
		-lcairo \
		-lgdk_pixbuf-2.0.0 \
		-lgio-2.0.0 \
		-lpangoft2-1.0.0 \
		-lpango-1.0.0 \
		-lgobject-2.0.0 \
		-lglib-2.0.0 \
		-lintl.8 \
		-lharfbuzz.0 \
		-lfontconfig.1 \
		-lfreetype.6 \
		-lgdk-quartz-2.0.0 \
		-lgtk-quartz-2.0.0

clean:
	@rm -rf $(DST_DIR)
