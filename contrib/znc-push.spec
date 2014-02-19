Name:     znc-push
Version:  20130708
Release:  3%{?dist}
Summary:  Push notifications module for ZNC

Group:    System Environment/Daemons
License:  MIT
URL:      https://github.com/jreese/znc-push
Source0:  %{name}-%{version}.tar.gz
BuildRoot:  %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  gcc-c++, znc-devel >= 1.0
Requires:  znc >= 1.0

%description
Push notifications module for ZNC

%prep

%setup -q

%build
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%{__mkdir_p} %{buildroot}/%{_libdir}/znc
%{__mkdir_p} %{buildroot}/%{_defaultdocdir}/%{name}-%{version}
%{__install} -Dp -m0755 push.so %{buildroot}/%{_libdir}/znc/push.so
%{__install} -Dp -m0644 README.md %{buildroot}/%{_defaultdocdir}/%{name}-%{version}/README.md
%{__install} -Dp -m0644 LICENSE %{buildroot}/%{_defaultdocdir}/%{name}-%{version}/LICENSE

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%dir %{_defaultdocdir}/%{name}-%{version}
%attr(0755, root, root) %{_libdir}/znc/push.so
%doc %{_defaultdocdir}/%{name}-%{version}/README.md
%doc %{_defaultdocdir}/%{name}-%{version}/LICENSE

%changelog
* Sat Jul 13 2013 Rene Cunningham <rene@linuxfoundation.org> - 20130708-3
- bumped znc requires to 1.0.

* Mon Jul 08 2013 Rene Cunningham <rene@linuxfoundation.org> - 20130708-2
- Removed use of Makefile.
- Added README & LICENSE.

* Thu Jun 06 2013 Andrew Grimberg <agrimberg@linuxfoundation.org> - 20130606-1
- Initial build
