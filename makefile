dirPath="$(shell xcodebuild -showBuildSettings | grep -m 1 "TEMP_ROOT" | grep -oEi '\/.*')"

all:
	xcodebuild -project "Hash Table.xcodeproj" -scheme "Tests With DirtyAllocation" OBJC_DISABLE_GC=YES CONFIGURATION_BUILD_DIR="$(CURDIR)/Build"
	open "$(dirPath)/Hash Table.build/Debug/Tests.build/Objects-normal/x86_64"

test:
	env OBJC_DISABLE_GC=YES /Applications/Xcode.app/Contents/Developer/Tools/otest --SenTest All "$(CURDIR)/Build/Tests.octest"
	env OBJC_DISABLE_GC=YES DYLD_FORCE_FLAT_NAMESPACE=1 DYLD_INSERT_LIBRARIES="$(CURDIR)/Build/libDirtyAllocation.dylib" /Applications/Xcode.app/Contents/Developer/Tools/otest --SenTest All "$(CURDIR)/Build/Tests.octest"

clean:
	rm -r "$(CURDIR)/Build"