#
# spec file for package xserver-xorg-video-imx-viv
#
# Copyright (c) 2014 Josua Mayer <privacy@not.given>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

Name: xserver-xorg-video-imx-viv
Version: 5.0.11.p4.4
Release: 1
License: MIT GPL
Group: System/X11/Servers/XF86_4
Summary: Vivante X.org driver
Source: xserver-xorg-video-imx-viv-%{version}.tar.gz
BuildRequires: xorg-x11-server-sdk libdrm-devel gpu-viv-bin-x11-devel

%description
Provides an X.org driver for Vivante GPUs using their galcore kernel interface.

%prep
%setup -q

%build
make -C EXA/src/ -f makefile.linux BUSID_HAS_NUMBER=1 XSERVER_GREATER_THAN_13=1 BUILD_HARD_VFP=1
make -C FslExt/src/ -f makefile.linux BUSID_HAS_NUMBER=1 XSERVER_GREATER_THAN_13=1 BUILD_HARD_VFP=1
make -C util/autohdmi/ -f makefile.linux BUSID_HAS_NUMBER=1 XSERVER_GREATER_THAN_13=1 BUILD_HARD_VFP=1
make -C util/pandisplay/ -f makefile.linux BUSID_HAS_NUMBER=1 XSERVER_GREATER_THAN_13=1 BUILD_HARD_VFP=1

%install
install -v -m755 -D EXA/src/vivante_drv.so $(DESTDIR)/usr/lib/xorg/modules/drivers/vivante_drv.so
install -v -m755 -D FslExt/src/libfsl_x11_ext.so $(DESTDIR)/usr/lib/xorg/modules/extensions/libfsl_x11_ext.so
install -v -m755 -D util/autohdmi/autohdmi $(DESTDIR)/usr/bin/autohdmi
install -v -m755 -D util/pandisplay/pandisplay $(DESTDIR)/usr/bin/pandisplay

%files
%defattr(-,root,root)
/usr/bin/autohdmi
/usr/bin/pandisplay
/usr/lib/xorg/modules/drivers/vivante_drv.so
/usr/lib/xorg/modules/extensions/libfsl_x11_ext.so

%changelog
