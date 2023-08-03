
INCFLAGS += -Iexternal/simde

SOURCES += main.cpp
SOURCES += ComplexRect.cpp
SOURCES += LayerBitmapBlit.cpp
SOURCES += LayerBitmapCopyRect.cpp
SOURCES += LayerBitmapFill.cpp
SOURCES += LayerBitmapFillMask.cpp
SOURCES += LayerBitmapBlendColor.cpp
SOURCES += LayerBitmapFillColor.cpp
SOURCES += LayerBitmapRemoveConstOpacity.cpp
SOURCES += LayerBitmapUtility.cpp
SOURCES += tvpgl.cpp
PROJECT_BASENAME = layerExRect

RC_LEGALCOPYRIGHT ?= Copyright (C) 2023-2023 Julian Uy; See details of license at license.txt, or the source code location.

include external/ncbind/Rules.lib.make
