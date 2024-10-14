CONFIG += debug_and_release

unix {
    QMAKE_CXXFLAGS_RELEASE += -Wno-deprecated
    QMAKE_CXXFLAGS_DEBUG += -Wno-deprecated

    !macx:LIBS += -lGLU
}

win32 {
    # Per tenir glew integrada al codi. Només necessari a windows.
    DEFINES += GLEW_STATIC
    # Avoid definition of min and max macros by windows.h
    DEFINES += NOMINMAX
    
    # Volem veure missatges a std::cout en debug
    CONFIG(debug, debug|release) {
        CONFIG += console
    }
    else {
        CONFIG -= console
    }
    LIBS += -lws2_32 #dcmtk >= 3.6.3 need
    # Required since Qt 5.6 to use OpenGl directly
    LIBS += opengl32.lib glu32.lib
    # Required since Qt 5.6 by ITKCommon
    LIBS += gdi32.lib

    # we define that for visual studio-based windows compilation systems
    # compilation is done in as many cores as possible
    QMAKE_CXXFLAGS += /MP

    # We indicate that for debug compilations, Runtime Library
    # is Multi-threaded DLL (as in release) and not Multi-threaded Debug DLL
    QMAKE_CXXFLAGS_DEBUG -= -MDd
    QMAKE_CXXFLAGS_DEBUG += -MD

    QMAKE_CXXFLAGS -= -Zc:strictStrings
}

macx {
    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_LFLAGS += -stdlib=libc++
    LIBS += -framework Cocoa
}

# definim que per sistemes de compilació windows basats en visual studio 
# la compilació es faci en tants cores com sigui possible

#win32-msvc2013:QMAKE_CXXFLAGS += /MP

# Indiquem que per compil·lacions en debug, Runtime Library sigui Multi-threaded DLL (com en release) i no Multi-threaded Debug DLL
#win32-msvc2013:QMAKE_CXXFLAGS_DEBUG -= -MDd
#win32-msvc2013:QMAKE_CXXFLAGS_DEBUG += -MD

#win32-msvc2013:QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings

# Definim variable per construir versió lite en temps de compilació
lite_version:DEFINES += STARVIEWER_LITE

ce_marking:DEFINES += STARVIEWER_CE
