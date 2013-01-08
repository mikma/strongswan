TARGET = android-14

PROJECT_PATH = $(abs_builddir)

INKSCAPE_FLAGS = --export-background-opacity=0

ICONS = \
	res/drawable-ldpi/ic_launcher.png \
	res/drawable-mdpi/ic_launcher.png \
	res/drawable-hdpi/ic_launcher.png \
	res/drawable-xhdpi/ic_launcher.png \
	google-play/icon.png

all: build.xml

clean:
	$(RM) -f build.xml

maintainer-clean:
	$(RM) $(ICONS)

build.xml:
	ant/genbuild.sh $(PROJECT_PATH) $(PACKAGE_NAME) $(TARGET)

native:
	ndk-build GNUMAKE=$(MAKE) $(MFLAGS)

debug: native build.xml
	ant debug

install-debug: debug
	adb install -r bin/$(PACKAGE_NAME)-debug.apk

release: native build.xml
	ant release

sign: 
	tools/sign-release $(PACKAGE_NAME)

install-release: release sign
	adb install -r bin/$(PACKAGE_NAME)-release.apk

#
# Icons built from icon.svg
#
icons: $(ICONS)

google-play/icon.png: icon.svg
	inkscape $< --export-png=$@ -w512 -h512 $(INKSCAPE_FLAGS) > /dev/null

res/drawable-ldpi/ic_launcher.png: icon.svg
	test -d res/drawable-ldpi || mkdir res/drawable-ldpi
	inkscape $< --export-png=$@ -d 67 $(INKSCAPE_FLAGS) > /dev/null

res/drawable-mdpi/ic_launcher.png: icon.svg
	inkscape $< --export-png=$@ -d 90 $(INKSCAPE_FLAGS) > /dev/null

res/drawable-hdpi/ic_launcher.png: icon.svg
	inkscape $< --export-png=$@ -d 135 $(INKSCAPE_FLAGS) > /dev/null

res/drawable-xhdpi/ic_launcher.png: icon.svg
	inkscape $< --export-png=$@ -d 180 $(INKSCAPE_FLAGS) > /dev/null
