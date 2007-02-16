#*******************************************************************************
# Copyright (c) 1995, 2004 IBM Corporation. All rights reserved.
# Copyright (c) 2005-2006 Rexx Language Association. All rights reserved.
#
# This program and the accompanying materials are made available under
# the terms of the Common Public License v1.0 which accompanies this
# distribution. A copy is also available at the following address:
# http://www.oorexx.org/license.html
#
# Redistribution and use in source and binary forms, with or
# without modification, are permitted provided that the following
# conditions are met:
#
# Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the distribution.
#
# Neither the name of Rexx Language Association nor the names
# of its contributors may be used to endorse or promote products
# derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#******************************************************************************

# The following unconditionally overrides the default prefix with our own.
# This will make us LSB compliant as far as where we install stuff when we are
# not an official part of a Linux distribution.
# If this RPM is for a Linux distribution you should replace "/opt/ooRexx"
# with "/"
%undefine _prefix
%define _prefix /opt/ooRexx

# Specify the package version
%define orx_major 3
%define orx_minor 1
%define orx_mod_lvl 1

# Specify the libtool library version
%define orx_current 3
%define orx_revision 2
%define orx_age 0
# the order of these looks wrong, but that is how it comes out!
%define orx_libversion %{orx_current}.%{orx_age}.%{orx_revision}

Name: ooRexx
Version: %{orx_major}.%{orx_minor}.%{orx_mod_lvl}
Release: 1
Summary: Open Object Rexx
Group: Development/Languages
License: CPL
URL: http://www.oorexx.org/
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
# If we don't include the following option we get bogus dependencies generated
Autoreq: 0

%description
Open Object Rexx is an object-oriented scripting language. The language
is designed for "non-programmer" type users, so it is easy to learn
and easy to use, and provides an excellent vehicle to enter the world
of object-oriented programming without much effort.

It extends the procedural way of programming with object-oriented
features that allow you to gradually change your programming style
as you learn more about objects.

For more information on ooRexx, visit http://www.oorexx.org/
For more information on Rexx, visit http://www.rexxla.org/
%prep
%setup -q

%build
# Do not remove the --prefix option! This is how we control where things get
# installed, either /opt/ooRexx or /usr
./configure --disable-static --prefix=%{_prefix}
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

#%prefix: /opt/ooRexx
#%prefix: /usr

