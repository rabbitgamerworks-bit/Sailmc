# Native Sailfish OS fork of MultiMC. Build with the Sailfish SDK:
#   sailfish/build-jars.sh            (on the host, needs a JDK)
#   mb2 -t SailfishOS-5.1.0.11-aarch64 build
Name:       sailmc
Summary:    Minecraft launcher for Sailfish OS based on MultiMC
Version:    0.7.0
Release:    1
License:    ASL 2.0
URL:        https://github.com/rabbitgamerworks-bit/Sailmc
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   tar
Requires:   qt5-qtsvg-plugin-imageformat-svg
BuildRequires:  cmake
BuildRequires:  zlib-devel
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Xml)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  pkgconfig(Qt5Test)

# the launcher's own shared libraries live in /usr/share/sailmc/lib
%define __provides_exclude_from ^%{_datadir}/%{name}/.*$
%define __requires_exclude ^(libLauncher_.*|libatomic\\.so.*)$

%description
SailMC is a native Sailfish OS port of the MultiMC Minecraft launcher.
It manages Minecraft instances, Microsoft accounts, mod loaders and Java
runtimes with a Silica interface inspired by MojoLauncher.

%prep
%setup -q -n %{name}-%{version}

%build
mkdir -p build-%{_target_cpu}
cd build-%{_target_cpu}
cmake .. \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_BUILD_TYPE=Release \
    -DLauncher_SAILFISH=ON \
    -DLauncher_GIT_COMMIT=sailfish
make %{?_smp_mflags}

%install
cd build-%{_target_cpu}
make install DESTDIR=%{buildroot}

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
