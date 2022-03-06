Name:       capd
Version:    v22.2.17
Release:    1
Summary:    Most simple RPM package
License:    GPL
BuildRoot:  %{_tmppath}/%{name}-%{version}-build.
BuildRequires:  systemd
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
mkdir -p %{buildroot}/%{_unitdir}
cp debian/capd.service %{buildroot}/%{_unitdir}/capd.service

%clean
rm -rf build %{buildroot}

%files
%{_unitdir}/capd.service
/usr/bin/capd
/usr/bin/openClose.sh

%changelog
# TODO:

%post
%systemd_post capd.service

%preun
%systemd_preun capd.service

%postun
%systemd_postun_with_restart capd.service
