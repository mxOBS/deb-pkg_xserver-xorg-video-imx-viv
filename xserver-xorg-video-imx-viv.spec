Name: xserver-xorg-video-imx-viv
Version: 1
Release: 1
License: MIT GPL
Group: System/X11/Servers/XF86_4
Summary: Vivante X.org driver
Source: xserver-xorg-video-imx-viv-%{version}.tar.gz
BuildRequires: xorg-x11-server-sdk gpu-viv-bin-x11-devel

%description
Provides an X.org driver for Vivante GPUs using their galcore kernel interface.

%prep
%setup -q

%build
make -C EXA/src/ -f makefile.linux BUSID_HAS_NUMBER=1 XSERVER_GREATER_THAN_13=1 BUILD_HARD_VFP=1 LFLAGS="-L/usr/lib/galcore"
make -C FslExt/src/ -f makefile.linux BUSID_HAS_NUMBER=1 XSERVER_GREATER_THAN_13=1 BUILD_HARD_VFP=1 LFLAGS="-L/usr/lib/galcore"
make -C util/autohdmi/ -f makefile.linux BUSID_HAS_NUMBER=1 XSERVER_GREATER_THAN_13=1 BUILD_HARD_VFP=1

%install
install -v -m755 -D EXA/src/vivante_drv.so $(DESTDIR)/usr/lib/xorg/modules/drivers/vivante_drv.so
install -v -m755 -D FslExt/src/libfsl_x11_ext.so $(DESTDIR)/usr/lib/xorg/modules/extensions/libfsl_x11_ext.so
install -v -m755 -D util/autohdmi/autohdmi $(DESTDIR)/usr/bin/autohdmi

%files
%defattr(-,root,root)
/usr/bin/autohdmi
/usr/lib/xorg/modules/drivers/vivante_drv.so
/usr/lib/xorg/modules/extensions/libfsl_x11_ext.so

%changelog
