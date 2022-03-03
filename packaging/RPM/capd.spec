Name:       capd
Version:    v22.2.17
Release:    1
Summary:    Most simple RPM package
License:    GPL
BuildRoot:  %{_tmppath}/%{name}-%{version}-build.
Source0:    capd-%{version}.tar.xz

%description
Runs the capd service.

%prep
%setup -q

%build
meson build -Dc_std=c11 --prefix=/usr
meson compile -C build

%install
rm -rf %{buildroot}
meson install -C build --destdir %{buildroot}

%clean
rm -rf build %{buildroot}

%files
/usr/bin/capd

%changelog
# TODO:
