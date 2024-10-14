# Directoris per defecte de les diferents llibreries. Si vols que siguin uns altres, simplement has de declarar
# com a variables de sistema les que vulguis substituir. Ex.: export ITKLIBDIR=/usr/lib64/InsightToolkit


SDK_INSTALL_PREFIX = D:\SDK
isEmpty(SDK_INSTALL_PREFIX){
    unix:SDK_INSTALL_PREFIX = $$(HOME)/starviewer-sdk-0.15/usr/local
    win32:SDK_INSTALL_PREFIX = $$(USERPROFILE)/starviewer-sdk-0.15/32
    win32:contains(QMAKE_TARGET.arch, x86_64):SDK_INSTALL_PREFIX = $$(USERPROFILE)/starviewer-sdk-0.15/64
}

# DCMTK Libraries

DCMTKLIBDIR = $$(DCMTKLIBDIR)
isEmpty(DCMTKLIBDIR){
    unix:DCMTKLIBDIR = $$SDK_INSTALL_PREFIX/lib
    win32:DCMTKLIBDIR = $$SDK_INSTALL_PREFIX/starviewer-win64-5.0.1-8.2.0-3.0.0/dcmtk/3.6.5/lib
}
DCMTKINCLUDEDIR = $$(DCMTKINCLUDEDIR)
isEmpty(DCMTKINCLUDEDIR){
    unix:DCMTKINCLUDEDIR = $$SDK_INSTALL_PREFIX/include/dcmtk
    win32:DCMTKINCLUDEDIR = $$SDK_INSTALL_PREFIX/starviewer-win64-5.0.1-8.2.0-3.0.0/dcmtk/3.6.5/include/dcmtk
}

# VTK Libraries
VTKLIBDIR = $$(VTKLIBDIR)
isEmpty(VTKLIBDIR){
    unix:VTKLIBDIR = $$SDK_INSTALL_PREFIX/lib
    win32:VTKLIBDIR = $$SDK_INSTALL_PREFIX/vtk-v6.2.0_vc17static/lib/RelWithDebInfo
}
VTKINCLUDEDIR = $$(VTKINCLUDEDIR)
isEmpty(VTKINCLUDEDIR){
    unix:VTKINCLUDEDIR     = $$SDK_INSTALL_PREFIX/include/vtk-8.1
    win32:VTKINCLUDEDIR    = $$SDK_INSTALL_PREFIX/vtk-v6.2.0
    #win32:VTKINCLUDEDIR_VC = $$SDK_INSTALL_PREFIX/VTK-8.2.0
}

# ITK Libraries
ITKLIBDIR = $$(ITKLIBDIR)
isEmpty(ITKLIBDIR){
    unix:ITKLIBDIR = $$SDK_INSTALL_PREFIX/lib
    win32:ITKLIBDIR = $$SDK_INSTALL_PREFIX/ITK_5.3.0_lib
}
ITKINCLUDEDIR = $$(ITKINCLUDEDIR)
isEmpty(ITKINCLUDEDIR){
    unix:ITKINCLUDEDIR     = $$SDK_INSTALL_PREFIX/include/ITK-4.13
    win32:ITKINCLUDEDIR    = $$SDK_INSTALL_PREFIX/ITK-5.3.0_vc17_include
    #win32:ITKINCLUDEDIR_VC = $$SDK_INSTALL_PREFIX/ITK-5.3.0
}

# GDCM Libraries
GDCMLIBDIR = $$(GDCMLIBDIR)
isEmpty(GDCMLIBDIR){
    unix:GDCMLIBDIR = $$SDK_INSTALL_PREFIX/lib
    win32:GDCMLIBDIR = $$SDK_INSTALL_PREFIX/GDCM-2.8.9_vc17static/bin/RelWithDebInfo
}
GDCMINCLUDEDIR = $$(GDCMINCLUDEDIR)
isEmpty(GDCMINCLUDEDIR){
    unix:GDCMINCLUDEDIR = $$SDK_INSTALL_PREFIX/include/gdcm-3.0
    win32:GDCMINCLUDEDIR = $$SDK_INSTALL_PREFIX/GDCM-2.8.9
}

# Log4cxx Libraries

LOG4CXXLIBDIR = $$(LOG4CXXLIBDIR)
isEmpty(LOG4CXXLIBDIR){
    unix:LOG4CXXLIBDIR = /usr/lib
    macx:LOG4CXXLIBDIR = /usr/local/lib/
    win32:LOG4CXXLIBDIR = C:/log4cxx
    win32:contains(QMAKE_TARGET.arch, x86_64):LOG4CXXLIBDIR = C:/log4cxx-64
}
LOG4CXXINCLUDEDIR = $$(LOG4CXXINCLUDEDIR)
isEmpty(LOG4CXXINCLUDEDIR){
    unix:LOG4CXXINCLUDEDIR = /usr/include/log4cxx
    macx:LOG4CXXINCLUDEDIR = /usr/local/include
    win32:LOG4CXXINCLUDEDIR = C:/log4cxx/include
    win32:contains(QMAKE_TARGET.arch, x86_64):LOG4CXXINCLUDEDIR = C:/log4cxx-64/include
}

# Threadweaver libraries

THREADWEAVERLIBDIR = $$(THREADWEAVERLIBDIR)
isEmpty(THREADWEAVERLIBDIR){
    exists(/etc/debian_version):unix:THREADWEAVERLIBDIR = $$SDK_INSTALL_PREFIX/lib/x86_64-linux-gnu # Debian-based systems
    !exists(/etc/debian_version):unix:THREADWEAVERLIBDIR = $$SDK_INSTALL_PREFIX/lib64               # Other systems
    macx:THREADWEAVERLIBDIR = $$SDK_INSTALL_PREFIX/lib
    win32:THREADWEAVERLIBDIR = $$SDK_INSTALL_PREFIX/threadweaver-5.46.0_lib
}
THREADWEAVERINCLUDEDIR = $$(THREADWEAVERINCLUDEDIR)
isEmpty(THREADWEAVERINCLUDEDIR){
    unix:THREADWEAVERINCLUDEDIR     = $$SDK_INSTALL_PREFIX/include/KF5
    win32:THREADWEAVERINCLUDEDIR    = $$SDK_INSTALL_PREFIX/threadweaver-5.46.0_vc17_include
    #win32:THREADWEAVERINCLUDEDIR_VC = $$SDK_INSTALL_PREFIX/threadweaver-5.46.0
}

# CUDA Libraries

CUDALIBDIR = $$(CUDALIBDIR)
isEmpty(CUDALIBDIR){
    unix:CUDALIBDIR = /usr/local/cuda/lib
}
CUDAINCLUDEDIR = $$(CUDAINCLUDEDIR)
isEmpty(CUDAINCLUDEDIR){
    unix:CUDAINCLUDEDIR = /usr/local/cuda/include
}
# De moment cal el CUDA SDK, però s'hauria de poder treballar sense ell
CUDASDKINCLUDEDIR = $$(CUDASDKINCLUDEDIR)
isEmpty(CUDASDKINCLUDEDIR){
    unix:CUDASDKINCLUDEDIR = /usr/local/cuda-sdk/common/inc
}
