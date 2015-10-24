Name:       ui-gadget-1
Summary:    UI Gadget Library
Version:    0.1.165
Release:    1
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(post): /usr/bin/api_feature_loader
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(bundle)
BuildRequires:  pkgconfig(pkgmgr-info)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(appsvc)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(dbus-glib-1)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(libprivilege-control)
BuildRequires:  cmake
BuildRequires:  edje-bin
BuildRequires:  hash-signer

%define feature_appfw_process_pool 1

%define appfw_feature_app_control_lite 0
%define appfw_feature_ug_process_pool 0

%description
UI gadget library (development headers)

%package devel
Summary:    Development files for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
%description devel
Development files for %{name}

%package samples
Summary:    Development files for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
%description samples
Development files for %{name}

%package template
Summary:    Development files for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
%description template
Development files for %{name}

# variables
%define _privilege_install_path /usr/share/privilege-control

%prep
%setup -q

%build


%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif
export CFLAGS="$CFLAGS -Wall -Werror -Wno-unused-function -Wno-unused-but-set-variable"
export CFLAGS="$CFLAGS -fPIC -pie"
%if 0%{?feature_appfw_process_pool}
_APPFW_FEATURE_PROCESS_POOL=ON
%endif
%if 0%{?appfw_feature_app_control_lite}
_APPFW_FEATURE_APP_CONTROL_LITE=ON
%endif
%if 0%{?appfw_feature_ug_process_pool}
_APPFW_FEATURE_UG_PROCESS_POOL=ON
%endif

cmake -DCMAKE_INSTALL_PREFIX=/usr \
	-D_APPFW_FEATURE_PROCESS_POOL:BOOL=${_APPFW_FEATURE_PROCESS_POOL} \
	-D_APPFW_FEATURE_APP_CONTROL_LITE:BOOL=${_APPFW_FEATURE_APP_CONTROL_LITE} \
	-D_APPFW_FEATURE_UG_PROCESS_POOL:BOOL=${_APPFW_FEATURE_UG_PROCESS_POOL} \
	.

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
install LICENSE %{buildroot}/usr/share/license/%{name}
cp -rf %{buildroot}/usr/bin/ug-client %{buildroot}/usr/bin/ug-launcher
mkdir -p %{buildroot}/usr/apps/org.tizen.helloworld/bin
cp -rf %{buildroot}/usr/bin/ug-client %{buildroot}/usr/apps/org.tizen.helloworld/bin/helloworld
cp -rf %{buildroot}/usr/bin/ug-client %{buildroot}/usr/apps/org.tizen.helloworld/bin/hello

#Signing
%define tizen_sign_base /usr/apps/org.tizen.helloworld/
%define tizen_sign 1
%define tizen_author_sign 1
%define tizen_dist_sign 1
%define tizen_sign_level platform

%post
/sbin/ldconfig
/usr/bin/api_feature_loader --verbose --dir=%{_privilege_install_path}
mkdir -p /opt/usr/ug/lib/
mkdir -p /usr/ug/lib/
mkdir -p /usr/ug/res/images

%post samples
chsmack -a "org.tizen.helloworld" -e "org.tizen.helloworld" /usr/apps/org.tizen.helloworld/bin/helloworld
/bin/cp /opt/usr/ug/lib/libug-helloUG-efl.so /usr/apps/org.tizen.helloworld/lib/ug/libhelloworld.so
/bin/cp /opt/usr/ug/lib/libug-helloUG-efl.so /usr/apps/org.tizen.helloworld/lib/ug/libhelloUG-efl.so
/bin/cp /opt/usr/ug/lib/libug-helloUG-efl.so /usr/apps/org.tizen.helloworld/lib/libug-helloworld.so
/bin/cp /opt/usr/ug/lib/libug-helloUG-efl.so /usr/apps/org.tizen.helloworld/lib/libug-hello.so
/bin/cp /opt/usr/ug/lib/libug-helloUG-efl.so /usr/ug/lib/libug-hello.so
mv /opt/usr/ug/lib/libug-helloUG-efl.so /opt/usr/ug/lib/libug-helloUG2-efl.so
ln -sf /usr/bin/ug-client /usr/apps/org.tizen.helloworld/bin/helloUG-efl

%postun -p /sbin/ldconfig

%posttrans
#for NAME in `find /opt/usr/apps/*/lib/ug/*`; do cp $NAME /opt/usr/ug/lib/; done
chsmack -a "_" /opt/usr/ug/lib/
chsmack -a "_" /usr/ug/lib/
chsmack -a "_" /usr/ug/res/
chsmack -a "_" /usr/ug/res/images/

%files
%manifest ui-gadget-1.manifest
/etc/smack/accesses.d/ui-gadget-1.efl
%defattr(-,root,root,-)
%{_libdir}/*.so.*
%{_libdir}/lib%{name}-efl-engine.so
%{_bindir}/ug-client
%{_bindir}/ug-launcher
/usr/share/edje/ug-client/*.edj
/usr/share/license/%{name}

%files devel
%defattr(-,root,root,-)
%{_includedir}/ug-1/*.h
%{_libdir}/libui-gadget-1.so
%{_libdir}/pkgconfig/%{name}.pc

%files samples
%manifest samples/helloUG-efl/org.tizen.helloworld.manifest
/etc/smack/accesses2.d/helloug.rule
/opt/usr/ug/lib/libug-helloUG-efl.so*
/usr/share/applications/*.desktop
/usr/share/packages/org.tizen.helloworld.xml
/opt/usr/apps/org.tizen.helloworld/data/ui-gadget_doc.h
/usr/apps/org.tizen.helloworld/lib/ug/*
/usr/apps/org.tizen.helloworld/bin/*
#Signing
/usr/apps/org.tizen.helloworld/author-signature.xml
/usr/apps/org.tizen.helloworld/signature1.xml

%files template
/usr/bin/ug-gen.sh
/usr/share/ug-template/*
