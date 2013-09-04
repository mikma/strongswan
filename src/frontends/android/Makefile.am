TARGET = android-14

PROJECT_PATH = $(abs_builddir)

all: build.xml

clean:
	$(RM) -f build.xml

build.xml:
	ant/genbuild.sh $(PROJECT_PATH) $(PACKAGE_NAME) $(TARGET)

native:
	ndk-build GNUMAKE=$(MAKE) $(MFLAGS)

debug: native build.xml
	ant debug

release: native build.xml
	ant release

install-debug: debug
	adb install -r bin/$(PACKAGE_NAME)-debug.apk