%files
%defattr(-,root,root,-)
%doc
%{_prefix}/bin/rexx
%{_prefix}/bin/rexxc
%{_prefix}/bin/rxqueue
%{_prefix}/bin/rxsubcom
%{_prefix}/bin/rxdelipc
%{_prefix}/bin/rxmigrate
%{_prefix}/bin/rexx.img
%{_prefix}/bin/rexx.cat
%{_prefix}/bin/rxregexp.cls
%{_prefix}/bin/rxftp.cls
%{_prefix}/bin/oorexx-config
%{_prefix}/lib/ooRexx/librexx.so
%{_prefix}/lib/ooRexx/librexx.so.%{orx_current}
%{_prefix}/lib/ooRexx/librexx.so.%{orx_libversion}
%{_prefix}/lib/ooRexx/librexx.la
%{_prefix}/lib/ooRexx/librexxapi.so
%{_prefix}/lib/ooRexx/librexxapi.so.%{orx_current}
%{_prefix}/lib/ooRexx/librexxapi.so.%{orx_libversion}
%{_prefix}/lib/ooRexx/librexxapi.la
%{_prefix}/lib/ooRexx/librxsock.so
%{_prefix}/lib/ooRexx/librxsock.so.%{orx_current}
%{_prefix}/lib/ooRexx/librxsock.so.%{orx_libversion}
%{_prefix}/lib/ooRexx/librxsock.la
%{_prefix}/lib/ooRexx/librxmath.so
%{_prefix}/lib/ooRexx/librxmath.so.%{orx_current}
%{_prefix}/lib/ooRexx/librxmath.so.%{orx_libversion}
%{_prefix}/lib/ooRexx/librxmath.la
%{_prefix}/lib/ooRexx/librxregexp.so
%{_prefix}/lib/ooRexx/librxregexp.so.%{orx_current}
%{_prefix}/lib/ooRexx/librxregexp.so.%{orx_libversion}
%{_prefix}/lib/ooRexx/librxregexp.la
%{_prefix}/lib/ooRexx/librexxutil.so
%{_prefix}/lib/ooRexx/librexxutil.so.%{orx_current}
%{_prefix}/lib/ooRexx/librexxutil.so.%{orx_libversion}
%{_prefix}/lib/ooRexx/librexxutil.la
%{_prefix}/man/man1/rexx.1
%{_prefix}/man/man1/rexxc.1
%{_prefix}/man/man1/rxsubcom.1
%{_prefix}/man/man1/rxqueue.1
%{_prefix}/man/man1/rxmigrate.1
%{_prefix}/include/rexx.h
%{_prefix}/share/ooRexx/rexx.sh
%{_prefix}/share/ooRexx/rexx.csh
%{_prefix}/share/ooRexx/*.rex
%{_prefix}/share/ooRexx/readme
# We still need to figure out how to get the following installed somewhere in
# the install tree. Obviously the following does not work.
# %{_prefix}/share/ooRexx/unix/trexx
# %{_prefix}/share/ooRexx/unix/api/*.README
# %{_prefix}/share/ooRexx/unix/api/callrexx/*.c
# %{_prefix}/share/ooRexx/unix/api/callrexx/*.rex
# %{_prefix}/share/ooRexx/unix/api/wpipe1/*.c
# %{_prefix}/share/ooRexx/unix/api/wpipe1/*.rex
# %{_prefix}/share/ooRexx/unix/api/wpipe2/*.c
# %{_prefix}/share/ooRexx/unix/api/wpipe2/*.rex
# %{_prefix}/share/ooRexx/unix/api/wpipe3/*.c
# %{_prefix}/share/ooRexx/unix/api/wpipe3/*.rex

%post
# We only need to create the links if we are installed in /opt/ooRexx
if [ -d /opt/ooRexx ]; then
   ln -sf %{_prefix}/bin/rexx                             /usr/bin/rexx
   ln -sf %{_prefix}/bin/rexxc                            /usr/bin/rexxc
   ln -sf %{_prefix}/bin/rxqueue                          /usr/bin/rxqueue
   ln -sf %{_prefix}/bin/rxsubcom                         /usr/bin/rxsubcom
   ln -sf %{_prefix}/bin/rxdelipc                         /usr/bin/rxdelipc
   ln -sf %{_prefix}/bin/rxmigrate                        /usr/bin/rxmigrate
   ln -sf %{_prefix}/bin/rexx.img                         /usr/bin/rexx.img
   ln -sf %{_prefix}/bin/rexx.cat                         /usr/bin/rexx.cat
   ln -sf %{_prefix}/bin/rxregexp.cls                     /usr/bin/rxregexp.cls
   ln -sf %{_prefix}/bin/rxftp.cls                        /usr/bin/rxftp.cls
   ln -sf %{_prefix}/bin/oorexx-config                    /usr/bin/oorexx-config
   ln -sf %{_prefix}/lib/ooRexx/librexx.so.%{orx_libversion}     /usr/lib/librexx.so.%{orx_libversion}
   ln -sf %{_prefix}/lib/ooRexx/librexx.so.%{orx_libversion}     /usr/lib/librexx.so.%{orx_current}
   ln -sf %{_prefix}/lib/ooRexx/librexx.so.%{orx_libversion}     /usr/lib/librexx.so
   ln -sf %{_prefix}/lib/ooRexx/librexx.la                /usr/lib/librexx.la
   ln -sf %{_prefix}/lib/ooRexx/librexxapi.so.%{orx_libversion}  /usr/lib/librexxapi.so.%{orx_libversion}
   ln -sf %{_prefix}/lib/ooRexx/librexxapi.so.%{orx_libversion}  /usr/lib/librexxapi.so.%{orx_current}
   ln -sf %{_prefix}/lib/ooRexx/librexxapi.so.%{orx_libversion}  /usr/lib/librexxapi.so
   ln -sf %{_prefix}/lib/ooRexx/librexxapi.la             /usr/lib/librexxapi.la
   ln -sf %{_prefix}/lib/ooRexx/librxsock.so.%{orx_libversion}   /usr/lib/librxsock.so.%{orx_libversion}
   ln -sf %{_prefix}/lib/ooRexx/librxsock.so.%{orx_libversion}   /usr/lib/librxsock.so.%{orx_current}
   ln -sf %{_prefix}/lib/ooRexx/librxsock.so.%{orx_libversion}   /usr/lib/librxsock.so
   ln -sf %{_prefix}/lib/ooRexx/librxsock.la              /usr/lib/librxsock.la
   ln -sf %{_prefix}/lib/ooRexx/librxmath.so.%{orx_libversion}   /usr/lib/librxmath.so.%{orx_libversion}
   ln -sf %{_prefix}/lib/ooRexx/librxmath.so.%{orx_libversion}   /usr/lib/librxmath.so.%{orx_current}
   ln -sf %{_prefix}/lib/ooRexx/librxmath.so.%{orx_libversion}   /usr/lib/librxmath.so
   ln -sf %{_prefix}/lib/ooRexx/librxmath.la              /usr/lib/librxmath.la
   ln -sf %{_prefix}/lib/ooRexx/librxregexp.so.%{orx_libversion} /usr/lib/librxregexp.so.%{orx_libversion}
   ln -sf %{_prefix}/lib/ooRexx/librxregexp.so.%{orx_libversion} /usr/lib/librxregexp.so.%{orx_current}
   ln -sf %{_prefix}/lib/ooRexx/librxregexp.so.%{orx_libversion} /usr/lib/librxregexp.so
   ln -sf %{_prefix}/lib/ooRexx/librxregexp.la            /usr/lib/librxregexp.la
   ln -sf %{_prefix}/lib/ooRexx/librexxutil.so.%{orx_libversion} /usr/lib/librexxutil.so.%{orx_libversion}
   ln -sf %{_prefix}/lib/ooRexx/librexxutil.so.%{orx_libversion} /usr/lib/librexxutil.so.%{orx_current}
   ln -sf %{_prefix}/lib/ooRexx/librexxutil.so.%{orx_libversion} /usr/lib/librexxutil.so
   ln -sf %{_prefix}/lib/ooRexx/librexxutil.la            /usr/lib/librexxutil.la
   ln -sf %{_prefix}/man/man1/rexx.1                      /usr/share/man/man1/rexx.1
   ln -sf %{_prefix}/man/man1/rexxc.1                     /usr/share/man/man1/rexxc.1
   ln -sf %{_prefix}/man/man1/rxsubcom.1                  /usr/share/man/man1/rxsubcom.1
   ln -sf %{_prefix}/man/man1/rxqueue.1                   /usr/share/man/man1/rxqueue.1
   ln -sf %{_prefix}/man/man1/rxmigrate.1                 /usr/share/man/man1/rxmigrate.1
   ln -sf %{_prefix}/include/rexx.h                       /usr/include/rexx.h
   ln -sf %{_prefix}/share/ooRexx/rexxtry.rex             /usr/bin/rexxtry.rex
# allow backwards compatibility to Object REXX 2.x
   ln -sf %{_prefix}/lib/ooRexx/librexxapi.so.%{orx_libversion}  /usr/lib/librexxapi.so.2
fi
ldconfig

%preun
# Remove all our installed files/links
rm -f /usr/bin/rexx
rm -f /usr/bin/rexxc
rm -f /usr/bin/rxqueue
rm -f /usr/bin/rxsubcom
rm -f /usr/bin/rxdelipc
rm -f /usr/bin/rxmigrate
rm -f /usr/bin/rexx.img
rm -f /usr/bin/rexx.cat
rm -f /usr/bin/rxregexp.cls
rm -f /usr/bin/rxftp.cls
rm -f /usr/bin/oorexx-config
rm -f /usr/lib/librexx.*
rm -f /usr/lib/librexxapi.*
rm -f /usr/lib/librxsock.*
rm -f /usr/lib/librxmath.*
rm -f /usr/lib/librxregexp.*
rm -f /usr/lib/librexxutil.*
rm -f /usr/share/man/man1/rexx.1
rm -f /usr/share/man/man1/rexxc.1
rm -f /usr/share/man/man1/rxsubcom.1
rm -f /usr/share/man/man1/rxqueue.1
rm -f /usr/share/man/man1/rxmigrate.1
rm -f /usr/include/rexx.h
rm -f /usr/bin/rexxtry.rex
rm -rf /usr/share/ooRexx

%postun
ldconfig
# Do no change this to rm -rf %{_prefix}
# If you do you could wipe out the all the /usr subdirs!
if [ -d /opt/ooRexx ]; then
   rm -rf /opt/ooRexx
fi

%changelog

