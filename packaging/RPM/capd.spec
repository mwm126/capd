Name:       capd
Version:    22.3.3
Release:    1
Summary:    Most simple RPM package
License:    GPL
BuildRoot:  %{_tmppath}/%{name}-%{version}-build.

%description
Runs the capd service.

%prep
# TODO: setup sources

%build
meson build -Dc_std=c11
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
