1i %global mingw_build_win32 0
s/^Release:.*/Release:     99%{?dist}/
# Enable schannel
s/^MINGW32_CONFIGURE_ARGS.*//
s/^MINGW64_CONFIGURE_ARGS.*//
s/^MINGW_CONFIGURE_ARGS.*/MINGW_CONFIGURE_ARGS="--with-schannel --enable-ipv6 --without-random"/
# Remove unneeded 64 bit dependencies
s/^BuildRequires:.*mingw64-libidn2//
s/^BuildRequires:.*mingw64-libssh2//
s/^BuildRequires:.*mingw64-openssl//
# Remove 32 bit since we're only building 64 bit
/^BuildRequires:.*mingw32/d
/^%package -n mingw32-curl/,/^Static version of the MinGW Windows Curl library/d
# Remove 64 bit static, disable debug package
/^%package -n mingw64-curl-static/,/^%{?mingw_debug_package}/c\%global debug_package %{nil}
# Remove 32 bit package files
/^%files -n mingw32-curl/,/^# Win64/c\# Win64
# Remove 64 bit static files
/^%files -n mingw64-curl-static/,/^%changelog/c\%changelog
# Misc remnants
/MINGW_BUILDDIR_SUFFIX=_static/d
/mv.*mingw32/d
/rm.*mingw32/d
/mv.*static/d
/rm.*static/d
