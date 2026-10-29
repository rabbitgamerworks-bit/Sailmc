# SailMC, qmake build (sailfish/sailmc.pro) - used when the project is opened in the Sailfish IDE.
# The CMake based package spec is rpm/sailmc.spec in the repository root.
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
Requires:   sailfish-components-pickers-qt5
BuildRequires:  zlib-devel
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Xml)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  pkgconfig(Qt5Svg)

# bundled game natives live in /usr/share/sailmc/renderers
%define __provides_exclude_from ^%{_datadir}/%{name}/.*$
%define __requires_exclude ^(libatomic\\.so.*|libglfw.*|liblwjgl.*|libopenal.*|libGL\\.so.*)$

%description
SailMC is a native Sailfish OS port of the MultiMC Minecraft launcher.
It manages Minecraft instances, Microsoft accounts, mod loaders and Java
runtimes with a Silica interface inspired by MojoLauncher.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
make %{?_smp_mflags}

%install
%qmake5_install

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
