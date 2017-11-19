Name:                 testovac
Version:              0.1
Release:              0
Summary:              Testovac monitors entered IpV4/IpV6/host name nodes.
License:              OSS FIT VUT
URL:                  https://github.com/ldrahnik/isa_1_project
Source0:              https://github.com/ldrahnik/isa_1_project/archive/v%{version}.tar.gz
BuildArch:            x86_64

%description
Print informations on standard output when is packet loss
or excedation Round trip time (RTT) over specified value. When is not
specificated protocol and port with options -u and -p is used ICMP echo request/reply.
When is selected protocol UDP packet send random data with size 64B and expect the same
packet with the same content. When is specified port with option -l program listen on
UDP port and on received packets reply the same packets with the same content. Program
handle each of node parallely. When is not used option -r is tested only packets loss but
no RTT. RTT is in this case included only to the summary statistics.

%prep
%setup -q -n isa_1_project-%{version}

%build
make

%install
sudo rm -rf %{buildroot}
sudo make install BUILD_ROOT=%{buildroot} VERSION=%{version}

%clean
sudo rm -rf %{buildroot}

%files
%dir /usr/local/lib/%{name}/
/usr/local/lib/%{name}/%{name}
/usr/local/bin/%{name}
/usr/local/man/man1/%{name}.1
/usr/share/doc/%{name}/manual.pdf
/usr/share/licenses/%{name}/LICENSE

%changelog
* Tue Oct 03 2017 Lukáš Drahník <xdrahn00@stud.fit.vutbr.cz, ldrahnik@gmail.com> - 0.1
  - First rpm draft
